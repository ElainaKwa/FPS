#include "FPSMatchFlow.h"
#include "FPSBotNavigation.h"
#include "FPSAISmoke.h"
#include "FPSWeaponComponent.h"

#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "EngineUtils.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "FPSFlowSmoke.h"
#include "FPSWeaponPickup.h"
#include "FPSWeaponData.h"
#include "FPSWeaponSmoke.h"
#include "Engine/GameInstance.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

AFPSLobbyMode::AFPSLobbyMode()
{
    GameStateClass = AFPSLobbyState::StaticClass();
    PlayerControllerClass = AFPSFrontController::StaticClass();
    DefaultPawnClass = nullptr;
    bStartPlayersAsSpectators = true;
}

AFPSMatchMode::AFPSMatchMode()
{
    // The saved teaching Blueprint still serializes the original movement component class.
    // Use the native avatar for the new predicted movement and weapon components.
    DefaultPawnClass = AFPSDemoCharacter::StaticClass();
    PlayerControllerClass = AFPSFrontController::StaticClass();
}

void AFPSMatchMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    ModeName = UGameplayStatics::ParseOption(Options, TEXT("Mode"));
    if (ModeName.IsEmpty() && MapName.Contains(TEXT("LV_FPSTraining"))) ModeName = TEXT("Training");
    if (ModeName != TEXT("LAN") && ModeName != TEXT("Training")) ModeName = TEXT("Solo");
    BotCount = FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Bots"), 3), 0, 9);
    RoomName = UGameplayStatics::ParseOption(Options, TEXT("Room")).Left(24);
    if (RoomName.IsEmpty()) RoomName = TEXT("FPS Room");
}

void AFPSMatchMode::InitGameState()
{
    Super::InitGameState();
    auto* GS = GetGameState<AFPSDemoGameState>();
    GS->bStructuredMatch = true;
    GS->bTraining = ModeName == TEXT("Training");
    GS->bWaiting = ModeName == TEXT("LAN");
    GS->RoomName = RoomName;
}

void AFPSMatchMode::BeginPlay()
{
    Super::BeginPlay();
    auto* GS = GetGameState<AFPSDemoGameState>();
    const auto* Catalog = GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    bool bHasWeaponMarkers = false;
    for (TActorIterator<ATargetPoint> It(GetWorld()); It; ++It)
        for (int32 Index = 1; Index <= 6; ++Index)
            if (It->ActorHasTag(FName(*FString::Printf(TEXT("FPSWeapon%d"), Index))))
            {
                bHasWeaponMarkers = true;
                auto* Pickup = GetWorld()->SpawnActor<AFPSWeaponPickup>(It->GetActorLocation(), It->GetActorRotation());
                if (Pickup) Pickup->Initialize(Catalog->CreateInstance(static_cast<EFPSWeaponModel>(Index)));
            }
    if (!bHasWeaponMarkers)
    for (int32 Side = 0; Side < (GS->bTraining ? 1 : 2); ++Side)
        for (int32 Index = 1; Index <= 6; ++Index)
        {
            auto* Pickup = GetWorld()->SpawnActor<AFPSWeaponPickup>(FVector(Side == 0 ? -1860 : 1860, -1200 + Index * 320, 30), FRotator(0, Side == 0 ? 0 : 180, 0));
            if (Pickup) Pickup->Initialize(Catalog->CreateInstance(static_cast<EFPSWeaponModel>(Index)));
        }
    if (ModeName == TEXT("Solo"))
    {
        SpawnBots();
        GS->RoundEndsAt = GS->GetServerWorldTimeSeconds() + 300;
        GetWorldTimerManager().SetTimer(RoundTimer, this, &AFPSMatchMode::FinishRound, 300, false);
    }
    else if (GS->bTraining)
    {
        // Passive practice dummies, not connected players. Same GAS damage/respawn path.
        BotCount = 3;
        SpawnBots();
    }
    FFPSFlowSmoke::Start(this);
    FFPSWeaponSmoke::Start(this);
    FFPSAISmoke::Start(this);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("FPSMapSmoke")))
    {
        FTimerHandle MapCheck;
        GetWorldTimerManager().SetTimer(MapCheck, [this]()
        {
            int32 Starts = 0, Pickups = 0, Pawns = 0;
            bool bClear = true;
            FCollisionQueryParams SpawnQuery;
            for (TActorIterator<AFPSDemoCharacter> It(GetWorld()); It; ++It) { ++Pawns; SpawnQuery.AddIgnoredActor(*It); }
            for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
            {
                ++Starts;
                const FVector P = It->GetActorLocation();
                const FRotator Rotation = It->GetActorRotation();
                bClear &= FMath::Abs(Rotation.Roll) < .1f && FMath::Abs(Rotation.Pitch) < .1f;
                SpawnQuery.AddIgnoredActor(*It);
                bClear &= !GetWorld()->OverlapBlockingTestByChannel(P, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(38, 88), SpawnQuery);
                FHitResult Floor;
                bClear &= GetWorld()->LineTraceSingleByObjectType(Floor, P, P - FVector(0, 0, 200), FCollisionObjectQueryParams(ECC_WorldStatic));
            }
            for (TActorIterator<AFPSWeaponPickup> It(GetWorld()); It; ++It) ++Pickups;
            const bool bTraining = GetGameState<AFPSDemoGameState>()->bTraining;
            const bool bPass = bClear && Starts == (bTraining ? 4 : 10) && Pickups == (bTraining ? 6 : 12) && Pawns == (bTraining ? 4 : BotCount + 1);
            UE_LOG(LogTemp, Display, TEXT("FPS_MAP_SMOKE %s map=%s starts=%d pickups=%d pawns=%d clear=%d"), bPass ? TEXT("PASS") : TEXT("FAIL"), *GetWorld()->GetMapName(), Starts, Pickups, Pawns, bClear);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/WindowsEditor/") / (bTraining ? TEXT("TrainingMap.png") : TEXT("FoundryMap.png")), false, false);
        }, 8.f, false);
    }
