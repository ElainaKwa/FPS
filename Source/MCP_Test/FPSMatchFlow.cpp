#include "FPSMatchFlow.h"
#include "FPSAcceptanceSmoke.h"
#include "FPSCharacterSmoke.h"
#include "FPSLoadoutSmoke.h"
#include "FPSDynamicDrops.h"
#include "FPSDropSmoke.h"
#include "FPSModeRules.h"
#include "FPSBaseRegion.h"
#include "Camera/CameraActor.h"
#include "FPSBotSettings.h"
#include "FPSAISmoke.h"
#include "FPSDifficultySmoke.h"
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
    const FString DifficultyOption = UGameplayStatics::ParseOption(Options, TEXT("Difficulty"));
    Difficulty = DifficultyOption == TEXT("Easy") ? EFPSDifficulty::Easy : DifficultyOption == TEXT("Hard") ? EFPSDifficulty::Hard : EFPSDifficulty::Normal;
    BotPlayerDamageMultiplier = GetDefault<UFPSDifficultySettings>()->Multiplier(Difficulty);
    if (!FMath::IsFinite(BotPlayerDamageMultiplier) || BotPlayerDamageMultiplier <= 0) ErrorMessage = TEXT("Invalid bot damage difficulty configuration");
    if (!GetDefault<UFPSBotSettings>()->IsValid()) ErrorMessage = TEXT("Invalid FPS bot navigation configuration");
    BotCount = FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Bots"), 3), 0, 9);
    RoomName = UGameplayStatics::ParseOption(Options, TEXT("Room")).Left(24);
    if (RoomName.IsEmpty()) RoomName = TEXT("FPS Room");
}

void AFPSMatchMode::InitGameState()
{
    Super::InitGameState();
    auto* GS = GetGameState<AFPSDemoGameState>();
    GS->bStructuredMatch = true;
    TArray<FString> ModeErrors;
    ModeRules = UFPSModeRules::ForWorld(GetWorld(), &ModeErrors);
    GS->bTraining = ModeRules && ModeRules->Kind == EFPSGameplayKind::Training;
    GS->bWaiting = ModeName == TEXT("LAN");
    GS->RoomName = RoomName;
    auto* Catalog = GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    Catalog->PrepareWorld(GetWorld(), true);
    GS->WeaponDataError = Catalog->ValidationError;
    if (ModeRules && !GS->bTraining) AFPSBaseRegion::ValidateWorld(GetWorld(), ModeErrors);
    if (!ModeErrors.IsEmpty()) GS->WeaponDataError += FString::Join(ModeErrors, TEXT("\n"));
    if (!GS->WeaponDataError.IsEmpty()) UE_LOG(LogTemp, Error, TEXT("FPS_WEAPON_DATA MATCH BLOCKED %s"), *GS->WeaponDataError);
}

