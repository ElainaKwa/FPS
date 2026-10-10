#include "FPSAcceptanceSmoke.h"
#include "FPSMatchFlow.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponPickup.h"
#include "FPSModeRules.h"
#include "FPSBaseRegion.h"
#include "FPSBaseMembership.h"
#include "FPSDynamicDrops.h"
#include "Engine/NetDriver.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "HAL/PlatformMemory.h"
#include "Misc/CommandLine.h"

bool FFPSAcceptanceSmoke::ReadyToStart(UWorld* World)
{
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("FPSAcceptance")))
    {
        int32 Expected = 10, Controllers = 0;
        FParse::Value(FCommandLine::Get(), TEXT("FPSExpectedPlayers="), Expected);
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
        {
            const auto* PC = Cast<AFPSFrontController>(It->Get());
            if (!PC || !PC->IsWeaponContentVerified()) return false;
            ++Controllers;
        }
        const auto* GS = World->GetGameState<AFPSDemoGameState>();
        if (!GS || Controllers != Expected || GS->PlayerArray.Num() != Expected) return false;
        UE_LOG(LogTemp, Display, TEXT("FPS_ACCEPT PASS SERVER READY realControllers=%d admitted=%d duration=300 protocol=%d netmode=%d"),
            Controllers, GS->PlayerArray.Num(), UFPSWeaponCatalog::ManifestVersion, int32(World->GetNetMode()));
    }
#endif
    return true;
}