#endif
}

void AFPSMatchMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
    if (!ErrorMessage.IsEmpty()) return;
    if (ModeName != TEXT("LAN")) ErrorMessage = TEXT("This is a single-player mode.");
    else if (GetNumPlayers() >= 10) ErrorMessage = TEXT("Room full (10/10).");
    else if (GetGameState<AFPSDemoGameState>()->bFinished) ErrorMessage = TEXT("Round has ended.");
}

int32 AFPSMatchMode::TeamSize(int32 Team) const
{
    int32 Count = 0;
    for (const APlayerState* Base : GameState->PlayerArray)
        if (const auto* PS = Cast<AFPSDemoPlayerState>(Base))
            if (!PS->IsInactive() && PS->TeamId == Team) ++Count;
    return Count;
}

FString AFPSMatchMode::InitNewPlayer(APlayerController* NewPlayer, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
    // Recheck at admission, because multiple PreLogin handshakes can overlap.
    if (TeamSize(0) >= 5 && TeamSize(1) >= 5) return TEXT("Room full (10/10).");
    if (UGameplayStatics::HasOption(Options, TEXT("SpectatorOnly"))) return TEXT("Spectator slots are not supported.");
    if (GetGameState<AFPSDemoGameState>()->bFinished) return TEXT("Round has ended.");
    const FString Error = Super::InitNewPlayer(NewPlayer, UniqueId, Options, Portal);
    if (!Error.IsEmpty()) return Error;
    if (auto* PS = NewPlayer->GetPlayerState<AFPSDemoPlayerState>())
    {
        PS->TeamId = TeamSize(0) <= TeamSize(1) ? 0 : 1;
        PS->ForceNetUpdate();
    }
    return FString();
}

void AFPSMatchMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    if (!GetGameState<AFPSDemoGameState>()->bWaiting)
        Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void AFPSMatchMode::RestartPlayer(AController* NewPlayer)
{
    if (!NewPlayer || !IsCombatActive()) return;
    auto* PS = NewPlayer->GetPlayerState<AFPSDemoPlayerState>();
    const int32 Team = PS ? PS->TeamId : 0;
    const int32 Lane = PS ? FMath::Abs(PS->GetPlayerId()) % 5 : 0;
    FName SpawnTag(*FString::Printf(TEXT("FPS%s%d"), Team == 0 ? TEXT("Blue") : TEXT("Red"), Lane));
    if (ModeName == TEXT("Training"))
        SpawnTag = NewPlayer->IsPlayerController() ? FName(TEXT("FPSTrainingPlayer"))
            : FName(*FString::Printf(TEXT("FPSTarget%d"), PS ? FMath::Clamp(FCString::Atoi(*PS->GetPlayerName().Right(2)) - 1, 0, 2) : 0));
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
        if (It->ActorHasTag(SpawnTag))
        {
            RestartPlayerAtTransform(NewPlayer, It->GetActorTransform());
            return;
        }
    // Keep teams at opposite ends of the existing arena, outside its cover geometry.
    const FVector Position(Team == 0 ? -1650.f : 1650.f, -1000.f + Lane * 470.f, 110.f);
    RestartPlayerAtTransform(NewPlayer, FTransform(FRotator(0, Team == 0 ? 0 : 180, 0), Position));
}