void AFPSMatchMode::BeginPlay()
{
    Super::BeginPlay();
    auto* GS = GetGameState<AFPSDemoGameState>();
    const auto* Catalog = GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    if (!GS->WeaponDataError.IsEmpty())
    {
#if !UE_BUILD_SHIPPING
        if (FParse::Param(FCommandLine::Get(), TEXT("FPSWeaponInvalidDataTest")))
        {
            FTimerHandle CheckTimer;
            GetWorldTimerManager().SetTimer(CheckTimer, [this]()
            {
                int32 Pawns = 0, Pickups = 0;
                for (TActorIterator<AFPSDemoCharacter> It(GetWorld()); It; ++It) ++Pawns;
                for (TActorIterator<AFPSWeaponPickup> It(GetWorld()); It; ++It) ++Pickups;
                UE_LOG(LogTemp, Display, TEXT("FPS_WEAPON_INVALID %s pawns=%d pickups=%d combat=%d"), Pawns == 0 && Pickups == 0 && !IsCombatActive() ? TEXT("PASS") : TEXT("FAIL"), Pawns, Pickups, IsCombatActive());
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/WindowsEditor/WeaponDataBlocked.png"), false, false);
            }, 3.f, false);
        }
#endif
        return;
    }
    for (TActorIterator<AFPSWeaponStand> It(GetWorld()); It; ++It)
    {
        auto* Pickup = GetWorld()->SpawnActor<AFPSWeaponPickup>(It->GetActorLocation(), It->GetActorRotation());
        if (Pickup) Pickup->Initialize(Catalog->CreateInstance(It->Definition->WeaponId));
    }
    if (!GS->bTraining && ModeName != TEXT("LAN"))
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
    FFPSCharacterSmoke::Start(this);
    FFPSLoadoutSmoke::Start(this);
    FFPSDropSmoke::Start(this);
    FFPSWeaponSmoke::Start(this);
    FFPSAISmoke::Start(this);
    FFPSDifficultySmoke::Start(this);
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
            const bool bPass = bClear && Starts == (bTraining ? 4 : 10) && Pickups == (bTraining ? 7 : 14) && Pawns == (bTraining ? 4 : BotCount + 1);
            UE_LOG(LogTemp, Display, TEXT("FPS_MAP_SMOKE %s map=%s starts=%d pickups=%d pawns=%d clear=%d"), bPass ? TEXT("PASS") : TEXT("FAIL"), *GetWorld()->GetMapName(), Starts, Pickups, Pawns, bClear);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/WindowsEditor/") / (bTraining ? TEXT("TrainingMap.png") : TEXT("FoundryMap.png")), false, false);
            if (!bTraining && FParse::Param(FCommandLine::Get(), TEXT("FPSRegionCapture")))
            {
                // Dedicated visual evidence for the new boundaries; ordinary spawn screenshots do not face the signs.
                for (int32 Team = 0; Team < 2; ++Team)
                {
                    FTimerHandle ViewTimer;
                    GetWorldTimerManager().SetTimer(ViewTimer, FTimerDelegate::CreateWeakLambda(this, [this, Team]()
                    {
                        const FVector Location(Team == 0 ? -1100 : 1100, -650, 700);
                        const FVector Target(Team == 0 ? -2050 : 2050, 0, 190);
                        auto* Camera = GetWorld()->SpawnActor<ACameraActor>(Location, (Target - Location).Rotation());
                        if (Camera) GetWorld()->GetFirstPlayerController()->SetViewTarget(Camera);
                        FTimerHandle CaptureTimer;
                        GetWorldTimerManager().SetTimer(CaptureTimer, FTimerDelegate::CreateWeakLambda(this, [Team]()
                        {
                            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/WindowsEditor/") /
                                (Team == 0 ? TEXT("CharacterBlueRegion.png") : TEXT("CharacterRedRegion.png")), false, false);
                        }), .8f, false);
                    }), 2.f + Team * 2.f, false);
                }
            }
        }, 8.f, false);
    }
#endif
}

void AFPSMatchMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (auto* PC = Cast<AFPSFrontController>(NewPlayer)) PC->BeginWeaponContentCheck();
}
void AFPSMatchMode::Logout(AController* Exiting)
{
    if (auto* Timer = SpawnRetries.Find(Exiting)) GetWorldTimerManager().ClearTimer(*Timer);
    SpawnRetries.Remove(Exiting);
    Super::Logout(Exiting);
}

void AFPSMatchMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
    if (const auto* GS = GetGameState<AFPSDemoGameState>(); GS && !GS->WeaponDataError.IsEmpty()) ErrorMessage = GS->WeaponDataError;
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
    const auto* PC = Cast<AFPSFrontController>(NewPlayer);
    if (PC && PC->IsWeaponContentVerified() && !GetGameState<AFPSDemoGameState>()->bWaiting)
        Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void AFPSMatchMode::RestartPlayer(AController* NewPlayer)
{
    if (!NewPlayer || !IsCombatActive()) return;
    if (const auto* PC = Cast<AFPSFrontController>(NewPlayer); PC && !PC->IsWeaponContentVerified()) return;
    auto* PS = NewPlayer->GetPlayerState<AFPSDemoPlayerState>();
    const int32 Team = PS ? PS->TeamId : 0;
    const int32 Lane = PS ? FMath::Abs(PS->GetPlayerId()) % 5 : 0;
    FName SpawnTag(*FString::Printf(TEXT("FPS%s%d"), Team == 0 ? TEXT("Blue") : TEXT("Red"), Lane));
    if (ModeRules && ModeRules->Kind == EFPSGameplayKind::Training)
        SpawnTag = NewPlayer->IsPlayerController() ? FName(TEXT("FPSTrainingPlayer"))
            : FName(*FString::Printf(TEXT("FPSTarget%d"), PS ? FMath::Clamp(FCString::Atoi(*PS->GetPlayerName().Right(2)) - 1, 0, 2) : 0));
    if (ModeRules && ModeRules->Kind == EFPSGameplayKind::TeamMatch)
    {
        for (TActorIterator<AFPSBaseRegion> It(GetWorld()); It; ++It)
            if (It->TeamId == Team)
            {
                FTransform Spawn;
                if (It->FindSpawn(NewPlayer, Spawn))
                {
                    if (auto* Retry = SpawnRetries.Find(NewPlayer)) GetWorldTimerManager().ClearTimer(*Retry);
                    SpawnRetries.Remove(NewPlayer);
                    RestartPlayerAtTransform(NewPlayer, Spawn);
                }
                else
                {
                    auto& Timer = SpawnRetries.FindOrAdd(NewPlayer);
                    if (!GetWorldTimerManager().IsTimerActive(Timer))
                        GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(NewPlayer, [this, NewPlayer]()
                        { SpawnRetries.Remove(NewPlayer); if (IsCombatActive() && !NewPlayer->GetPawn()) RestartPlayer(NewPlayer); }), .25f, false);
                }
                return;
            }
        return;
    }
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
        if (It->ActorHasTag(SpawnTag)) { RestartPlayerAtTransform(NewPlayer, It->GetActorTransform()); return; }
}

bool AFPSMatchMode::IsCombatActive() const
{
    const auto* GS = GetGameState<AFPSDemoGameState>();
    return GS && GS->IsCombatActive();
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
    // Player requests retain the existing listen-host-only rule.
    if (!Requester || !Requester->IsLocalController()) return;
    BeginRound();
}

void AFPSMatchMode::TryStartDedicatedRound()
{
    if (GetNetMode() == NM_DedicatedServer) BeginRound();
}

void AFPSMatchMode::BeginRound()
{
    auto* GS = GetGameState<AFPSDemoGameState>();
    if (!HasAuthority() || !GS->WeaponDataError.IsEmpty() || !GS->bWaiting || TeamSize(0) == 0 || TeamSize(1) == 0) return;
    if (!FFPSAcceptanceSmoke::ReadyToStart(GetWorld())) return;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (const auto* PC = Cast<AFPSFrontController>(It->Get()); PC && !PC->IsWeaponContentVerified()) return;
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
    GetWorld()->GetSubsystem<UFPSDynamicDrops>()->ClearRound();
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
            const bool bTraining = ModeRules && ModeRules->Kind == EFPSGameplayKind::Training;
            PS->SetPlayerName(FString::Printf(TEXT("%s %02d"), bTraining ? TEXT("Target") : TEXT("Bot"), Index + 1));
            PS->TeamId = bTraining ? 1 : (TeamSize(0) <= TeamSize(1) ? 0 : 1);
            RestartPlayer(Bot);
        }
    }
}

float AFPSMatchMode::DamageMultiplier(const AFPSDemoCharacter* Attacker, const AFPSDemoCharacter* Victim) const
{
    return ModeRules && ModeRules->Kind == EFPSGameplayKind::TeamMatch && Attacker && Victim && Cast<AFPSBotController>(Attacker->GetController()) &&
        Victim->GetController() && Victim->GetController()->IsPlayerController() ? BotPlayerDamageMultiplier : 1.f;
}