void FFPSAcceptanceSmoke::Tick(AFPSFrontController* PC, float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSAcceptance"))) return;
    auto* World = PC->GetWorld();
    auto* GS = World->GetGameState<AFPSDemoGameState>();
    if (!GS || !GS->bStructuredMatch || GS->bWaiting || GS->RoundEndsAt <= 0) return;
    const bool Server = PC->HasAuthority(), Local = PC->IsLocalController();
    int32 Expected = 10; FParse::Value(FCommandLine::Get(), TEXT("FPSExpectedPlayers="), Expected);
    const double Age = GS->GetServerWorldTimeSeconds() - (GS->RoundEndsAt - 300);
    struct FPlayer
    {
        double JoinedAge = 0, FirstPawnAt = -1;
        bool Initialized = false, Late = false, Bad = false, Complete = false;
        int32 Step = -1;
        TSet<int32> Events;
        FGuid InitialPrimary, SelectedPrimary, Secondary, Life;
        FName Desired;
        FVector Inside = FVector::ZeroVector, Outside = FVector::ZeroVector;
        bool Initial = false, Selected = false, Exited = false, Returned = false, ExitedAgain = false;
        bool EarlyDropped = false, FinalDropped = false;
        uint32 MaxShots = 0, ReturnShots = 0;
    };
    struct FRun
    {
        int32 Report = -1, RelevancePhase = 0, Presets = -1;
        double Started = 0, LastSample = -1;
        bool Done = false, Bad = false, Blackout = false, Recovered = false;
        bool RelevanceReported = false, MissingFirst = false, RestoredFirst = false, MissingSecond = false, RestoredSecond = false;
        bool EarlyDrops = false, EarlyAlive = false, LifetimeClean = false, FinalDrops = false;
        TWeakObjectPtr<AFPSDemoCharacter> HiddenPawn;
        FVector Original;
        TSet<int32> Events;
        uint32 MaxShots = 0;
        int32 PeakPlayers = 0;
        double FrameSum = 0; uint64 Frames = 0; float MaxFrame = 0;
    };
    static TMap<TWeakObjectPtr<AFPSFrontController>, FPlayer> Players;
    static TMap<TWeakObjectPtr<UWorld>, FRun> Runs;
    auto& P = Players.FindOrAdd(PC); auto& Run = Runs.FindOrAdd(World);
    if (Run.Started == 0) Run.Started = FPlatformTime::Seconds();
    auto* Pawn = Cast<AFPSDemoCharacter>(PC->GetPawn());
    auto* PS = PC->GetPlayerState<AFPSDemoPlayerState>();
    auto* W = Pawn ? Pawn->GetWeapons() : nullptr;
    auto* Membership = Pawn ? Pawn->FindComponentByClass<UFPSBaseMembership>() : nullptr;
    auto Check = [&](bool Pass, const TCHAR* What)
    {
        P.Bad |= !Pass; Run.Bad |= !Pass;
        UE_LOG(LogTemp, Display, TEXT("FPS_ACCEPT %s %s CHECK player=%d age=%.2f %s"), Pass ? TEXT("PASS") : TEXT("FAIL"),
            Server ? TEXT("SERVER") : TEXT("CLIENT"), PS ? PS->GetPlayerId() : -1, Age, What);
    };
    auto At = [&](int32 Key, double When, auto Action)
    {
        if (Age >= When && !P.Events.Contains(Key)) { P.Events.Add(Key); Action(); }
    };
    if (Pawn && P.FirstPawnAt < 0) P.FirstPawnAt = Age;
    const bool Ready = Pawn && PS && PS->TeamId >= 0 && Membership && Membership->GetLifeId().IsValid() &&
        PC->IsWeaponContentVerified() && W && W->GetLoadout().Primary.IsValid() && W->GetLoadout().Secondary.IsValid() &&
        Membership->IsProtected() && Membership->CanSelect();
    if (!P.Initialized && P.FirstPawnAt >= 0 && Age > P.FirstPawnAt + 5 && !P.Events.Contains(900))
    {
        P.Events.Add(900); Check(false, TEXT("initial owner inventory team life protection and content replication readiness timeout"));
    }
    if (!P.Initialized && Ready)
    {
        P.Initialized = true; P.Late = Age > 20; P.JoinedAge = P.Late ? Age : 0;
        P.InitialPrimary = W->GetLoadout().Primary.InstanceId; P.Secondary = W->GetLoadout().Secondary.InstanceId;
        P.Life = Membership->GetLifeId();
        const FName Choices[] = {TEXT("Weapon.M4"), TEXT("Weapon.AK"), TEXT("Weapon.UMP45")};
        P.Desired = Choices[FMath::Abs(PS->GetPlayerId()) % 3];
        const float Lanes[] = {-1200, -1000, 800, 1000, 1200};
        const float Lane = Lanes[FMath::Abs(PS->GetPlayerId()) % 5];
        for (TActorIterator<AFPSBaseRegion> It(World); It; ++It)
            if (It->TeamId == PS->TeamId)
            {
                P.Inside = It->GetActorLocation(); P.Inside.Z = Pawn->GetActorLocation().Z; P.Inside.Y = Lane;
                P.Outside = P.Inside; P.Outside.X += PS->TeamId == 0 ? 750 : -750;
            }
        const auto* Rules = UFPSModeRules::ForWorld(World);
        const auto* PrimaryDef = Rules && Rules->DefaultLoadout ? Rules->DefaultLoadout->Primary.Get() : nullptr;
        const auto* SecondaryDef = Rules && Rules->DefaultLoadout ? Rules->DefaultLoadout->Secondary.Get() : nullptr;
        P.Initial = PrimaryDef && SecondaryDef && W->GetLoadout().Primary.WeaponId == PrimaryDef->WeaponId &&
            W->GetLoadout().Secondary.WeaponId == SecondaryDef->WeaponId && PS->SpawnPrimary() == PrimaryDef->WeaponId &&
            PS->SpawnSecondary() == SecondaryDef->WeaponId && Membership->IsProtected() && Membership->CanSelect() && PC->IsWeaponContentVerified();
        Check(P.Initial, P.Late ? TEXT("rejoined real player fresh default inventory and base permission") : TEXT("same mode defaults admitted real player protected spawn"));
        UE_LOG(LogTemp, Display, TEXT("FPS_ACCEPT IDENTITY role=%s player=%d team=%d late=%d joinedAge=%.2f primary=%s secondary=%s primaryGuid=%s life=%s"),
            Server ? TEXT("SERVER") : TEXT("CLIENT"), PS->GetPlayerId(), PS->TeamId, P.Late, P.JoinedAge,
            *W->GetLoadout().Primary.WeaponId.ToString(), *W->GetLoadout().Secondary.WeaponId.ToString(), *P.InitialPrimary.ToString(), *P.Life.ToString());
    }
    if (P.Initialized && Pawn && PS && W && Membership && !GS->bFinished)
    {
        const double Base = P.JoinedAge;
        At(1, Base + 3, [&]() { if (Local) { W->StartFire(); W->Drop(); } });
        At(2, Base + 6, [&]()
        {
            Check(Membership->IsProtected() && Membership->CanSelect() && W->GetLoadout().Primary.InstanceId == P.InitialPrimary &&
                W->GetLoadout().Primary.ShotSerial == 0 && W->GetLoadout().Secondary.InstanceId == P.Secondary,
                TEXT("spawn base rejects actual attack and drop without inventory change"));
            if (Local) { W->StopFire(); PC->RequestSelection(P.Desired); }
        });
        At(3, Base + 9, [&]()
        {
            P.SelectedPrimary = W->GetLoadout().Primary.InstanceId;
            P.Selected = P.SelectedPrimary.IsValid() && P.SelectedPrimary != P.InitialPrimary && W->GetLoadout().Primary.WeaponId == P.Desired &&
                W->GetLoadout().Secondary.InstanceId == P.Secondary && PS->SpawnPrimary() == P.Desired;
            Check(P.Selected, TEXT("individual selection committed immediately with fresh GUID and unchanged secondary"));
        });
        // Real local Pawn input traverses CharacterMovement RPCs. Route around the saved spawn shield.
        auto MoveTo = [&](const FVector& Destination)
        {
            FVector Direction = Destination - Pawn->GetActorLocation(); Direction.Z = 0;
            if (Direction.SizeSquared() > FMath::Square(35.f)) Pawn->AddMovementInput(Direction.GetSafeNormal());
        };
        if (Local && !Pawn->IsEliminated() && Pawn->GetCharacterMovement()->MovementMode != MOVE_None)
        {
            if (Age >= Base + 10 && Age < Base + 24)
            {
                if (FMath::Abs(Pawn->GetActorLocation().Y - P.Inside.Y) > 35) MoveTo(FVector(Pawn->GetActorLocation().X, P.Inside.Y, P.Inside.Z));
                else MoveTo(P.Outside);
            }
            if (!P.Late && Age >= 140 && Age < 169) MoveTo(P.Inside);
            if (!P.Late && Age >= 180 && Age < 199) MoveTo(P.Outside);
            PC->SetControlRotation(FRotator(60, PS->TeamId == 0 ? 0 : 180, 0));
        }
        At(4, Base + 25, [&]()
        {
            P.Exited = !Membership->IsProtected() && !Membership->CanSelect();
            Check(P.Exited, TEXT("real local movement left own base and permanently closed selection for this life"));
            if (Local) { W->StopFire(); PC->ServerSelectWeapon(Pawn, Membership->GetLifeId(), TEXT("Weapon.M4"), 1000); }
        });
        At(5, Base + 28, [&]()
        {
            Check(W->GetLoadout().Primary.InstanceId == P.SelectedPrimary && PS->SpawnPrimary() == P.Desired && !Membership->CanSelect(),
                TEXT("out of base stale selection RPC rejected without inventory or preference mutation"));
        });
        if (!P.Late)
        {
            At(6, 49, [&]() { if (Local) { W->StopFire(); W->Primary(); } });
            At(7, 52, [&]() { if (Local) W->Drop(); });
            At(8, 58, [&]()
            {
                P.EarlyDropped = !W->GetLoadout().Primary.IsValid() && W->GetLoadout().Secondary.InstanceId == P.Secondary && PS->SpawnPrimary() == P.Desired;
                Check(P.EarlyDropped, TEXT("outside real GAS drop removes primary while preserving saved model"));
                if (Local) W->Secondary();
            });
            At(9, 137, [&]() { if (Local) W->StopFire(); });
            At(10, 170, [&]()
            {
                P.Returned = Membership->IsProtected() && !Membership->CanSelect();
                P.ReturnShots = W->GetLoadout().Secondary.ShotSerial;
                Check(P.Returned, TEXT("real movement returned to own base restoring protection without selection"));
                if (Local) { W->StartFire(); W->Drop(); PC->ServerSelectWeapon(Pawn, Membership->GetLifeId(), TEXT("Weapon.M4"), 1001); }
            });
            At(11, 175, [&]()
            {
                Check(Membership->IsProtected() && !Membership->CanSelect() && !W->GetLoadout().Primary.IsValid() &&
                    W->GetLoadout().Secondary.InstanceId == P.Secondary && PS->SpawnPrimary() == P.Desired &&
                    W->GetLoadout().Secondary.ShotSerial == P.ReturnShots && !W->GetLoadout().bFireActive,
                    TEXT("returned base rejects attack drop and selection RPC; dropped inventory stays empty"));
                if (Local) W->StopFire();
            });
            At(12, 200, [&]()
            {
                P.ExitedAgain = !Membership->IsProtected() && !Membership->CanSelect();
                Check(P.ExitedAgain, TEXT("second real exit removes protection without restoring selection"));
            });
        }
        At(13, 272, [&]() { if (Local) { W->StopFire(); W->Secondary(); } });
        At(14, 275, [&]() { if (Local) W->Drop(); });
        At(15, 282, [&]()
        {
            P.FinalDropped = !W->GetLoadout().Secondary.IsValid() && PS->SpawnPrimary() == P.Desired;
            Check(P.FinalDropped, TEXT("late round real GAS drop supplies live dynamic actor for round cleanup"));
        });
        const bool Actions = Age > Base + 28 && Age < 272 && !(Age >= 48 && Age < 60) && !(Age >= 137 && Age < 201);
        if (Local && Actions && !Membership->IsProtected() && !Pawn->IsEliminated())
        {
            const int32 Step = int32(Age * 2);
            if (P.Step != Step)
            {
                P.Step = Step; W->StopFire();
                if (Step % 20 == 0) W->Secondary();
                else if (Step % 20 == 2 && W->GetLoadout().Primary.IsValid()) W->Primary();
                else if (Step % 20 == 4) W->Inspect();
                else if (Step % 20 == 6) W->AimOn();
                else if (Step % 20 == 8) W->AimOff();
                else if (W->Equipped().Magazine == 0) W->Reload();
                else W->StartFire();
            }
        }
        const auto& L = W->GetLoadout();
        P.MaxShots = FMath::Max(P.MaxShots, L.Primary.ShotSerial + L.Secondary.ShotSerial);
    }
    if (!Server && Local && FParse::Param(FCommandLine::Get(), TEXT("FPSAcceptanceBlackout")))
    {
        if (Age >= 60 && !Run.Blackout)
        {
            Run.Blackout = true;
#if DO_ENABLE_NET_TEST
            if (auto* Driver = World->GetNetDriver()) { auto Settings = Driver->PacketSimulationSettings; Settings.PktLoss = 100; Driver->SetPacketSimulationSettings(Settings); Run.Bad |= Driver->PacketSimulationSettings.PktLoss != 100; }
#else
            Run.Bad = true;
#endif
            UE_LOG(LogTemp, Display, TEXT("FPS_ACCEPT BLACKOUT begin age=%.1f"), Age);
        }
        if (Age >= 80 && !Run.Recovered)
        {
            Run.Recovered = true;
#if DO_ENABLE_NET_TEST
            int32 Loss = 3; FParse::Value(FCommandLine::Get(), TEXT("PktLoss="), Loss);
            if (auto* Driver = World->GetNetDriver()) { auto Settings = Driver->PacketSimulationSettings; Settings.PktLoss = Loss; Driver->SetPacketSimulationSettings(Settings); Run.Bad |= Driver->PacketSimulationSettings.PktLoss != Loss; }
#endif
            PC->ServerAcceptanceProbe(); UE_LOG(LogTemp, Display, TEXT("FPS_ACCEPT BLACKOUT end age=%.1f"), Age);
        }
    }
    if (!Server && Local && GS->bFinished && !P.Complete)
    {
        P.Complete = true;
        const bool Pass = !P.Bad && !Run.Bad && P.Initial && P.Selected && P.Exited && P.FinalDropped &&
            (P.Late || (P.EarlyDropped && P.Returned && P.ExitedAgain)) && P.MaxShots > 0 && Age >= 300 &&
            GS->PlayerArray.Num() == Expected && GS->Podium.Num() == FMath::Min(3, Expected) && (!W || !W->CanAct());
        UE_LOG(LogTemp, Display, TEXT("FPS_ACCEPT %s CLIENT COMPLETE age=%.2f observedWall=%.2f late=%d players=%d podium=%d shots=%u netmode=%d"),
            Pass ? TEXT("PASS") : TEXT("FAIL"), Age, FPlatformTime::Seconds() - Run.Started, P.Late, GS->PlayerArray.Num(), GS->Podium.Num(), P.MaxShots, int32(World->GetNetMode()));
    }
    // World observations run once per frame, independently of ten controller ticks.
    const double Now = World->GetTimeSeconds(); if (Run.LastSample == Now) return; Run.LastSample = Now;
    Run.FrameSum += DeltaSeconds; ++Run.Frames; Run.MaxFrame = FMath::Max(Run.MaxFrame, DeltaSeconds);
    Run.PeakPlayers = FMath::Max(Run.PeakPlayers, GS->PlayerArray.Num());
    int32 Presets = 0, Dynamic = 0; uint32 Shots = 0;
    for (TActorIterator<AFPSWeaponPickup> It(World); It; ++It) { if (It->IsDynamic()) ++Dynamic; else ++Presets; }
    if (Server && Run.Presets < 0) Run.Presets = Presets;
    for (TActorIterator<AFPSDemoCharacter> It(World); It; ++It)
        if (Server || It->IsLocallyControlled())
        {
            auto* Weapons = It->GetWeapons();
            for (const auto* Gun : {&Weapons->GetLoadout().Primary, &Weapons->GetLoadout().Secondary})
            {
                Shots += Gun->ShotSerial;
                if (const auto* Def = Weapons->Catalog()->Find(Gun->WeaponId)) Run.Bad |= Gun->Magazine < 0 || Gun->Magazine > Def->MagazineSize || Gun->Reserve < 0;
            }
            Run.Bad |= !FMath::IsFinite(It->GetHealth());
        }
    Run.MaxShots = FMath::Max(Run.MaxShots, Shots);
    auto WorldAt = [&](int32 Key, double When, auto Action) { if (Age >= When && !Run.Events.Contains(Key)) { Run.Events.Add(Key); Action(); } };
    if (Server)
    {
        auto Drops = [&](bool Condition, const TCHAR* What)
        {
            Run.Bad |= !Condition;
            UE_LOG(LogTemp, Display, TEXT("FPS_ACCEPT %s SERVER DROPS age=%.2f dynamic=%d registry=%d presets=%d %s"),
                Condition ? TEXT("PASS") : TEXT("FAIL"), Age, Dynamic, World->GetSubsystem<UFPSDynamicDrops>()->Count(), Presets, What);
        };
        WorldAt(1, 60, [&]() { Run.EarlyDrops = Dynamic == Expected && Presets == Run.Presets; Drops(Run.EarlyDrops, TEXT("all ten real primary drops observed; selection recycled old guns")); });
        WorldAt(2, 220, [&]() { Run.EarlyAlive = Dynamic == Expected; Drops(Run.EarlyAlive, TEXT("early drops still alive before natural 180 second deadline")); });
        WorldAt(3, 242, [&]() { Run.LifetimeClean = Dynamic == 0 && Presets == Run.Presets; Drops(Run.LifetimeClean, TEXT("natural 180 second lifetime cleanup without time scaling")); });
        WorldAt(4, 285, [&]() { Run.FinalDrops = Dynamic == Expected && Presets == Run.Presets; Drops(Run.FinalDrops, TEXT("late drops present for actual round end cleanup")); });
    }
    int32 FirstPlayerId = MAX_int32;
    for (const APlayerState* Entry : GS->PlayerArray) FirstPlayerId = FMath::Min(FirstPlayerId, Entry->GetPlayerId());
    AFPSDemoCharacter* FirstPawn = nullptr;
    for (TActorIterator<AFPSDemoCharacter> It(World); It; ++It)
        if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == FirstPlayerId) { FirstPawn = *It; break; }
    if (Server && GS->PlayerArray.Num() == Expected)
    {
        if ((Run.RelevancePhase == 0 && Age >= 30) || (Run.RelevancePhase == 2 && Age >= 90))
            if (FirstPawn)
            {
                Run.HiddenPawn = FirstPawn; Run.Original = FirstPawn->GetActorLocation(); FirstPawn->GetWeapons()->StopAllActions();
                FirstPawn->GetCharacterMovement()->DisableMovement(); FirstPawn->SetActorLocation(FVector(200000,0,100)); FirstPawn->ForceNetUpdate(); ++Run.RelevancePhase;
                UE_LOG(LogTemp, Display, TEXT("FPS_ACCEPT RELEVANCE hide phase=%d player=%d explicit relevance position fixture"), Run.RelevancePhase, FirstPlayerId);
            }
        if ((Run.RelevancePhase == 1 && Age >= 45) || (Run.RelevancePhase == 3 && Age >= 105))
            if (auto* Hidden = Run.HiddenPawn.Get())
            {
                Hidden->SetActorLocation(Run.Original); Hidden->GetCharacterMovement()->SetMovementMode(MOVE_Walking); Hidden->ForceNetUpdate(); ++Run.RelevancePhase;
                UE_LOG(LogTemp, Display, TEXT("FPS_ACCEPT RELEVANCE return phase=%d"), Run.RelevancePhase);
            }
    }
    else if (!Server && Local && PS && PS->GetPlayerId() != FirstPlayerId && !P.Late)
    {
        if (Age >= 38 && Age < 45) Run.MissingFirst |= FirstPawn == nullptr;
        if (Age >= 50 && Age < 60) Run.RestoredFirst |= FirstPawn && FirstPawn->GetWeapons()->Equipped().IsValid();
        if (Age >= 98 && Age < 105) Run.MissingSecond |= FirstPawn == nullptr;
        if (Age >= 110 && Age < 120) Run.RestoredSecond |= FirstPawn && FirstPawn->GetWeapons()->Equipped().IsValid();
        if (Age >= 120 && !Run.RelevanceReported)
        {
            Run.RelevanceReported = true;
            UE_LOG(LogTemp, Display, TEXT("FPS_ACCEPT RELEVANCE observed missing1=%d restored1=%d missing2=%d restored2=%d"), Run.MissingFirst, Run.RestoredFirst, Run.MissingSecond, Run.RestoredSecond);
        }
    }
    const int32 Report = int32(Age / 10);
    if (Report != Run.Report)
    {
        Run.Report = Report; const auto Memory = FPlatformMemory::GetStats(); auto* Net = World->GetNetDriver();
        UE_LOG(LogTemp, Display, TEXT("FPS_ACCEPT SAMPLE role=%s netmode=%d age=%.1f players=%d shots=%u dynamic=%d presets=%d rssMB=%.1f frameMeanMS=%.2f frameMaxMS=%.2f inBps=%u outBps=%u"),
            Server ? TEXT("SERVER") : TEXT("CLIENT"), int32(World->GetNetMode()), Age, GS->PlayerArray.Num(), Shots, Dynamic, Presets,
            double(Memory.UsedPhysical) / 1048576, Run.FrameSum / Run.Frames * 1000, Run.MaxFrame * 1000, Net ? Net->InBytesPerSecond : 0, Net ? Net->OutBytesPerSecond : 0);
    }
    if (Server && GS->bFinished && !Run.Done)
    {
        Run.Done = true; int32 Valid = 0;
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
            if (auto* Entry = Cast<AFPSFrontController>(It->Get()))
            {
                const auto* State = Players.Find(Entry);
                if (State && !State->Bad && State->Initial && State->Selected && State->Exited && State->FinalDropped &&
                    (State->Late || (State->EarlyDropped && State->Returned && State->ExitedAgain)) && State->MaxShots > 0) ++Valid;
            }
        const bool Pass = !Run.Bad && Age >= 300 && Run.PeakPlayers == Expected && GS->PlayerArray.Num() == Expected && Valid == Expected &&
            Run.MaxShots > 0 && GS->Podium.Num() == FMath::Min(3, Expected) && Run.EarlyDrops && Run.EarlyAlive && Run.LifetimeClean &&
            Run.FinalDrops && Dynamic == 0 && Presets == Run.Presets;
        UE_LOG(LogTemp, Display, TEXT("FPS_ACCEPT %s SERVER COMPLETE age=%.2f observedWall=%.2f peakPlayers=%d players=%d validPlayers=%d podium=%d shots=%u dynamic=%d presets=%d netmode=%d"),
            Pass ? TEXT("PASS") : TEXT("FAIL"), Age, FPlatformTime::Seconds() - Run.Started, Run.PeakPlayers, GS->PlayerArray.Num(), Valid,
            GS->Podium.Num(), Run.MaxShots, Dynamic, Presets, int32(World->GetNetMode()));
    }
#endif
}

void AFPSFrontController::ServerAcceptanceProbe_Implementation()
{
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("FPSAcceptance"))) ClientAcceptanceProbe();
#endif
}
void AFPSFrontController::ClientAcceptanceProbe_Implementation()
{
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("FPSAcceptance"))) UE_LOG(LogTemp, Display, TEXT("FPS_ACCEPT PASS BLACKOUT reliable round trip recovered"));
#endif
}