bool AFPSMatchMode::IsCombatActive() const
{
    const auto* GS = GetGameState<AFPSDemoGameState>();
    return GS && !GS->bWaiting && !GS->bFinished && (GS->bTraining || GS->RoundEndsAt <= 0 || GS->GetServerWorldTimeSeconds() < GS->RoundEndsAt);
}

bool AFPSMatchMode::CanDamage(const AFPSDemoCharacter* Attacker, const AFPSDemoCharacter* Victim) const
{
    if (!IsCombatActive() || !Attacker || !Victim || Attacker == Victim) return false;
    const auto* A = Attacker->GetPlayerState<AFPSDemoPlayerState>();
    const auto* B = Victim->GetPlayerState<AFPSDemoPlayerState>();
    return A && B && A->TeamId != B->TeamId;
}

void AFPSMatchMode::SwitchTeam(APlayerController* Requester)
{
    if (!Requester || !GetGameState<AFPSDemoGameState>()->bWaiting) return;
    if (auto* PS = Requester->GetPlayerState<AFPSDemoPlayerState>())
    {
        const int32 Destination = 1 - PS->TeamId;
        if (TeamSize(Destination) < 5) { PS->TeamId = Destination; PS->ForceNetUpdate(); }
    }
}

void AFPSMatchMode::StartRound(APlayerController* Requester)
{
    auto* GS = GetGameState<AFPSDemoGameState>();
    // Only the listen server's local controller may start the room.
    if (!Requester || !Requester->IsLocalController() || !GS->bWaiting || TeamSize(0) == 0 || TeamSize(1) == 0) return;
    GS->bWaiting = false;
    GS->RoundEndsAt = GS->GetServerWorldTimeSeconds() + 300;
    GS->ForceNetUpdate();
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        RestartPlayer(It->Get());
    GetWorldTimerManager().SetTimer(RoundTimer, this, &AFPSMatchMode::FinishRound, 300, false);
}

void AFPSMatchMode::FinishRound()
{
    auto* GS = GetGameState<AFPSDemoGameState>();
    GS->bFinished = true;
    GS->Podium.Reset();
    for (const APlayerState* Base : GS->PlayerArray)
        if (const auto* PS = Cast<AFPSDemoPlayerState>(Base))
        {
            FFPSResult Result;
            Result.Name = PS->GetPlayerName();
            Result.Kills = PS->GetEliminationCount();
            Result.Deaths = PS->GetDeathCount();
            Result.PlayerId = PS->GetPlayerId();
            GS->Podium.Add(Result);
        }
    GS->Podium.Sort([](const FFPSResult& A, const FFPSResult& B)
    {
        if (A.Kills != B.Kills) return A.Kills > B.Kills;
        if (A.Deaths != B.Deaths) return A.Deaths < B.Deaths;
        return A.PlayerId < B.PlayerId;
    });
    if (GS->Podium.Num() > 3) GS->Podium.SetNum(3);
    GS->ForceNetUpdate();
    for (TActorIterator<AFPSDemoCharacter> It(GetWorld()); It; ++It)
        It->GetCharacterMovement()->DisableMovement();
}

void AFPSMatchMode::SpawnBots()
{
    for (int32 Index = 0; Index < BotCount; ++Index)
    {
        auto* Bot = GetWorld()->SpawnActor<AFPSBotController>();
        if (!Bot) continue;
        if (auto* PS = Bot->GetPlayerState<AFPSDemoPlayerState>())
        {
            PS->SetPlayerName(FString::Printf(TEXT("%s %02d"), ModeName == TEXT("Training") ? TEXT("Target") : TEXT("Bot"), Index + 1));
            PS->TeamId = ModeName == TEXT("Training") ? 1 : (TeamSize(0) <= TeamSize(1) ? 0 : 1);
            RestartPlayer(Bot);
        }
    }
}

AFPSBotController::AFPSBotController()
{
    bWantsPlayerState = true;
    PrimaryActorTick.bCanEverTick = true;
}

void AFPSBotController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("FPSFlowSmoke")) || FParse::Param(FCommandLine::Get(), TEXT("FPSMapSmoke"))) return;
#endif
    auto* Rules = GetWorld()->GetAuthGameMode<AFPSMatchMode>();
    auto* Self = Cast<AFPSDemoCharacter>(GetPawn());
    const auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
    if (!HasAuthority() || !Rules || !GS || GS->bTraining || !Rules->IsCombatActive() || !Self || Self->IsEliminated()) return;
    const float Now = GetWorld()->GetTimeSeconds();
    auto* Navigation = GetWorld()->GetSubsystem<UFPSBotNavigation>();
    auto* Weapon = Self->GetWeapons();
    if (PreviousPawn.Get() != Self)
    {
        PreviousPawn = Self; Enemy.Reset(); Route.Reset(); RouteIndex = 0;
        NextRoute = NextSense = NextShot = 0; BurstShots = 0;
        ProgressLocation = Self->GetActorLocation(); ProgressTime = Now;
        PatrolIndex = GetPlayerState<AFPSDemoPlayerState>() ? GetPlayerState<AFPSDemoPlayerState>()->GetPlayerId() % 4 : 0;
    }
    if (Now >= NextSense)
    {
        NextSense = Now + .25f;
        AFPSDemoCharacter* Visible = nullptr;
        float Best = FMath::Square(3200.f);
        for (TActorIterator<AFPSDemoCharacter> It(GetWorld()); It; ++It)
        {
            if (It->IsEliminated() || !Rules->CanDamage(Self, *It)) continue;
            float Distance = FVector::DistSquared2D(Self->GetActorLocation(), It->GetActorLocation());
            if (*It == Enemy.Get()) Distance *= .7f; // retain targets instead of switching every scan
            if (Distance >= Best || !LineOfSightTo(*It)) continue;
            Best = Distance; Visible = *It;
        }
        bVisibleEnemy = Visible != nullptr;
        if (!bVisibleEnemy) ClearFocus(EAIFocusPriority::Gameplay);
        if (Visible)
        {
            if (Enemy.Get() != Visible) { ReactionUntil = Now + .45f; BurstShots = 0; }
            Enemy = Visible; LastSeenTime = Now; LastSeenLocation = Visible->GetActorLocation();
        }
        else if (!Enemy.IsValid() || Enemy->IsEliminated() || Now - LastSeenTime > 3.f) Enemy.Reset();
    }
    // Pursue only a visible opponent or its last observed position. Otherwise patrol.
    static const FVector Patrol[] = {FVector(-1050,-1250,90), FVector(1050,-1250,90), FVector(1050,1250,90), FVector(-1050,1250,90)};
    if (Enemy.IsValid()) Destination = LastSeenLocation;
    else
    {
        if (FVector::DistSquared2D(Self->GetActorLocation(), Patrol[PatrolIndex % 4]) < FMath::Square(180.f))
        { ++PatrolIndex; NextRoute = 0; }
        Destination = Patrol[PatrolIndex % 4];
    }
    bool bNeedAmmo = false;
    if (Weapon->Equipped().Magazine <= 0)
    {
        if (Weapon->Equipped().Reserve > 0) Weapon->Reload();
        else if (Weapon->GetLoadout().ActiveSlot == EFPSWeaponSlot::Primary &&
            Weapon->GetLoadout().Secondary.Magazine + Weapon->GetLoadout().Secondary.Reserve > 0) Weapon->Secondary();
        else bNeedAmmo = true;
    }
    if (bNeedAmmo)
    {
        AFPSWeaponPickup* Ammo = nullptr;
        float Nearest = MAX_flt;
        for (TActorIterator<AFPSWeaponPickup> It(GetWorld()); It; ++It)
        {
            const float D = FVector::DistSquared2D(Self->GetActorLocation(), It->GetActorLocation());
            if (It->IsAvailable() && It->GetInstance().Magazine + It->GetInstance().Reserve > 0 && D < Nearest)
            { Nearest = D; Ammo = *It; }
        }
        if (Ammo)
        {
            // Stop beside the display plinth rather than asking for a path inside it.
            const FVector Toward = (Self->GetActorLocation() - Ammo->GetActorLocation()).GetSafeNormal2D();
            Destination = Ammo->GetActorLocation() + Toward * 125;
            if (Nearest < FMath::Square(180.f)) Weapon->Request(EFPSWeaponRequest::Pickup, Ammo);
        }
    }
    const bool bEngage = Enemy.IsValid() && bVisibleEnemy && !bNeedAmmo &&
        FVector::DistSquared2D(Self->GetActorLocation(), LastSeenLocation) < FMath::Square(1450.f);
    if (!bEngage)
    {
        if (Now >= NextRoute)
        {
            NextRoute = Now + .9f;
            Navigation->FindRoute(Self->GetActorLocation(), Destination, Route);
            RouteIndex = 0;
        }
        while (RouteIndex < Route.Num() && FVector::DistSquared2D(Self->GetActorLocation(), Route[RouteIndex]) < FMath::Square(65.f))
        {
            if (RouteIndex + 1 < Route.Num() && !Navigation->ClearSegment(Self->GetActorLocation(), Route[RouteIndex + 1])) break;
            ++RouteIndex;
        }
        if (RouteIndex < Route.Num())
        {
            FVector Direction = (Route[RouteIndex] - Self->GetActorLocation()).GetSafeNormal2D();
            // A small local separation steers around teammates without changing the static grid.
            for (TActorIterator<AFPSDemoCharacter> It(GetWorld()); It; ++It)
                if (*It != Self && !It->IsEliminated())
                {
                    const FVector Away = Self->GetActorLocation() - It->GetActorLocation();
                    const float Distance = Away.Size2D();
                    if (Distance > 1 && Distance < 120) Direction += Away.GetSafeNormal2D() * (1.f - Distance / 120.f);
                }
            Direction = Direction.GetSafeNormal2D();
            if (!Navigation->ClearSegment(Self->GetActorLocation(), Self->GetActorLocation() + Direction * 65))
                Direction = (Route[RouteIndex] - Self->GetActorLocation()).GetSafeNormal2D();
            Self->AddMovementInput(Direction, 1.f);
            if (!bVisibleEnemy)
            {
                ClearFocus(EAIFocusPriority::Gameplay);
                SetControlRotation(Direction.Rotation());
                Self->FaceRotation(GetControlRotation(), DeltaSeconds);
            }
        }
        if (Now - ProgressTime > 2.f)
        {
            if (FVector::DistSquared2D(Self->GetActorLocation(), ProgressLocation) < FMath::Square(80.f))
            { NextRoute = 0; if (!Enemy.IsValid()) ++PatrolIndex; }
            ProgressTime = Now; ProgressLocation = Self->GetActorLocation();
        }
    }
    if (Enemy.IsValid() && bVisibleEnemy)
    {
        SetFocus(Enemy.Get());
        const FRotator Aim = (Enemy->GetPawnViewLocation() - Self->GetPawnViewLocation()).Rotation();
        SetControlRotation(FMath::RInterpTo(GetControlRotation(), Aim, DeltaSeconds, 9.f));
        Self->FaceRotation(GetControlRotation(), DeltaSeconds);
        if (bEngage && Now >= NextShot && Now >= ReactionUntil && Weapon->Equipped().Magazine > 0)
        {
            FHitResult Hit;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(FPSBotAim), true, Self);
            GetWorld()->LineTraceSingleByChannel(Hit, Self->GetPawnViewLocation(), Enemy->GetPawnViewLocation(), ECC_Visibility, Query);
            if (Hit.GetActor() == Enemy.Get() && FMath::Abs(FRotator::NormalizeAxis(GetControlRotation().Yaw - Aim.Yaw)) < 5.f)
            {
                // Brief bursts with modest spread. Damage still goes through the normal server shot path.
                SetControlRotation(Aim + FRotator(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.5f, 1.5f), 0));
                Self->FireBotShot();
                NextShot = Now + (++BurstShots >= 3 ? .85f : .24f);
                if (BurstShots >= 3) BurstShots = 0;
            }
            else NextShot = Now + .1f;
        }
    }
}
