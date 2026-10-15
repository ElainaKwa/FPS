#include "FPSMovementSmoke.h"
#include "FPSMovementComponent.h"
#include "FPSMovementSettings.h"
#include "FPSInputSettings.h"
#include "FPSCharacterInput.h"
#include "FPSInterface.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoAttributeSet.h"
#include "FPSDemoEffects.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "FPSMatchFlow.h"
#include "FPSLifeComponent.h"
#include "FPSActionPolicy.h"
#include "FPSBaseMembership.h"
#include "FPSBaseRegion.h"
#include "FPSWeaponComponent.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/InputSettings.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/FileManager.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UnrealClient.h"

AFPSMovementSmokeBox::AFPSMovementSmokeBox()
{
    bReplicates = true; bAlwaysRelevant = true; SetReplicateMovement(true);
    Box = CreateDefaultSubobject<UBoxComponent>(TEXT("MovementFixtureCollision"));
    SetRootComponent(Box); Box->SetBoxExtent(BoxExtent);
    Box->SetCollisionObjectType(ECC_WorldStatic);
    Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Box->SetCollisionResponseToAllChannels(ECR_Block);
}
void AFPSMovementSmokeBox::Configure(const FVector& Extent) { BoxExtent = Extent; OnRep_Extent(); ForceNetUpdate(); }
void AFPSMovementSmokeBox::OnRep_Extent() { Box->SetBoxExtent(BoxExtent); }
void AFPSMovementSmokeBox::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AFPSMovementSmokeBox, BoxExtent);
}
AFPSMovementSmokeState::AFPSMovementSmokeState() { bReplicates = true; bAlwaysRelevant = true; SetNetUpdateFrequency(20); }
void AFPSMovementSmokeState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AFPSMovementSmokeState, Scene); DOREPLIFETIME(AFPSMovementSmokeState, StartsAt);
    DOREPLIFETIME(AFPSMovementSmokeState, ReferenceScene); DOREPLIFETIME(AFPSMovementSmokeState, ReferenceAt);
    DOREPLIFETIME(AFPSMovementSmokeState, References);
}

#if !UE_BUILD_SHIPPING
namespace
{
constexpr float GroundZ = 2400;
enum EScene : int32
{
    Speeds, SprintCost, Wall, RetainedTwelve, Directions, Exhaustion, NormalJump, WeakJump,
    HeldJump, CrouchNormal, CrouchLow, LowRoof, AirMomentum, FailedRecovery, InputModes,
    WeaponPriority, AutomaticPreparation, SemiPreparation, BurstPreparation, WeaponDeadlines,
    Lifecycle, BaseAndMenu, EmptyInventory, TrainingAI, GroundResponse, AirSteering, RoundEnd, SceneCount
};
const TCHAR* SceneName(int32 Scene)
{
    static const TCHAR* Names[] = { TEXT("speeds"), TEXT("sprint-cost-replay"), TEXT("wall"), TEXT("retained-twelve"),
        TEXT("directions"), TEXT("exhaustion"), TEXT("normal-jump"), TEXT("weak-jump"), TEXT("held-jump"),
        TEXT("crouch-normal"), TEXT("crouch-low"), TEXT("low-roof"), TEXT("air-momentum"), TEXT("failed-recovery"),
        TEXT("input-modes"), TEXT("weapon-priority"), TEXT("automatic-preparation"), TEXT("semi-preparation"),
        TEXT("burst-preparation"), TEXT("weapon-deadlines"), TEXT("lifecycle"), TEXT("base-menu"),
        TEXT("empty-inventory"), TEXT("training-ai"), TEXT("ground-response"), TEXT("air-steering"), TEXT("round-end") };
    return Scene >= 0 && Scene < SceneCount ? Names[Scene] : TEXT("invalid");
}
double Duration(int32 Scene)
{
    static const double Values[] = { 7,5,4,5,5,8,4,4,5,4,5,5,5,4,6,6,6,6,4,6,8,5,4,4,9,8,3 };
    return Scene >= 0 && Scene < SceneCount ? Values[Scene] : 0;
}
float Seed(int32 Scene)
{
    switch (Scene)
    {
    case RetainedTwelve: return 22;
    case Exhaustion: return 21;
    case WeakJump: return 10;
    case CrouchLow: return 5;
    case LowRoof: return 20;
    case FailedRecovery: return 0;
    case BaseAndMenu: return 40;
    default: return 100;
    }
}
void Key(APlayerController* PC, const FKey& Value, EInputEvent Event)
{
    PC->InputKey(FInputKeyEventArgs(nullptr, FInputDeviceId::CreateFromInternalId(0), Value, Event,
        Event == IE_Pressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}
void ReleaseKeys(APlayerController* PC)
{
    for (const FKey& Value : { EKeys::LeftShift, EKeys::LeftControl, EKeys::SpaceBar, EKeys::LeftMouseButton,
        EKeys::RightMouseButton, EKeys::R, EKeys::Q, EKeys::One, EKeys::Two }) Key(PC, Value, IE_Released);
}
struct FGroundResponseMeasure
{
    FString Name;
    FVector Position = FVector::ZeroVector, Direction = FVector::ForwardVector;
    double Started = -1, Elapsed = 0;
    float SecondsLimit = 0, MinimumSeconds = 0, DistanceLimit = 0, MinSpeed = 0, MaxSpeed = 0, AngleLimit = 5;
    uint32 Corrections = 0;
    bool Reported = false;
};
struct FAirLandingSample
{
    double At = 0;
    float Speed = 0;
    bool Falling = false;
};
struct FSceneProgress
{
    int32 Index = -1;
    TSet<int32> Done;
    TArray<FKey> DeferredReleases;
    bool Initialized = false, WasAirborne = false, WasFalling = false, ObservedAirBackward = false;
    bool SawPending = false, ObservedPreparationShot = false, PreparationRespected = true;
    bool HasRecoverySample = false;
    float BaseZ = 0, Apex = 0, InitialStamina = 0, StaminaSample = 0, WallSprintSeconds = 0;
    double SprintSeconds = 0, FirstLanding = 0, FireReady = 0, ShotAt = 0, SpeedCrossedAt = 0;
    uint32 Jumps = 0, Crouches = 0, Replays = 0, Corrections = 0, ShotSerial = 0;
    uint32 FrameSamples = 0; int32 MaxUnacked = 0; double SampleSeconds = 0;
    int32 Magazine = 0;
    FGuid Gun;
    TWeakObjectPtr<AFPSDemoCharacter> InitialBody;
    TArray<FGroundResponseMeasure> GroundMeasures;
    float TurnStartedYaw = 0;
    int32 AirSteeringPhase = 0;
    double AirTurnAt = -1, AirCoastAt = -1, AirReverseAt = -1, AirLandedAt = -1;
    double SecondTakeoffAt = -1;
    FVector AirTurnVelocity = FVector::ZeroVector, AirCoastVelocity = FVector::ZeroVector;
    FVector SecondTakeoffPosition = FVector::ZeroVector;
    float AirHighestSpeed = 0;
    float SecondJumpBaseZ = 0, SecondJumpApex = 0;
    uint32 SecondTakeoffCorrections = 0;
    bool SawSecondFlight = false, SecondWasFalling = false, LandingCurveReported = false;
    TArray<FAirLandingSample> LandingCurve;
};
struct FProgress
{
    FSceneProgress Scene;
    bool Failed = false, Completed = false;
    bool OriginalSprintToggle = false, OriginalCrouchToggle = false, SettingsSaved = false;
    float NormalApex = 0, WeakApex = 0, PreviousForceAdjustment = 0;
    float PreviousClientFPS = 60;
    TMap<TWeakObjectPtr<APlayerController>, FTransform> SpawnTransforms;
    TWeakObjectPtr<AFPSDemoCharacter> AI;
    uint32 AIJumps = 0, AICrouches = 0;
    double AIMoveSeconds = 0, AIJumpRequestedAt = -1;
};
AFPSMovementSmokeState* Fixture(UWorld* World)
{
    for (TActorIterator<AFPSMovementSmokeState> It(World); It; ++It) return *It;
    return nullptr;
}
float AuthorityRecoveryEstimate(const FFPSMovementSmokeReference& Reference, double Elapsed)
{
    const float RecoveringSeconds = FMath::Max(0.f,float(Elapsed)-Reference.RecoveryRemaining);
    return FMath::Min(Reference.MaxStamina, Reference.Stamina + RecoveringSeconds * 25.f);
}
}
#endif

void FFPSMovementSmoke::Start(AFPSMatchMode* Mode)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSMovementSmoke")) || FParse::Param(FCommandLine::Get(), TEXT("FPSMovementTeaching")) || FParse::Param(FCommandLine::Get(), TEXT("FPSMovementInputUI"))) return;
    FTimerHandle Handle;
    Mode->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Mode, [Mode]()
    {
        auto* World = Mode->GetWorld(); auto* GS = World->GetGameState<AFPSDemoGameState>();
        if (!GS || !GS->WeaponDataError.IsEmpty() || Fixture(World)) return;
        int32 Players = 0;
        for (auto It = World->GetPlayerControllerIterator(); It; ++It)
        {
            auto* PC = Cast<AFPSFrontController>(It->Get());
            if (!PC || !PC->IsWeaponContentVerified()) return;
            ++Players;
        }
        if (Players != (GS->bTraining ? 1 : 2)) return;
        if (GS->bWaiting) Mode->StartRound(World->GetFirstPlayerController());
        if (GS->bWaiting) return;
        for (auto It = World->GetPlayerControllerIterator(); It; ++It) if (!It->Get()->GetPawn()) return;
        static TMap<TWeakObjectPtr<UWorld>, double> Ready;
        double& ReadyAt = Ready.FindOrAdd(World);
        if (ReadyAt == 0) ReadyAt = GS->GetServerWorldTimeSeconds();
        if (GS->GetServerWorldTimeSeconds() - ReadyAt < 10) return;
        auto* State = World->SpawnActor<AFPSMovementSmokeState>();
        State->Floor = World->SpawnActor<AFPSMovementSmokeBox>(FVector(0,0,GroundZ-50), FRotator::ZeroRotator);
        State->Floor->Configure(FVector(6500,1500,50));
        State->Wall = World->SpawnActor<AFPSMovementSmokeBox>(FVector(0,0,GroundZ-1000), FRotator::ZeroRotator);
        State->Wall->Configure(FVector(20,1000,300));
        State->Roof = World->SpawnActor<AFPSMovementSmokeBox>(FVector(0,0,GroundZ-1000), FRotator::ZeroRotator);
        State->Roof->Configure(FVector(600,1000,10));
        State->Scene = Speeds; State->StartsAt = GS->GetServerWorldTimeSeconds() + 2;
        State->ForceNetUpdate();
        UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE INJECTION SERVER isolated replicated collision floor/wall/roof; native movement and GAS actions; map unchanged"));
    }), .2f, true);
#endif
}

void FFPSMovementSmoke::Tick(AFPSFrontController* PC, float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("FPSMovementInputUI"))) { TickInputUI(PC); return; }
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSMovementSmoke")) || !PC->IsLocalController() || FParse::Param(FCommandLine::Get(), TEXT("FPSMovementTeaching"))) return;
    auto* World = PC->GetWorld(); auto* GS = World->GetGameState<AFPSDemoGameState>();
    auto* State = Fixture(World);
    if (!GS || !State || State->Scene < 0 || !PC->IsWeaponContentVerified()) return;
    static TMap<TWeakObjectPtr<UWorld>, FProgress> Progress;
    auto& P = Progress.FindOrAdd(World);
    if (P.Completed) return;
    const bool Server = PC->HasAuthority(), Training = GS->bTraining;
    const TCHAR* Role = Server ? TEXT("SERVER") : TEXT("CLIENT");
    auto Check = [&](bool Pass, const TCHAR* Name)
    {
        P.Failed |= !Pass;
        UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE %s %s %s scene=%d(%s)"), Pass ? TEXT("PASS") : TEXT("FAIL"), Role, Name, State->Scene, SceneName(State->Scene));
    };
    auto At = [&](double Seconds, int32 Id, TFunction<void()> Action)
    {
        if (GS->GetServerWorldTimeSeconds() - State->StartsAt >= Seconds && !P.Scene.Done.Contains(Id)) { P.Scene.Done.Add(Id); Action(); }
    };
    auto CaptureHUD = [&](const TCHAR* Name)
    {
        if (FParse::Param(FCommandLine::Get(),TEXT("FPSMovementVisual"))) FScreenshotRequest::RequestScreenshot(FPaths::ScreenShotDir()/Name,true,false);
    };
    auto* Body = Cast<AFPSDemoCharacter>(PC->GetPawn());
    auto* Move = Body ? Cast<UFPSMovementComponent>(Body->GetCharacterMovement()) : nullptr;
    auto* W = Body ? Body->GetWeapons() : nullptr;
    if (P.Scene.Index != State->Scene)
    {
        ReleaseKeys(PC);
        if (!P.SettingsSaved)
        {
            auto* Settings = UFPSInputSettings::Get();
            P.OriginalSprintToggle = Settings->bSprintToggle; P.OriginalCrouchToggle = Settings->bCrouchToggle; P.SettingsSaved = true;
        }
        UFPSInputSettings::Get()->bSprintToggle = false; UFPSInputSettings::Get()->bCrouchToggle = false;
        P.Scene = FSceneProgress(); P.Scene.Index = State->Scene;
        if (Server)
        {
            State->Wall->SetActorLocation(FVector(0,0,GroundZ-1000));
            State->Roof->SetActorLocation(FVector(0,0,GroundZ-1000));
            if (State->Scene == Wall) State->Wall->SetActorLocation(FVector(-2780,0,GroundZ+200));
            if (State->Scene == LowRoof) State->Roof->SetActorLocation(FVector(-2300,0,GroundZ+120));
            int32 Lane = 0;
            for (auto It = World->GetPlayerControllerIterator(); It; ++It)
            {
                auto* Controller = It->Get(); auto* Target = Cast<AFPSDemoCharacter>(Controller->GetPawn());
                if (!Target) continue;
                auto* Movement = CastChecked<UFPSMovementComponent>(Target->GetCharacterMovement());
                if (!P.SpawnTransforms.Contains(Controller)) P.SpawnTransforms.Add(Controller,Target->GetActorTransform());
                Target->StopCombatInput(); Movement->ClearMovementIntents(); Movement->UnCrouch(); Movement->StopMovementImmediately();
                Movement->SetMovementMode(MOVE_Walking);
                if (State->Scene == BaseAndMenu) Target->SetActorTransform(P.SpawnTransforms[Controller]);
                else Target->SetActorLocation(FVector(-3000,Lane++ ? 450 : -450,GroundZ+Target->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()+2));
                Controller->SetControlRotation(FRotator::ZeroRotator); Target->SetActorRotation(FRotator::ZeroRotator);
                if (auto* Membership = Target->FindComponentByClass<UFPSBaseMembership>()) Membership->RefreshAuthority();
                Movement->SetStaminaForTest(Seed(State->Scene));
                // Scene setup is explicit resource injection; preserve the seed until the shared start,
                // then use the ordinary recovery delay. Actual actions reset it through production Spend.
                Movement->Resource.RecoveryRemaining = float(FMath::Max(0.,State->StartsAt-GS->GetServerWorldTimeSeconds())) + 1.5f;
                Movement->ForceClientAdjustment();
                auto* Weapons = Target->GetWeapons(); Weapons->StopAllActions();
                FName Weapon = State->Scene == SemiPreparation ? TEXT("Weapon.HeavyPistol") : State->Scene == BurstPreparation ? TEXT("Weapon.BR3") : TEXT("Weapon.M4");
                if ((State->Scene >= WeaponPriority && State->Scene <= Lifecycle) || State->Scene == RoundEnd)
                {
                    // Inventory is explicit fixture injection after the real admitted content session.
                    // Never call selection or restore a life permission after leaving the base.
                    auto Primary=Weapons->Catalog()->CreateInstance(Weapon==TEXT("Weapon.BR3")?Weapon:FName(TEXT("Weapon.M4")));
                    auto Secondary=Weapons->Catalog()->CreateInstance(Weapon==TEXT("Weapon.HeavyPistol")?Weapon:FName(TEXT("Weapon.M9")));
                    Check(Primary.IsValid() && Secondary.IsValid(),TEXT("admitted catalog creates inventory fixture instances"));
                    Weapons->Loadout.Primary=Primary; Weapons->Loadout.Secondary=Secondary;
                    ++Weapons->Loadout.PrimaryInventoryRevision; ++Weapons->Loadout.SecondaryInventoryRevision;
                    Weapons->Loadout.ActiveSlot=Weapon==TEXT("Weapon.HeavyPistol")?EFPSWeaponSlot::Secondary:EFPSWeaponSlot::Primary;
                    Weapons->Loadout.Action = EFPSWeaponAction::Idle;
                    Weapons->Loadout.ActionEndsAt = 0; Weapons->Loadout.EquipStartedAt = 0;
                    Weapons->EquippedMutable().Magazine = State->Scene == WeaponPriority ? 10 : Weapons->Definition() ? Weapons->Definition()->MagazineSize : 0;
                    Weapons->EquippedMutable().Reserve = 120;
                    Weapons->EquippedMutable().NextShotAt = State->Scene == WeaponDeadlines ? State->StartsAt+3 : 0;
                    Weapons->Changed();
                    UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE INJECTION SERVER inventory player=%d primary=%s secondary=%s active=%s; subsequent actions use real GAS"),
                        Controller->GetPlayerState<APlayerState>()?Controller->GetPlayerState<APlayerState>()->GetPlayerId():-1,*Primary.WeaponId.ToString(),*Secondary.WeaponId.ToString(),*Weapon.ToString());
                }
                if (State->Scene == EmptyInventory) { Weapons->Loadout = {}; Weapons->Changed(); }
                Target->ForceNetUpdate();
            }
            UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE SCENE SERVER index=%d name=%s starts=%.3f seed=%.1f"),State->Scene,SceneName(State->Scene),State->StartsAt,Seed(State->Scene));
        }
        if (Move) { Move->ClearMovementIntents(); PC->SetControlRotation(FRotator::ZeroRotator); }
    }
    const double Time = GS->GetServerWorldTimeSeconds() - State->StartsAt;
    if (Time < 0) return;
    for (const auto& Value : P.Scene.DeferredReleases) Key(PC,Value,IE_Released);
    P.Scene.DeferredReleases.Reset();
    if (Body && Move && !P.Scene.Initialized)
    {
        auto& S = P.Scene; S.Initialized = true; S.InitialBody = Body;
        S.InitialStamina = Move->GetStamina(); S.BaseZ = Body->GetActorLocation().Z; S.Jumps = Move->GetJumpCharges(); S.Crouches = Move->GetCrouchCharges();
        S.Replays = Move->DebugReplayCount; S.Corrections = Move->DebugCorrectionCount; S.SprintSeconds = Move->DebugSprintConsumedSeconds;
        S.ShotSerial = W->Equipped().ShotSerial; S.Magazine = W->Equipped().Magazine; S.Gun = W->Equipped().InstanceId;
        Check(Move->UsesMovementRules() && Move->UsesStamina(),TEXT("player uses movement and GAS stamina rules"));
        Check(FMath::IsNearlyEqual(Move->GetStamina(),Seed(State->Scene),1.f) && FMath::IsNearlyEqual(Move->GetMaxStamina(),100.f),TEXT("authority scene resource reset reached owner"));
        if (State->Scene == Speeds) UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE READY role=%s netmode=%d rules=1 stamina=1"),Role,int32(World->GetNetMode()));
    }
    auto& S = P.Scene;
    if (!Body || !Move || !W)
    {
        if (State->Scene != Lifecycle) { At(.5,900,[&]() { Check(false,TEXT("live owned pawn available for scene")); }); return; }
    }
    auto Forward = [&](float Value) { if (Body) Body->MoveForward(FInputActionValue(Value)); };
    auto Right = [&](float Value) { if (Body) Body->MoveRight(FInputActionValue(Value)); };
    auto Press = [&](const FKey& Value) { Key(PC,Value,IE_Pressed); };
    auto Release = [&](const FKey& Value) { Key(PC,Value,IE_Released); };
    // Keep the key down through one real Enhanced Input frame; an immediate release
    // in the same frame can erase Started and would not exercise the mapping.
    auto Tap = [&](const FKey& Value) { Press(Value); S.DeferredReleases.Add(Value); };
    auto Sprint = [&]() { Press(EKeys::LeftShift); };
    auto Jump = [&]() { Press(EKeys::SpaceBar); };
    auto Fire = [&]() { W->StartFire(); };
    if (Body && Move)
    {
        if(!Server && State->Scene==SprintCost)
        {
            ++S.FrameSamples; S.SampleSeconds+=DeltaSeconds;
            S.MaxUnacked=FMath::Max(S.MaxUnacked,Move->GetPredictionData_Client_Character()->SavedMoves.Num());
        }
        if (Move->IsFalling())
        {
            S.WasAirborne = true; S.Apex = FMath::Max(S.Apex,Body->GetActorLocation().Z-S.BaseZ);
        }
        if (S.WasFalling && !Move->IsFalling() && S.FirstLanding == 0) S.FirstLanding = Time;
        S.WasFalling = Move->IsFalling();
        if (W->HasPendingSprintFire()) { S.SawPending = true; S.FireReady = FMath::Max(S.FireReady,W->GetSprintFireReadyAt()); }
        if (W->Equipped().InstanceId == S.Gun && W->Equipped().ShotSerial > S.ShotSerial && S.ShotAt == 0)
        {
            S.ShotAt = W->Equipped().LastShotAt; S.ObservedPreparationShot = true;
            S.PreparationRespected = S.FireReady > 0 && S.ShotAt+.017 >= S.FireReady;
            UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s fire_first=%.4f observation=%.4f sprint_ready=%.4f serial=%u predicted=%d"),Role,S.ShotAt,GS->GetServerWorldTimeSeconds(),S.FireReady,W->Equipped().ShotSerial,W->PredictedShots.Contains(S.Gun));
        }
    }
    switch (State->Scene)
    {
    case Speeds:
        At(.05,20,[&]() { const auto View=UFPSInterface::ReadStaminaHUD(Body); Check(!View.Visible && FMath::IsNearlyEqual(View.Ratio,1.f),TEXT("full unrestricted player hides shared HUD stamina view")); CaptureHUD(TEXT("MovementFull.png")); });
        if (Time < 3.6 || (Time >= 4 && Time < 5.1)) Forward(1);
        At(.7,1,[&]() { Check(FMath::IsNearlyEqual(Move->Velocity.Size2D(),480.f,30.f),TEXT("actual ordinary ground speed 480")); });
        At(1,2,Sprint);
        if (Time >= 1 && Time < 2 && Move->Velocity.Size2D() >= 660 && S.SpeedCrossedAt == 0) S.SpeedCrossedAt = Time;
        At(2,3,[&]() { Check(Move->IsSprinting() && FMath::IsNearlyEqual(Move->Velocity.Size2D(),720.f,30.f),TEXT("actual sprint ground speed 720")); UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s sprint_660_seconds=%.3f"),Role,S.SpeedCrossedAt-1); });
        At(2.5,4,[&]() { Release(EKeys::LeftShift); });
        At(3,5,[&]() { Check(!Move->IsSprinting() && FMath::IsNearlyEqual(Move->Velocity.Size2D(),480.f,30.f),TEXT("exit sprint transitions back to ordinary speed")); });
        At(4,6,[&]() { W->AimOn(); });
        At(4.7,7,[&]() { Check(W->GetLoadout().bAiming && FMath::IsNearlyEqual(Move->Velocity.Size2D(),312.f,30.f),TEXT("actual standing ADS speed 312")); Press(EKeys::LeftControl); });
        if (Time >= 5.1 && Time < 6.2) Forward(1);
        At(5.8,8,[&]() { Check(Body->bIsCrouched && FMath::IsNearlyEqual(Move->Velocity.Size2D(),240.f,30.f),TEXT("crouch ADS uses actual crouch speed 240 without multiplying")); Release(EKeys::LeftControl); W->AimOff(); });
        break;
    case SprintCost:
        if(!Server) At(.05,20,[&]() { if(auto* FPS=IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"))) { P.PreviousClientFPS=FPS->GetFloat(); FPS->Set(120.f,ECVF_SetByConsole); } UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE INJECTION CLIENT frame cap120 during real native replay clip; server60 and network lag/loss unchanged")); });
        At(.1,1,Sprint); if (Time >= .1 && Time < 3.7) Forward(1);
        if (Server) At(1,2,[&]() { auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("p.NetForceClientAdjustmentPercent")); Check(CVar != nullptr,TEXT("native correction cvar exists")); if (CVar) { P.PreviousForceAdjustment=CVar->GetFloat(); CVar->Set(1.f,ECVF_SetByCode); } UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE INJECTION SERVER native server adjustment forced during genuine unacked moves")); });
        if (Server) At(3,3,[&]() { if (auto* CVar=IConsoleManager::Get().FindConsoleVariable(TEXT("p.NetForceClientAdjustmentPercent"))) CVar->Set(P.PreviousForceAdjustment,ECVF_SetByCode); });
        At(3.75,4,[&]() { const double Seconds = Move->DebugSprintConsumedSeconds-S.SprintSeconds; Check(Seconds>3 && Seconds<4.1 && FMath::Abs((S.InitialStamina-Move->GetStamina())-6*Seconds)<.75,TEXT("six per actual ground sprint second without replay duplicate consumption")); UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s sprint_seconds=%.4f stamina=%.3f replays=%u corrections=%u unacked_peak=%d observed_fps=%.2f"),Role,Seconds,Move->GetStamina(),Move->DebugReplayCount-S.Replays,Move->DebugCorrectionCount-S.Corrections,S.MaxUnacked,S.SampleSeconds>0?S.FrameSamples/S.SampleSeconds:0); });
        At(4.2,5,[&]() { Check(Move->HasSprintIntent(),TEXT("release all movement directions preserves sprint intent")); if (!Server) Check(Move->DebugReplayCount>S.Replays && Move->DebugCorrectionCount>S.Corrections,TEXT("native correction replay observed")); });
        break;
    case Wall:
        At(.1,1,Sprint); if (Time >= .1 && Time < 3.3) Forward(1);
        At(1.1,2,[&]() { S.WallSprintSeconds=Move->DebugSprintConsumedSeconds; Check(Move->HasSprintIntent() && Move->Velocity.Size2D()<10,TEXT("real world wall blocks movement while preserving intent")); });
        At(3,3,[&]() { Check(Move->HasSprintIntent() && Move->DebugSprintConsumedSeconds-S.WallSprintSeconds<.12,TEXT("blocked wall has no continuous sprint resource charge")); });
        break;
    case RetainedTwelve:
        At(.1,1,Sprint); if ((Time >= .1 && Time < 1.75) || (Time >= 2.35 && Time < 3.5)) Forward(1);
        At(2,2,[&]() { Check(Move->GetStamina()>7 && Move->GetStamina()<20 && Move->HasSprintIntent(),TEXT("below twenty after stop retains same sprint intent")); });
        At(3,3,[&]() { Check(Move->HasSprintIntent() && Move->IsSprinting() && Move->Velocity.Size2D()>650 && Move->GetStamina()<20,TEXT("twelve point retained sprint resumes without new twenty point gate")); });
        break;
    case Directions:
        At(.1,1,Sprint); if (Time < 1.3) Forward(1); if (Time >= 1.3 && Time < 2) Forward(-1);
        At(1.7,2,[&]() { Check(!Move->HasSprintIntent() && !Move->IsSprinting(),TEXT("backward movement clears sprint intent immediately")); Release(EKeys::LeftShift); });
        At(2.5,3,Sprint); if (Time >= 2.5 && Time < 3.25) Forward(1); if (Time >= 3.25 && Time < 4) Right(1);
        At(3.65,4,[&]() { Check(!Move->HasSprintIntent(),TEXT("pure side input clears sprint intent")); });
        if (Time >= 4 && Time < 4.6) Forward(1);
        At(4.5,5,[&]() { Check(!Move->IsSprinting() && Move->Velocity.Size2D()<520,TEXT("forward after interrupted intent needs a fresh sprint trigger")); });
        break;
    case Exhaustion:
        At(.1,1,Sprint); if (Time < 4.2 || (Time>=6.4 && Time<7.4)) Forward(1);
        At(4.1,2,[&]() { Check(Move->GetStamina()<.5 && !Move->HasSprintIntent(),TEXT("sprint exhaustion clears intent without ordinary movement penalty")); });
        At(6.5,3,[&]() { Check(Move->GetStamina()>=20 && !Move->HasSprintIntent(),TEXT("recovered above twenty does not automatically restart exhausted sprint")); });
        At(7.2,4,[&]() { Check(!Move->IsSprinting() && FMath::IsNearlyEqual(Move->Velocity.Size2D(),480.f,30.f),TEXT("ordinary movement remains 480 after exhaustion")); });
        break;
    case NormalJump:
    case WeakJump:
        At(.25,1,Jump); At(.4,2,[&]() { Release(EKeys::SpaceBar); Check(Move->IsFalling() && Move->GetJumpCharges()==S.Jumps+1,TEXT("actual successful jump consumes once")); Check(Move->IsWeakJump()==(State->Scene==WeakJump),TEXT("jump strength reflects pre-action stamina")); Check(FMath::Abs(Move->GetStamina()-(State->Scene==WeakJump ? 0.f : 80.f))<1,TEXT("normal jump costs twenty and weak jump consumes remaining stamina")); if (State->Scene==WeakJump) { const auto View=UFPSInterface::ReadStaminaHUD(Body); Check(View.Visible && View.Alert && View.Exhausted && View.Status.Contains(TEXT("弱跳")) && View.Status.Contains(TEXT("体力耗尽")) && FMath::IsNearlyEqual(View.Value,Move->GetStamina()),TEXT("shared stamina HUD reports actual airborne weak jump and exhaustion")); CaptureHUD(TEXT("MovementWeakJump.png")); } });
        At(2,3,[&]() { Check(S.WasAirborne && S.FirstLanding>0 && S.Apex>0,TEXT("actual vertical trajectory rose and landed")); if (State->Scene==NormalJump) { P.NormalApex=S.Apex; Check(S.Apex>=60 && S.Apex<=72,TEXT("normal JumpZ360 observed apex is within sixty to seventy two centimeters")); } else { P.WeakApex=S.Apex; Check(S.Apex>=28 && S.Apex<=38,TEXT("weak JumpZ360 observed apex is within twenty eight to thirty eight centimeters")); Check(P.NormalApex>0 && FMath::Abs(S.Apex/P.NormalApex-.5f)<.09f,TEXT("weak jump apex is half normal height, not half velocity")); } UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s normal_apex_cm=%.3f weak_apex_cm=%.3f landing=%.3f configured_jump_velocity=%.3f gravity_z=%.3f"),Role,P.NormalApex,P.WeakApex,S.FirstLanding,GetDefault<UFPSMovementSettings>()->JumpVelocity,Move->GetGravityZ()); });
        break;
    case HeldJump:
        At(.25,1,Jump);
        At(2,2,[&]() { Check(!Move->IsFalling() && Move->GetJumpCharges()==S.Jumps+1,TEXT("holding Space does not extend jump or auto jump after landing")); Release(EKeys::SpaceBar); });
        At(2.2,3,Jump); At(2.25,4,[&]() { Release(EKeys::SpaceBar); });
        if (S.FirstLanding>0 && Time>=S.FirstLanding+.01 && !S.Done.Contains(10)) { S.Done.Add(10); Move->RequestJump(); Check(Move->GetJumpCooldownRemaining()>0,TEXT("landing starts common jump cooldown")); }
        At(3.6,5,[&]() { Check(Move->GetJumpCharges()==S.Jumps+2,TEXT("blocked landing request did not queue and fresh later jump succeeds")); });
        break;
    case CrouchNormal:
        At(.25,1,[&]() { Press(EKeys::LeftControl); }); At(.4,2,[&]() { Release(EKeys::LeftControl); });
        At(.45,3,[&]() { Press(EKeys::LeftControl); }); At(.5,4,[&]() { Release(EKeys::LeftControl); });
        At(.57,5,[&]() { Check(!Body->bIsCrouched && Move->GetCrouchCharges()==S.Crouches+1,TEXT("normal crouch interval rejects repeated entry without queue")); });
        At(.75,6,[&]() { Press(EKeys::LeftControl); }); At(.95,7,[&]() { Check(Body->bIsCrouched && Move->GetCrouchCharges()==S.Crouches+2 && Move->GetStamina()<81,TEXT("fresh crouch after point35 costs ten once")); Release(EKeys::LeftControl); });
        At(2,8,[&]() { Check(!Body->bIsCrouched && Move->GetCrouchCharges()==S.Crouches+2,TEXT("standing and maintained posture have no extra crouch costs")); });
        break;
    case CrouchLow:
        At(.25,1,[&]() { Press(EKeys::LeftControl); }); At(.45,2,[&]() { Release(EKeys::LeftControl); });
        At(.65,3,[&]() { Press(EKeys::LeftControl); }); At(.7,4,[&]() { Release(EKeys::LeftControl); });
        At(1,5,[&]() { Check(!Body->bIsCrouched && Move->GetCrouchCharges()==S.Crouches+1 && Move->GetStamina()<.5,TEXT("low stamina crouch point8 interval rejects without queue or extra cost")); const auto View=UFPSInterface::ReadStaminaHUD(Body); Check(View.Visible && View.Alert && View.Exhausted && FMath::IsNearlyEqual(View.Value,Move->GetStamina()),TEXT("exhausted HUD consumes actual locally predicted movement resource")); CaptureHUD(TEXT("MovementExhausted.png")); });
        At(1.2,6,[&]() { Press(EKeys::LeftControl); }); At(1.35,7,[&]() { Check(Body->bIsCrouched && Move->GetCrouchCharges()==S.Crouches+2,TEXT("zero stamina fresh crouch still succeeds")); Release(EKeys::LeftControl); });
        At(2.5,8,[&]() { Check(Move->GetStamina()<.5,TEXT("successful zero stamina crouch resets recovery delay")); });
        At(3.4,9,[&]() { Check(Move->IsRecovering() && Move->GetStamina()>10,TEXT("low crouch normal recovery resumes after delay")); const auto View=UFPSInterface::ReadStaminaHUD(Body); Check(View.Visible && View.Status.Contains(TEXT("恢复中")),TEXT("shared stamina HUD reports actual recovery state")); Check(Move->GetStamina()<20 && View.Alert && View.Status.Contains(TEXT("疾跑启动受限")),TEXT("low recovering HUD reports the actual twenty point sprint start restriction")); CaptureHUD(TEXT("MovementLowRecovering.png")); });
        break;
    case LowRoof:
        At(.25,1,[&]() { Press(EKeys::LeftControl); });
        if (Server) At(.8,2,[&]() { for (auto It=World->GetPlayerControllerIterator();It;++It) if (auto* Target=Cast<AFPSDemoCharacter>(It->Get()->GetPawn())) Target->SetActorLocation(FVector(-2600,Target->GetActorLocation().Y,GroundZ+50)); UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE INJECTION SERVER crouched pawn placed beneath replicated 110cm clearance roof")); });
        At(1.6,3,[&]() { Release(EKeys::LeftControl); });
        At(2.2,4,[&]() { Check(Body->bIsCrouched && !Move->IsFalling(),TEXT("actual low ceiling prevents uncrouch without penetration")); S.StaminaSample=Move->GetStamina(); });
        At(2.25,5,Jump); At(2.35,6,[&]() { Release(EKeys::SpaceBar); Check(Body->bIsCrouched && !Move->IsFalling() && Move->GetJumpCharges()==S.Jumps,TEXT("crouch jump without headroom remains crouched and has no jump charge")); });
        At(2.8,7,[&]() { Check(Move->GetStamina()>S.StaminaSample+8 && Move->IsRecovering(),TEXT("failed low roof jump does not postpone stamina recovery")); });
        break;
    case AirMomentum:
        At(.1,1,Sprint); if (Time<1.08) Forward(1); At(1,2,Jump); At(1.15,3,[&]() { Release(EKeys::SpaceBar); S.WallSprintSeconds=Move->DebugSprintConsumedSeconds; Check(Move->IsFalling() && Move->Velocity.Size2D()>550,TEXT("sprint jump preserves horizontal takeoff momentum")); });
        if (Time>=1 && Move->IsFalling() && !S.Done.Contains(10)) { S.Done.Add(10); Check(Move->Acceleration.Size()>0 && Move->Acceleration.Size()<=1201.f,TEXT("first observed airborne tick with held direction uses the air acceleration limit")); UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s first_observed_air_acceleration=%.3f air_acceleration_limit=1200 analog=%.3f"),Role,Move->Acceleration.Size(),Move->GetAnalogInputModifier()); }
        if (Time>=1.2 && Time<1.5) { Forward(-1); if (Move->IsFalling() && !S.ObservedAirBackward) { S.ObservedAirBackward=true; Check(Move->Velocity.X>100,TEXT("air backward input does not instantly reverse takeoff velocity")); } }
        At(1.6,4,[&]() { Check(!Move->HasSprintIntent() && Move->DebugSprintConsumedSeconds-S.WallSprintSeconds<.08,TEXT("air backward clears intent and air has no ground sprint charge")); });
        At(2.7,5,[&]() { Check(!Move->HasSprintIntent() && !Move->IsSprinting() && Move->Velocity.Size2D()<=730,TEXT("landing does not restore interrupted sprint or add horizontal speed")); });
        break;
    case FailedRecovery:
        At(.25,1,Jump); At(.35,2,[&]() { Release(EKeys::SpaceBar); }); At(.45,3,Jump); At(.55,4,[&]() { Release(EKeys::SpaceBar); });
        At(1.65,5,[&]() { Check(Move->GetStamina()<.5 && Move->GetJumpCharges()==S.Jumps+1,TEXT("zero stamina weak jump charges once and resets recovery delay")); });
        At(1.9,6,[&]() { Check(Move->GetStamina()>1 && Move->IsRecovering(),TEXT("failed airborne jump does not extend recovery delay")); });
        At(2.9,7,[&]() { Check(FMath::IsNearlyEqual(Move->GetStamina(),28.75f,4.f),TEXT("recovery is twenty five per simulation second after point1five delay")); });
        break;
    case InputModes:
        At(.1,1,[&]() { UFPSInputSettings::Get()->bSprintToggle=true; UFPSInputSettings::Get()->bCrouchToggle=true; });
        At(.25,2,Sprint); At(.35,3,[&]() { Release(EKeys::LeftShift); }); if (Time<1.2) Forward(1);
        At(.9,4,[&]() { Check(Move->HasSprintIntent() && Move->IsSprinting(),TEXT("native toggle sprint survives key release")); }); At(1.2,5,Sprint); At(1.3,6,[&]() { Release(EKeys::LeftShift); });
        At(1.55,7,[&]() { Check(!Move->HasSprintIntent(),TEXT("native toggle second press clears sprint")); Press(EKeys::LeftControl); }); At(1.65,8,[&]() { Release(EKeys::LeftControl); });
        At(1.9,9,[&]() { Check(Body->bIsCrouched,TEXT("native toggle crouch survives key release")); }); At(2.1,10,[&]() { Press(EKeys::LeftControl); }); At(2.2,11,[&]() { Release(EKeys::LeftControl); });
        At(2.5,12,[&]() { Check(!Body->bIsCrouched,TEXT("native toggle crouch second press stands")); UFPSInputSettings::Get()->bSprintToggle=false; UFPSInputSettings::Get()->bCrouchToggle=false; });
        At(3,13,Sprint); if (Time>=3 && Time<3.6) Forward(1); At(3.5,14,[&]() { Release(EKeys::LeftShift); }); At(3.8,15,[&]() { Check(!Move->HasSprintIntent(),TEXT("native hold sprint release clears intent")); Press(EKeys::LeftControl); });
        At(4.1,16,[&]() { Release(EKeys::LeftControl); }); At(4.4,17,[&]() { Check(!Body->bIsCrouched,TEXT("native hold crouch release stands")); });
        break;
    case WeaponPriority:
        At(.2,1,[&]() { W->Reload(); }); At(.35,2,Sprint);
        At(.8,3,[&]() { Check(W->GetLoadout().Action==EFPSWeaponAction::Reloading && !Move->HasSprintIntent(),TEXT("sprint request cannot cancel real GAS reload")); Release(EKeys::LeftShift); });
        At(3.5,4,[&]() { W->AimOn(); }); At(3.7,5,Sprint);
        At(4.1,6,[&]() { Check(W->GetLoadout().bAiming && !Move->HasSprintIntent(),TEXT("real GAS ADS blocks sprint without canceling ADS")); Release(EKeys::LeftShift); W->AimOff(); });
        At(4.6,7,Sprint); if (Time>=4.6 && Time<5.3) Forward(1);
        At(5.1,8,[&]() { Check(Move->IsSprinting(),TEXT("fresh trigger after weapon action permits sprint")); });
        break;
    case AutomaticPreparation:
        At(.1,1,Sprint); if (Time<1 || (Time>=2.2 && Time<3)) Forward(1);
        At(1,2,Fire); At(1.06,3,[&]() { Check(W->Equipped().ShotSerial==S.ShotSerial,TEXT("sprint automatic cannot shoot before point15 preparation")); W->StopFire(); });
        At(1.7,4,[&]() { Check(W->Equipped().ShotSerial==S.ShotSerial && !W->HasPendingSprintFire() && !Move->HasSprintIntent(),TEXT("automatic trigger release cancels prepared shot without auto sprint restart")); Release(EKeys::LeftShift); });
        At(2.2,5,Sprint); At(3,6,Fire); At(3.8,7,[&]() { W->StopFire(); });
        At(4.5,8,[&]() { Check(S.SawPending && S.ObservedPreparationShot && S.PreparationRespected && W->Equipped().ShotSerial>S.ShotSerial,TEXT("held automatic actual GAS shot respects sprint preparation")); });
        break;
    case SemiPreparation:
        At(.1,1,Sprint); if (Time<1 || (Time>=2.2 && Time<3)) Forward(1);
        At(1,2,Fire); At(1.04,3,[&]() { W->StopFire(); });
        At(1.8,4,[&]() { Check(S.SawPending && S.PreparationRespected && W->Equipped().ShotSerial==S.ShotSerial+1,TEXT("semi automatic tap is retained through point15 preparation exactly once")); Release(EKeys::LeftShift); });
        At(2.2,5,Sprint); At(3,6,Fire); At(3.04,7,[&]() { W->StopFire(); W->Primary(); });
        At(4.2,8,[&]() { const auto& Old=W->GetLoadout().Secondary; Check(Old.InstanceId==S.Gun && Old.ShotSerial==S.ShotSerial+1 && !W->HasPendingSprintFire() && !W->GetLoadout().bFireActive,TEXT("switch during initial preparation cancels old tap and prevents new gun phantom shot")); });
        break;
    case BurstPreparation:
        At(.1,1,Sprint); if (Time<1) Forward(1); At(1,2,Fire); At(1.04,3,[&]() { W->StopFire(); });
        At(2.3,4,[&]() { Check(S.SawPending && S.PreparationRespected && W->Equipped().ShotSerial==S.ShotSerial+3 && !W->GetLoadout().bFireActive,TEXT("burst tap waits preparation then completes exactly three existing-semantic shots")); });
        break;
    case WeaponDeadlines:
        At(.15,1,[&]() { W->Secondary(); Check(W->GetLoadout().Action==EFPSWeaponAction::Switching && !W->CanStart(EFPSWeaponRequest::Fire),TEXT("actual GAS semi equip preparation rejects premature fire")); W->StartFire(); W->StopFire(); });
        At(.7,2,Fire); At(.75,5,[&]() { W->StopFire(); });
        At(.95,6,[&]() { Check(W->GetLoadout().Secondary.ShotSerial==1,TEXT("fresh semi press after actual equip readiness shoots once")); W->Primary(); });
        At(1.65,7,Sprint); if (Time>=1.65 && Time<2.25) Forward(1); At(2.25,8,Fire);
        At(2.7,3,[&]() { Check(W->Equipped().ShotSerial==S.ShotSerial,TEXT("sprint preparation cannot bypass existing instance cooldown and equip ready deadline")); });
        At(3.6,4,[&]() { W->StopFire(); Check(W->Equipped().ShotSerial>S.ShotSerial && S.ShotAt>=State->StartsAt+2.99 && S.PreparationRespected,TEXT("actual firing waits maximum of movement and existing weapon deadlines")); });
        break;
    case Lifecycle:
        At(.1,1,Sprint); if (Time<1) Forward(1); At(1,2,Fire); At(1.04,3,[&]() { Press(EKeys::Escape); }); At(1.1,4,[&]() { Release(EKeys::Escape); });
        At(2,5,[&]() { Check(PC->IsMoveInputIgnored() && !Move->HasSprintIntent() && !W->HasPendingSprintFire() && W->Equipped().ShotSerial==S.ShotSerial,TEXT("native menu clears sprint and initial pending shot")); });
        At(2.5,6,[&]() { Release(EKeys::LeftShift); Tap(EKeys::Escape); });
        At(3.2,7,[&]() { Check(!PC->IsMoveInputIgnored() && !Move->HasSprintIntent() && !W->GetLoadout().bFireActive && Move->GetStamina()>S.InitialStamina-5,TEXT("menu world recovery continues and close does not restore old inputs")); Move->RequestJump(); Body->StopCombatInput(); });
        At(3.42,14,[&]() { Check(Move->GetJumpCharges()==S.Jumps && !Move->IsFalling(),TEXT("same-frame queued jump then StopCombatInput cancels before actual movement simulation")); });
        At(3.35,10,Sprint); if(Time>=3.35 && Time<4) Forward(1); At(4,11,Fire);
        if (Server) At(4.06,8,[&]() { Check(W->HasPendingSprintFire(),TEXT("actual prepared GAS shot exists before death injection")); for (auto It=World->GetPlayerControllerIterator();It;++It) if (auto* Target=Cast<AFPSDemoCharacter>(It->Get()->GetPawn())) { auto* ASC=Target->GetAbilitySystemComponent(); auto Context=ASC->MakeEffectContext(); auto Spec=ASC->MakeOutgoingSpec(UFPSWeaponDamageEffect::StaticClass(),1,Context); Spec.Data->SetSetByCallerMagnitude(FName(TEXT("WeaponDamage")),-200); ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()); } });
        At(4.6,12,[&]() { auto* Old=S.InitialBody.Get(); auto* OldMove=Old?Cast<UFPSMovementComponent>(Old->GetCharacterMovement()):nullptr; Check(Old && Old->IsEliminated() && OldMove && !OldMove->HasSprintIntent() && !Old->GetWeapons()->HasPendingSprintFire() && !Old->GetWeapons()->GetLoadout().bFireActive,TEXT("actual dead old body retires sprint and prepared weapon lifetime")); });
        if(Server) At(7.2,13,[&]() { for(auto It=World->GetPlayerControllerIterator();It;++It) if(auto* NewBody=Cast<AFPSDemoCharacter>(It->Get()->GetPawn())) P.SpawnTransforms.Add(It->Get(),NewBody->GetActorTransform()); });
        At(7.5,9,[&]() { Check(Body && Body!=S.InitialBody.Get() && !Body->IsEliminated() && Move && FMath::IsNearlyEqual(Move->GetStamina(),100.f) && !Move->HasSprintIntent() && W && !W->HasPendingSprintFire(),TEXT("GAS death and actual respawn creates fresh full stamina with no old intents or prepared shot")); });
        break;
    case BaseAndMenu:
        At(.05,20,[&]()
        {
            const auto* PS=PC->GetPlayerState<AFPSDemoPlayerState>(); bool Found=false;
            for(TActorIterator<AFPSBaseRegion> It(World);It;++It)
                if(PS && It->TeamId==PS->TeamId && It->Contains(Body->GetActorLocation(),4))
                {
                    FVector Inward=(It->GetActorLocation()-Body->GetActorLocation()).GetSafeNormal2D();
                    if(Inward.IsNearlyZero()) Inward=FVector::ForwardVector;
                    PC->SetControlRotation(Inward.Rotation()); Found=true; break;
                }
            if(!Training) Check(Found,TEXT("new life actual spawn is inside own region before inward movement"));
        });
        At(.1,1,Sprint); if (Time>=.1 && Time<.35) Forward(1);
        At(.45,2,[&]() { Check(Body->FindComponentByClass<UFPSBaseMembership>()->IsProtected()==!Training,TEXT("actual own base protection remains in team match and absent in training")); Check(Move->GetStamina()<40 && Move->GetStamina()>35,TEXT("base ground sprint uses ordinary six-per-second resource rules")); W->StartFire(); W->StopFire(); });
        At(1,3,[&]() { if (!Training) Check(W->Equipped().ShotSerial==S.ShotSerial,TEXT("base real attack request is still denied")); Check(Move->GetStamina()<41,TEXT("base does not instantly refill stamina")); });
        At(2,4,[&]() { Tap(EKeys::P); });
        At(2.5,5,[&]() { Check(PC->IsLoadoutOpen() && !Move->HasSprintIntent(),TEXT("actual selection menu clears sprint intent while world runs")); });
        At(3.1,6,[&]() { PC->HideLoadout(); });
        At(3.8,7,[&]() { Check(!Move->HasSprintIntent() && Move->GetStamina()>75 && Move->GetStamina()<100,TEXT("base menu recovery follows ordinary delay and twenty five rate without full refill")); });
        break;
    case EmptyInventory:
        At(.1,1,Sprint); if (Time<1.8) Forward(1);
        At(1.4,2,[&]() { Check(!W->Equipped().IsValid() && Move->IsSprinting() && Move->Velocity.Size2D()>650,TEXT("empty inventory still permits actual predicted sprint")); });
        At(2,3,Jump); At(2.15,4,[&]() { Release(EKeys::SpaceBar); Check(Move->IsFalling() && Move->GetJumpCharges()==S.Jumps+1,TEXT("empty inventory jump remains actual movement and resource action")); });
        break;
    case TrainingAI:
        if (Server && Training)
        {
            At(.1,1,[&]() { for (TActorIterator<AFPSBotController> It(World);It;++It) if (auto* Target=Cast<AFPSDemoCharacter>(It->GetPawn())) { P.AI=Target; auto* AI=CastChecked<UFPSMovementComponent>(Target->GetCharacterMovement()); AI->ClearMovementIntents(); AI->StopMovementImmediately(); Target->SetActorLocation(FVector(-3000,1000,GroundZ+90)); Target->SetActorRotation(FRotator::ZeroRotator); AI->SetStaminaForTest(0); P.AIJumps=AI->GetJumpCharges(); P.AICrouches=AI->GetCrouchCharges(); P.AIMoveSeconds=AI->DebugSprintConsumedSeconds; AI->RequestSprint(true); break; } Check(P.AI.IsValid(),TEXT("training contains actual bot controller fixture")); });
            if (P.AI.IsValid() && Time>=.1 && Time<1.2) P.AI->AddMovementInput(FVector::ForwardVector,1);
            At(1,2,[&]() { auto* AI=P.AI.IsValid()?Cast<UFPSMovementComponent>(P.AI->GetCharacterMovement()):nullptr; Check(AI && AI->UsesMovementRules() && !AI->UsesStamina() && AI->Velocity.Size2D()>600 && AI->DebugSprintConsumedSeconds==P.AIMoveSeconds,TEXT("actual AI ground sprint bypasses stamina consumption only")); if(AI) { P.AIJumpRequestedAt=World->GetTimeSeconds(); AI->RequestJump(); } });
            At(1.15,3,[&]() { auto* AI=P.AI.IsValid()?Cast<UFPSMovementComponent>(P.AI->GetCharacterMovement()):nullptr; Check(AI && AI->IsFalling() && !AI->UsesStamina() && !AI->IsWeakJump() && FMath::IsNearlyEqual(AI->JumpZVelocity,360.f) && AI->Velocity.Z>=180 && AI->Velocity.Z<=240 && AI->GetStamina()==0,TEXT("zero stamina actual AI has ordinary JumpZ360 trajectory without weak penalty or stamina cost")); UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE SERVER ai_jump vertical_velocity=%.3f jump_velocity=%.3f weak=%d observed_seconds_after_request=%.4f vertical_velocity_limits=180..240 frame_seconds=%.4f"),AI?AI->Velocity.Z:-1.f,AI?AI->JumpZVelocity:-1.f,AI?AI->IsWeakJump():true,World->GetTimeSeconds()-P.AIJumpRequestedAt,DeltaSeconds); });
            At(2.5,4,[&]() { if(P.AI.IsValid()) { auto* AI=CastChecked<UFPSMovementComponent>(P.AI->GetCharacterMovement()); AI->RequestCrouch(true); } });
            At(2.7,5,[&]() { auto* AI=P.AI.IsValid()?Cast<UFPSMovementComponent>(P.AI->GetCharacterMovement()):nullptr; Check(AI && P.AI->bIsCrouched && AI->GetCrouchCooldownRemaining()<.36f,TEXT("AI crouch avoids low stamina penalty while retaining posture rules")); });
        }
        break;
    case GroundResponse:
    {
        if (S.GroundMeasures.IsEmpty()) S.GroundMeasures.SetNum(7);
        auto BeginMeasure = [&](int32 Index,const TCHAR* Name,float SecondsLimit,float MinSpeed,float MaxSpeed,
            const FVector& Direction,float DistanceLimit=0.f,float MinimumSeconds=0.f,float AngleLimit=5.f)
        {
            auto& Measurement=S.GroundMeasures[Index]; Measurement.Name=Name;
            Measurement.Started=World->GetTimeSeconds(); Measurement.Position=Body->GetActorLocation(); Measurement.Direction=Direction;
            Measurement.SecondsLimit=SecondsLimit; Measurement.MinSpeed=MinSpeed; Measurement.MaxSpeed=MaxSpeed;
            Measurement.DistanceLimit=DistanceLimit; Measurement.MinimumSeconds=MinimumSeconds; Measurement.AngleLimit=AngleLimit;
            Measurement.Corrections=Move->DebugCorrectionCount;
            UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s ground_begin=%s local_time=%.4f native_stamp=%.4f speed=%.3f speed_limit=%.3f grounded=%d limit_seconds=%.3f limit_distance=%.3f angle_limit=%.1f"),
                Role,Name,Measurement.Started,Server?-1.f:Move->GetPredictionData_Client_Character()->CurrentTimeStamp,Move->Velocity.Size2D(),Move->Resource.SpeedLimit,Move->IsMovingOnGround(),SecondsLimit,DistanceLimit,AngleLimit);
        };
        At(.1,1,[&]() { Check(Move->Velocity.Size2D()<5,TEXT("ground start measurement begins from actual rest")); BeginMeasure(0,TEXT("walk-start"),.10f,460,500,FVector::ForwardVector); });
        if ((Time>=.1 && Time<.8) || (Time>=1.3 && Time<1.9) || (Time>=3 && Time<4.1) || (Time>=5 && Time<7.8)) Forward(1);
        At(.8,2,[&]() { Check(FMath::IsNearlyEqual(Move->Velocity.Size2D(),480.f,20.f),TEXT("ordinary release begins at actual walking speed")); BeginMeasure(1,TEXT("walk-stop"),.10f,0,5,FVector::ForwardVector,20); });
        At(1.9,3,[&]() { Check(Move->Velocity.X>460,TEXT("ordinary reverse begins from actual forward walking")); BeginMeasure(2,TEXT("walk-reverse"),.15f,460,500,-FVector::ForwardVector); });
        if (Time>=1.9 && Time<2.6) Forward(-1);
        At(3,4,Sprint);
        At(4.1,5,[&]() { Check(Move->IsSprinting() && FMath::IsNearlyEqual(Move->Velocity.Size2D(),720.f,20.f),TEXT("sprint release begins at actual full sprint speed")); BeginMeasure(3,TEXT("sprint-stop"),.12f,0,5,FVector::ForwardVector,40); });
        At(4.6,6,[&]() { Release(EKeys::LeftShift); });
        At(5.2,7,[&]() { Check(!Move->IsSprinting() && FMath::IsNearlyEqual(Move->Velocity.Size2D(),480.f,20.f),TEXT("sprint ramp begins from actual ordinary walking")); BeginMeasure(4,TEXT("walk-to-sprint"),.28f,700,740,FVector::ForwardVector,0,.10f); Sprint(); });
        At(6.4,8,[&]()
        {
            Check(Move->IsSprinting() && Move->Velocity.X>680,TEXT("ninety degree look turn begins from actual forward sprint"));
            S.TurnStartedYaw=Body->GetActorRotation().Yaw;
            BeginMeasure(5,TEXT("sprint-look-turn90"),.15f,660,740,FVector::RightVector,0,0,10);
            float YawScale=1.f;
            PRAGMA_DISABLE_DEPRECATION_WARNINGS
            if(GetDefault<UInputSettings>()->bEnableLegacyInputScales) YawScale=PC->GetDeprecatedInputYawScale();
            PRAGMA_ENABLE_DEPRECATION_WARNINGS
            Check(FMath::Abs(YawScale)>KINDA_SMALL_NUMBER,TEXT("real yaw input has a nonzero configured scale"));
            if(FMath::Abs(YawScale)>KINDA_SMALL_NUMBER)
            {
                Body->LookYaw(FInputActionValue(90.f/YawScale));
                // The opt-in hook runs after PlayerController's normal UpdateRotation.
                // Consume this real yaw input before TickActor clears RotationInput,
                // then replace only this frame's already accumulated old forward input.
                PC->UpdateRotation(0.f);
                Body->ConsumeMovementInputVector();
                Forward(1);
            }
        });
        At(6.6,9,[&]() { const float Turn=FMath::FindDeltaAngleDegrees(S.TurnStartedYaw,Body->GetActorRotation().Yaw); Check(FMath::Abs(Turn-90.f)<1.f && Move->HasSprintIntent(),TEXT("real look input rotates the actual actor ninety degrees while preserving forward sprint intent")); UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s actual_look_turn_degrees=%.3f"),Role,Turn); });
        At(6.9,10,[&]() { Check(Move->IsSprinting() && Move->Velocity.Y>680,TEXT("sprint exit measurement starts at actual turned sprint speed")); BeginMeasure(6,TEXT("sprint-to-walk"),.17f,450,500,FVector::RightVector,0,.08f); Release(EKeys::LeftShift); });
        for (auto& Measurement:S.GroundMeasures)
        {
            if(Measurement.Started<0 || Measurement.Reported) continue;
            const float Speed=Move->Velocity.Size2D();
            const float Angle=Speed<5 ? 0.f : FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Move->Velocity.GetSafeNormal2D(),Measurement.Direction),-1.,1.)));
            const bool Reached=Move->IsMovingOnGround() && Speed>=Measurement.MinSpeed && Speed<=Measurement.MaxSpeed && Angle<=Measurement.AngleLimit;
            Measurement.Elapsed=World->GetTimeSeconds()-Measurement.Started;
            if(!Reached && Measurement.Elapsed<.75) continue;
            Measurement.Reported=true;
            const float Distance=FVector::Dist2D(Measurement.Position,Body->GetActorLocation());
            UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s ground_response=%s elapsed=%.4f distance_cm=%.3f speed=%.3f angle_degrees=%.3f elapsed_limit=%.3f distance_limit=%.3f angle_limit=%.1f minimum_seconds=%.3f frame_seconds=%.4f native_stamp=%.4f corrections=%u reached=%d"),
                Role,*Measurement.Name,Measurement.Elapsed,Distance,Speed,Angle,Measurement.SecondsLimit,Measurement.DistanceLimit,Measurement.AngleLimit,Measurement.MinimumSeconds,DeltaSeconds,Server?-1.f:Move->GetPredictionData_Client_Character()->CurrentTimeStamp,Move->DebugCorrectionCount-Measurement.Corrections,Reached);
            Check(Reached && Measurement.Elapsed<=Measurement.SecondsLimit && Measurement.Elapsed>=Measurement.MinimumSeconds && (Measurement.DistanceLimit<=0 || Distance<=Measurement.DistanceLimit),*FString::Printf(TEXT("actual ground response %s meets measured time distance and direction limits"),*Measurement.Name));
        }
        At(8.5,11,[&]() { bool Complete=true; for(const auto& Measurement:S.GroundMeasures) Complete &= Measurement.Reported; Check(Complete,TEXT("all seven actual ground response measurements completed")); });
        break;
    }
    case AirSteering:
    {
        const double LocalNow=World->GetTimeSeconds();
        auto FlatVelocity=[&]() { FVector Value=Move->Velocity; Value.Z=0; return Value; };
        auto AngleBetween=[&](const FVector& A,const FVector& B)
        { return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(A.GetSafeNormal(),B.GetSafeNormal()),-1.,1.))); };
        auto LookTo=[&](float DesiredYaw)
        {
            const float DeltaYaw=FMath::FindDeltaAngleDegrees(Body->GetActorRotation().Yaw,DesiredYaw);
            float YawScale=1.f;
            PRAGMA_DISABLE_DEPRECATION_WARNINGS
            if(GetDefault<UInputSettings>()->bEnableLegacyInputScales) YawScale=PC->GetDeprecatedInputYawScale();
            PRAGMA_ENABLE_DEPRECATION_WARNINGS
            Check(FMath::Abs(YawScale)>KINDA_SMALL_NUMBER,TEXT("air steering real yaw input has a nonzero configured scale"));
            if(FMath::Abs(YawScale)>KINDA_SMALL_NUMBER)
            {
                Body->LookYaw(FInputActionValue(DeltaYaw/YawScale)); PC->UpdateRotation(0.f);
                Body->ConsumeMovementInputVector();
            }
            Check(FMath::Abs(FMath::FindDeltaAngleDegrees(Body->GetActorRotation().Yaw,DesiredYaw))<1.f,TEXT("air steering actual actor heading follows real look input"));
        };
        At(.1,1,Sprint);
        At(1,2,[&]() { Check(Move->IsSprinting() && Move->Velocity.X>700 && !Move->IsFalling(),TEXT("air steering first normal jump begins at actual full ground sprint")); Jump(); });
        At(1.06,3,[&]() { Check(Move->IsFalling() && Move->Velocity.Size2D()>700 && !Move->IsWeakJump(),TEXT("air steering fixture observes actual normal airborne sprint momentum")); S.AirTurnAt=LocalNow; S.AirTurnVelocity=FlatVelocity(); S.AirSteeringPhase=1; LookTo(90); });
        At(1.12,4,[&]() { Release(EKeys::SpaceBar); });
        if(S.AirSteeringPhase==1 && LocalNow-S.AirTurnAt>=.25)
        {
            const FVector Velocity=FlatVelocity(); const float SideGain=Velocity.Y-S.AirTurnVelocity.Y;
            const float Angle=AngleBetween(S.AirTurnVelocity,Velocity);
            UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s air_steering elapsed=%.4f lateral_gain=%.3f angle_degrees=%.3f speed=%.3f acceleration=%.3f effective_air_control=%.3f lateral_gain_limits=120..300 angle_limits=10..30 speed_limit=722 native_stamp=%.4f corrections=%u"),Role,LocalNow-S.AirTurnAt,SideGain,Angle,Velocity.Size(),Move->Acceleration.Size(),Move->AirControl,Server?-1.f:Move->GetPredictionData_Client_Character()->CurrentTimeStamp,Move->DebugCorrectionCount);
            Check(Move->IsFalling() && SideGain>=120 && SideGain<=300 && Angle>=10 && Angle<=30 && Velocity.Size()<=722,TEXT("real airborne look turn gains useful gradual lateral control without exceeding takeoff speed"));
            S.AirCoastAt=LocalNow; S.AirCoastVelocity=Velocity; S.AirSteeringPhase=2; Body->ConsumeMovementInputVector();
        }
        if(S.AirSteeringPhase==2 && LocalNow-S.AirCoastAt>=.10)
        {
            const FVector Velocity=FlatVelocity(); const float Drift=(Velocity-S.AirCoastVelocity).Size();
            const float Angle=AngleBetween(S.AirCoastVelocity,Velocity);
            UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s air_coast elapsed=%.4f velocity_drift=%.3f angle_degrees=%.3f speed=%.3f drift_limit=5 angle_limit=1"),Role,LocalNow-S.AirCoastAt,Drift,Angle,Velocity.Size());
            Check(Move->IsFalling() && Drift<=5 && Angle<=1,TEXT("releasing airborne direction preserves actual horizontal momentum"));
            LookTo(0); S.AirReverseAt=LocalNow; S.AirSteeringPhase=3;
        }
        if(S.AirSteeringPhase==3 && LocalNow-S.AirReverseAt>=.10)
        {
            UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s air_reverse elapsed=%.4f vx=%.3f vy=%.3f speed=%.3f intent=%d retained_x_limit=400 speed_limit=722"),Role,LocalNow-S.AirReverseAt,Move->Velocity.X,Move->Velocity.Y,Move->Velocity.Size2D(),Move->HasSprintIntent());
            Check(Move->IsFalling() && Move->Velocity.X>400 && FVector::DotProduct(FlatVelocity(),-FVector::ForwardVector)<0 && !Move->HasSprintIntent() && Move->Velocity.Size2D()<=722,TEXT("real airborne backward input clears sprint intent and cannot instantly reverse horizontal momentum"));
            S.AirSteeringPhase=4; Body->ConsumeMovementInputVector();
        }
        At(2.7,5,[&]() { Release(EKeys::LeftShift); });
        At(3,6,[&]() { Check(Move->IsMovingOnGround(),TEXT("second air fixture begins after actual first landing")); LookTo(0); });
        At(3.3,7,Sprint);
        At(4.2,8,[&]() { Check(Move->IsSprinting() && Move->Velocity.X>700 && Move->HasSprintIntent(),TEXT("continuous landing fixture begins from actual seven twenty forward sprint")); S.SecondJumpBaseZ=Body->GetActorLocation().Z; Jump(); });
        At(4.32,9,[&]() { Release(EKeys::SpaceBar); });
        if((Time>=.05 && Time<1.06) || S.AirSteeringPhase==1 || (Time>=3 && Time<6.2)) Forward(1);
        if(S.AirSteeringPhase==3) Forward(-1);
        if(Move->IsFalling()) S.AirHighestSpeed=FMath::Max(S.AirHighestSpeed,Move->Velocity.Size2D());
        if(Time>=4.2 && !S.LandingCurveReported)
        {
            if(Move->IsFalling() && !S.SawSecondFlight)
            {
                S.SawSecondFlight=true; S.SecondTakeoffAt=LocalNow; S.SecondTakeoffPosition=Body->GetActorLocation();
                S.SecondTakeoffCorrections=Move->DebugCorrectionCount;
                UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s flight_takeoff first_observed_falling_time=%.4f speed=%.3f vertical_velocity=%.3f frame_seconds=%.4f native_stamp=%.4f"),Role,LocalNow,Move->Velocity.Size2D(),Move->Velocity.Z,DeltaSeconds,Server?-1.f:Move->GetPredictionData_Client_Character()->CurrentTimeStamp);
            }
            if(S.SawSecondFlight)
            {
                S.SecondJumpApex=FMath::Max(S.SecondJumpApex,float(Body->GetActorLocation().Z-S.SecondJumpBaseZ));
                S.LandingCurve.Add({LocalNow,float(Move->Velocity.Size2D()),Move->IsFalling()});
                if(S.SecondWasFalling && Move->IsMovingOnGround() && S.AirLandedAt<0)
                {
                    S.AirLandedAt=LocalNow;
                    const double AirTime=LocalNow-S.SecondTakeoffAt;
                    const float Distance=FVector::Dist2D(S.SecondTakeoffPosition,Body->GetActorLocation());
                    UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s sprint_jump360 airtime=%.4f horizontal_distance=%.3f apex_cm=%.3f airtime_limits=0.68..0.79 distance_limits=490..570 apex_limits=60..72 sampling=first_observed_Falling_to_first_observed_Walking frame_seconds=%.4f native_stamp=%.4f corrections=%u"),Role,AirTime,Distance,S.SecondJumpApex,DeltaSeconds,Server?-1.f:Move->GetPredictionData_Client_Character()->CurrentTimeStamp,Move->DebugCorrectionCount-S.SecondTakeoffCorrections);
                    Check(AirTime>=.68 && AirTime<=.79 && Distance>=490 && Distance<=570 && S.SecondJumpApex>=60 && S.SecondJumpApex<=72,TEXT("actual full sprint JumpZ360 has shortened airborne time range and independently measured height"));
                }
                S.SecondWasFalling=Move->IsFalling();
                if(S.AirLandedAt>=0 && LocalNow-S.AirLandedAt>=.15)
                {
                    float Minimum=BIG_NUMBER,Maximum=0; int32 Samples=0,AirSamples=0,GroundSamples=0;
                    for(const auto& Sample:S.LandingCurve)
                    {
                        if(Sample.At<S.AirLandedAt-.10 || Sample.At>S.AirLandedAt+.17) continue;
                        Minimum=FMath::Min(Minimum,Sample.Speed); Maximum=FMath::Max(Maximum,Sample.Speed); ++Samples;
                        Sample.Falling?++AirSamples:++GroundSamples;
                        UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s air_landing_curve relative_time=%.4f speed=%.3f falling=%d"),Role,Sample.At-S.AirLandedAt,Sample.Speed,Sample.Falling);
                    }
                    UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s air_landing_summary minimum=%.3f maximum=%.3f samples=%d air=%d ground=%d minimum_limit=680 maximum_limit=722 native_stamp=%.4f corrections=%u"),Role,Minimum,Maximum,Samples,AirSamples,GroundSamples,Server?-1.f:Move->GetPredictionData_Client_Character()->CurrentTimeStamp,Move->DebugCorrectionCount);
                    Check(Samples>=6 && AirSamples>0 && GroundSamples>0 && Minimum>=680 && Maximum<=722 && Move->HasSprintIntent(),TEXT("held forward sprint has no seven twenty to four eighty speed dip across actual landing"));
                    S.LandingCurveReported=true;
                }
            }
        }
        At(6.3,10,[&]() { Release(EKeys::LeftShift); Check(S.AirSteeringPhase==4 && S.LandingCurveReported && S.AirHighestSpeed<=722,TEXT("both actual air steering flights finish with horizontal speed cap and landing evidence")); });
        break;
    }
    case RoundEnd:
        At(.1,1,Sprint); if (Time<.6) Forward(1); At(.6,4,Fire);
        if (Server && !Training) At(.66,2,[&]() { Check(W->HasPendingSprintFire(),TEXT("actual prepared GAS shot exists before round end")); World->GetAuthGameMode<AFPSMatchMode>()->FinishRound(); });
        At(1.4,3,[&]() { if (!Training) Check(!Body->GetActionPolicy()->CanMove() && !Move->HasSprintIntent() && !W->HasPendingSprintFire(),TEXT("actual round end clears movement and prepared weapon input")); else { Body->StopCombatInput(); Check(!Move->HasSprintIntent(),TEXT("training explicit lifecycle input stop clears sprint")); } });
        break;
    }
    if (Server)
    {
        At(Duration(State->Scene),980,[&]()
        {
            State->References.Reset(); State->ReferenceScene=State->Scene; State->ReferenceAt=GS->GetServerWorldTimeSeconds();
            for (auto It=World->GetPlayerControllerIterator();It;++It)
            {
                auto* Target=Cast<AFPSDemoCharacter>(It->Get()->GetPawn()); auto* PS=It->Get()->GetPlayerState<AFPSDemoPlayerState>();
                if (!Target || !PS) continue; auto* Movement=CastChecked<UFPSMovementComponent>(Target->GetCharacterMovement());
                FFPSMovementSmokeReference Ref; Ref.PlayerId=PS->GetPlayerId(); Ref.Stamina=Movement->GetStamina(); Ref.MaxStamina=Movement->GetMaxStamina(); Ref.Recovering=Movement->IsRecovering(); Ref.RecoveryRemaining=Movement->Resource.RecoveryRemaining; Ref.SprintIntent=Movement->HasSprintIntent();
                if (!It->Get()->IsLocalController()) Ref.NativeMoveTimeStamp=Movement->GetPredictionData_Server_Character()->CurrentClientTimeStamp;
                Ref.JumpCharges=Movement->GetJumpCharges(); Ref.CrouchCharges=Movement->GetCrouchCharges(); Ref.SprintSeconds=Movement->DebugSprintConsumedSeconds; State->References.Add(Ref);
                UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE AUTHORITY SERVER scene=%d player=%d stamina=%.3f recovering=%d recovery_remaining=%.4f native_processed=%.4f world_ref=%.4f sprintIntent=%d jumps=%u crouches=%u groundSeconds=%.4f"),State->Scene,Ref.PlayerId,Ref.Stamina,Ref.Recovering,Ref.RecoveryRemaining,Ref.NativeMoveTimeStamp,State->ReferenceAt,Ref.SprintIntent,Ref.JumpCharges,Ref.CrouchCharges,Ref.SprintSeconds);
            }
            State->ForceNetUpdate();
        });
    }
    At(Duration(State->Scene)+.65,990,[&]()
    {
        if(!Server && State->Scene==SprintCost) { if(auto* FPS=IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"))) FPS->Set(P.PreviousClientFPS,ECVF_SetByConsole); UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE INJECTION CLIENT restored original frame cap%.1f"),P.PreviousClientFPS); }
        if (!Server)
        {
            const auto* PS=PC->GetPlayerState<AFPSDemoPlayerState>(); const auto* Reference=State->References.FindByPredicate([&](const FFPSMovementSmokeReference& Ref) { return PS && Ref.PlayerId==PS->GetPlayerId(); });
            const auto* Native=Move->GetPredictionData_Client_Character();
            float SimulationAge=Reference ? Native->CurrentTimeStamp-Reference->NativeMoveTimeStamp : -1.f;
            // The authority resource is at the last processed client move, which can precede
            // its world-time capture. Compare at the predicted CMC boundary, including real
            // unacknowledged moves, rather than treating that capture as simulation time.
            const float ResetPeriod=Move->MinTimeBetweenTimeStampResets;
            if (SimulationAge < -ResetPeriod*.5f) SimulationAge+=ResetPeriod;
            else if (SimulationAge > ResetPeriod*.5f) SimulationAge-=ResetPeriod;
            const float Expected=Reference ? AuthorityRecoveryEstimate(*Reference,SimulationAge) : -100;
            Check(Reference && Reference->NativeMoveTimeStamp>=0 && SimulationAge>=0 && State->ReferenceScene==State->Scene && FMath::Abs(Move->GetStamina()-Expected)<2.5f,TEXT("owner predicted resource converges with timestamped server simulation reference"));
            Check(Reference && Move->GetJumpCharges()==Reference->JumpCharges && Move->GetCrouchCharges()==Reference->CrouchCharges,TEXT("owner and authority actual discrete action counts agree after replay"));
            UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE CLIENT scene=%d stamina=%.3f authority_estimate=%.3f ref_age=%.3f native_age=%.4f native_processed=%.4f native_predicted=%.4f native_ack=%.4f recovery_remaining=%.4f unacked=%d replays=%u corrections=%u"),State->Scene,Move->GetStamina(),Expected,GS->GetServerWorldTimeSeconds()-State->ReferenceAt,SimulationAge,Reference?Reference->NativeMoveTimeStamp:-1.f,Native->CurrentTimeStamp,Native->LastAckedMove.IsValid()?Native->LastAckedMove->TimeStamp:-1.f,Reference?Reference->RecoveryRemaining:-1.f,Native->SavedMoves.Num(),Move->DebugReplayCount,Move->DebugCorrectionCount);
        }
        if (FParse::Param(FCommandLine::Get(),TEXT("FPSMovementVisual")) && State->Scene==CrouchLow) FScreenshotRequest::RequestScreenshot(FPaths::ScreenShotDir()/TEXT("MovementStamina.png"),true,false);
    });
    if (Server && Time>=Duration(State->Scene)+1.25)
    {
        if (State->Scene+1<SceneCount) { ++State->Scene; State->StartsAt=GS->GetServerWorldTimeSeconds()+1.2; State->ForceNetUpdate(); }
    }
    if (State->Scene==RoundEnd) At(Duration(RoundEnd)+1,999,[&]()
    {
        UFPSInputSettings::Get()->bSprintToggle=P.OriginalSprintToggle; UFPSInputSettings::Get()->bCrouchToggle=P.OriginalCrouchToggle;
        ReleaseKeys(PC); P.Completed=true;
        Check(!P.Failed,TEXT("COMPLETE"));
    });
#endif
}

void FFPSMovementSmoke::TickInputUI(AFPSFrontController* PC)
{
#if !UE_BUILD_SHIPPING
    if(!FParse::Param(FCommandLine::Get(),TEXT("FPSMovementSmoke")) || !FParse::Param(FCommandLine::Get(),TEXT("FPSMovementInputUI")) || !PC->IsLocalController() || !PC->HasAuthority() || !PC->IsWeaponContentVerified() || !PC->GetPawn()) return;
    struct FUIProgress
    {
        double Started=0,SaveAt=0; int32 Step=0; bool Failed=false,Complete=false;
        bool ExpectedSprint=false,ExpectedCrouch=false; FString Screenshot;
    };
    static TMap<TWeakObjectPtr<UWorld>,FUIProgress> Progress;auto& P=Progress.FindOrAdd(PC->GetWorld());
    if(P.Complete)return;
    auto Check=[&](bool Pass,const TCHAR* Name)
    {P.Failed|=!Pass;UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_UI_INPUT %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),Name);return Pass;};
    auto Finish=[&]() {P.Complete=true;UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_UI_INPUT %s COMPLETE"),P.Failed?TEXT("FAIL"):TEXT("PASS"));};
    if(P.Started==0)
    {
        P.Started=PC->GetWorld()->GetTimeSeconds();
        FString ExpectedIni;FParse::Value(FCommandLine::Get(),TEXT("GameUserSettingsINI="),ExpectedIni);
        const FString ResolvedIni=FPaths::GetExtension(GGameUserSettingsIni)==TEXT("ini")?GGameUserSettingsIni:FConfigCacheIni::GetDestIniFilename(*GGameUserSettingsIni,nullptr,*FPaths::GeneratedConfigDir());
        const FString Actual=FPaths::ConvertRelativePathToFull(ResolvedIni);
        const FString Expected=FPaths::ConvertRelativePathToFull(ExpectedIni);
        const FString Allowed=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("FPSMovementTests"));
        UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_UI_INPUT PATH key=%s actual=%s expected=%s allowed=%s"),*GGameUserSettingsIni,*Actual,*Expected,*Allowed);
        if(!Check(!ExpectedIni.IsEmpty() && FPaths::IsSamePath(Actual,Expected) && FPaths::IsUnderDirectory(Actual,Allowed),TEXT("isolated GameUserSettingsINI path matches command line and stays in movement evidence directory"))) {Finish();return;}
        UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_UI_INPUT INJECTION actual Widget OnClicked broadcasts; settings path=%s; no hardware mouse click claim"),*Actual);
        if(FParse::Param(FCommandLine::Get(),TEXT("FPSMovementInputUIReadback")))
        {
            int32 Sprint=-1,Crouch=-1;FParse::Value(FCommandLine::Get(),TEXT("FPSMovementInputUIExpectedSprint="),Sprint);FParse::Value(FCommandLine::Get(),TEXT("FPSMovementInputUIExpectedCrouch="),Crouch);
            auto* Settings=UFPSInputSettings::Get();FConfigFile Disk;bool SavedSprint=false,SavedCrouch=false;Disk.Read(Actual);
            const bool Found=Disk.GetBool(TEXT("/Script/MCP_Test.FPSInputSettings"),TEXT("bSprintToggle"),SavedSprint)&&Disk.GetBool(TEXT("/Script/MCP_Test.FPSInputSettings"),TEXT("bCrouchToggle"),SavedCrouch);
            Check(Found && (Sprint==0||Sprint==1) && (Crouch==0||Crouch==1) && SavedSprint==(Sprint!=0) && SavedCrouch==(Crouch!=0) && Settings->bSprintToggle==SavedSprint && Settings->bCrouchToggle==SavedCrouch,TEXT("new process reads persisted control preferences from actual disk"));
            UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_UI_INPUT %s NEW_PROCESS sprint=%d crouch=%d"),P.Failed?TEXT("FAIL"):TEXT("PASS"),Settings->bSprintToggle,Settings->bCrouchToggle);
            Finish();return;
        }
        FParse::Value(FCommandLine::Get(),TEXT("FPSMovementInputUIScreenShot="),P.Screenshot);
        if(!Check(!P.Screenshot.IsEmpty() && FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(P.Screenshot),Allowed),TEXT("screenshot path is explicit and isolated"))) {Finish();return;}
    }
    const double Time=PC->GetWorld()->GetTimeSeconds()-P.Started;
    if(Time>35){Check(false,TEXT("bounded UI input and disk persistence deadline"));Finish();return;}
    if(P.Step==0 && Time>=.5)
    {
        PC->ShowMenu();auto* UI=PC->MenuAsset.Get();
        if(!Check(UI!=nullptr,TEXT("actual current mode menu widget exists"))){Finish();return;}
        auto* Button=Cast<UFPSActionButton>(UI->GetWidgetFromName(TEXT("InputSettings")));
        if(!Check(Button!=nullptr,TEXT("saved menu contains actual input settings button"))){Finish();return;}
        Button->OnClicked.Broadcast();P.Step=1;return;
    }
    if(P.Step==1 && Time>=.85)
    {
        auto* UI=PC->MenuAsset.Get();auto* Panel=UI?UI->GetWidgetFromName(TEXT("InputSettingsPanel")):nullptr;
        auto* Sprint=UI?Cast<UFPSActionButton>(UI->GetWidgetFromName(TEXT("SprintMode"))):nullptr;
        auto* Crouch=UI?Cast<UFPSActionButton>(UI->GetWidgetFromName(TEXT("CrouchMode"))):nullptr;
        if(!Check(Panel && Panel->GetVisibility()!=ESlateVisibility::Collapsed && Panel->GetVisibility()!=ESlateVisibility::Hidden && Sprint && Crouch,TEXT("actual settings panel opens with both mode buttons"))){Finish();return;}
        auto* Settings=UFPSInputSettings::Get();P.ExpectedSprint=!Settings->bSprintToggle;P.ExpectedCrouch=!Settings->bCrouchToggle;
        Sprint->OnClicked.Broadcast();Crouch->OnClicked.Broadcast();P.SaveAt=PC->GetWorld()->GetTimeSeconds();P.Step=2;return;
    }
    if(P.Step==2 && PC->GetWorld()->GetTimeSeconds()-P.SaveAt>=.3)
    {
        auto* Settings=UFPSInputSettings::Get();auto* UI=PC->MenuAsset.Get();
        Check(Settings->bSprintToggle==P.ExpectedSprint && Settings->bCrouchToggle==P.ExpectedCrouch,TEXT("actual mode buttons apply local control settings"));
        auto* SprintLabel=UI?Cast<UTextBlock>(UI->GetWidgetFromName(TEXT("SprintModeLabel"))):nullptr;
        auto* CrouchLabel=UI?Cast<UTextBlock>(UI->GetWidgetFromName(TEXT("CrouchModeLabel"))):nullptr;
        Check(SprintLabel && CrouchLabel && SprintLabel->GetText().ToString().Contains(P.ExpectedSprint?TEXT("切换"):TEXT("按住")) && CrouchLabel->GetText().ToString().Contains(P.ExpectedCrouch?TEXT("切换"):TEXT("按住")),TEXT("actual displayed mode labels follow settings"));
        const FString ResolvedIni=FPaths::GetExtension(GGameUserSettingsIni)==TEXT("ini")?GGameUserSettingsIni:FConfigCacheIni::GetDestIniFilename(*GGameUserSettingsIni,nullptr,*FPaths::GeneratedConfigDir());
        FConfigFile Disk;Disk.Read(FPaths::ConvertRelativePathToFull(ResolvedIni));bool SavedSprint=false,SavedCrouch=false;
        Check(Disk.GetBool(TEXT("/Script/MCP_Test.FPSInputSettings"),TEXT("bSprintToggle"),SavedSprint)&&Disk.GetBool(TEXT("/Script/MCP_Test.FPSInputSettings"),TEXT("bCrouchToggle"),SavedCrouch)&&SavedSprint==P.ExpectedSprint&&SavedCrouch==P.ExpectedCrouch,TEXT("mode buttons saved actual isolated disk values"));
        Settings->bSprintToggle=!P.ExpectedSprint;Settings->bCrouchToggle=!P.ExpectedCrouch;
        Settings->ReloadConfig(nullptr,*GGameUserSettingsIni);
        Check(Settings->bSprintToggle==P.ExpectedSprint && Settings->bCrouchToggle==P.ExpectedCrouch,TEXT("ReloadConfig restores persisted values after deliberately changing memory"));
        UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_UI_INPUT %s SAVED sprint=%d crouch=%d"),P.Failed?TEXT("FAIL"):TEXT("PASS"),Settings->bSprintToggle,Settings->bCrouchToggle);
        FScreenshotRequest::RequestScreenshot(P.Screenshot,true,false);P.Step=3;return;
    }
    if(P.Step==3 && PC->GetWorld()->GetTimeSeconds()-P.SaveAt>=.6 && IFileManager::Get().FileSize(*P.Screenshot)>0)
    {Check(true,TEXT("rendered settings screenshot exists after real frames"));Finish();}
#endif
}

void FFPSMovementSmoke::TickTeaching(AFPSDemoCharacter* Body, float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(),TEXT("FPSMovementSmoke")) || !FParse::Param(FCommandLine::Get(),TEXT("FPSMovementTeaching")) || !Body->IsLocallyControlled()) return;
    auto* World=Body->GetWorld(); auto* GS=World->GetGameState<AFPSDemoGameState>();
    if (!GS || GS->PlayerArray.Num()!=2) return;
    struct FTeaching { double Started=0,WaitingSince=0; bool Complete=false,Failed=false; TSet<int32> Done; float BaseZ=0,Apex=0; };
    static TMap<TWeakObjectPtr<UWorld>,FTeaching> Progress; auto& P=Progress.FindOrAdd(World);
    if(P.Complete)return;
    const bool Server=Body->HasAuthority(); const TCHAR* Role=Server?TEXT("SERVER"):TEXT("CLIENT");
    auto* Move=Body->GetCharacterMovement();
    const auto* CustomMove=Cast<UFPSMovementComponent>(Move);
    auto Check=[&](bool Pass,const TCHAR* Name){P.Failed|=!Pass;UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE %s %s %s teaching=1"),Pass?TEXT("PASS"):TEXT("FAIL"),Role,Name);};
    auto Diagnose=[&](const TCHAR* Stage)
    {
        UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE %s teaching_stage=%s character=%s movement_class=%s mode=%d local_role=%d remote_role=%d grounded=%d max_walk=%.3f max_speed=%.3f jump_z=%.3f base_z=%.3f apex=%.3f"),Role,Stage,*Body->GetClass()->GetName(),*Move->GetClass()->GetName(),int32(Move->MovementMode),int32(Body->GetLocalRole()),int32(Body->GetRemoteRole()),Move->IsMovingOnGround(),Move->MaxWalkSpeed,Move->GetMaxSpeed(),Move->JumpZVelocity,P.BaseZ,P.Apex);
    };
    if(P.Started==0)
    {
        if(P.WaitingSince==0) P.WaitingSince=World->GetTimeSeconds();
        if(Move->MovementMode==MOVE_None || !Move->IsMovingOnGround())
        {
            if(World->GetTimeSeconds()-P.WaitingSince>=10) { Diagnose(TEXT("physical-ready-timeout")); Check(false,TEXT("teaching native pawn becomes physically ready within ten seconds")); P.Complete=true; }
            return;
        }
        P.Started=World->GetTimeSeconds(); P.BaseZ=Body->GetActorLocation().Z; Diagnose(TEXT("physical-ready"));
    }
    const double Time=World->GetTimeSeconds()-P.Started;
    auto At=[&](double Seconds,int32 Id,TFunction<void()> Action){if(Time>=Seconds&&!P.Done.Contains(Id)){P.Done.Add(Id);Action();}};
    At(.5,1,[&](){Diagnose(TEXT("parameter-assertion"));Check(!GS->bStructuredMatch&&(!CustomMove||(!CustomMove->UsesMovementRules()&&!CustomMove->UsesStamina()))&&!Body->GetWeapons()->IsEnabled(),TEXT("original teaching bypasses movement stamina and weapon rules"));Check(FMath::IsNearlyEqual(Move->GetMaxSpeed(),570.f)&&FMath::IsNearlyEqual(Move->JumpZVelocity,650.f),TEXT("original teaching retains 570 walking and 650 jumping"));});
    if(Time>=1&&Time<2)Body->MoveForward(FInputActionValue(1.f));
    At(1.7,2,[&](){Check(FMath::IsNearlyEqual(Move->Velocity.Size2D(),570.f,35.f),TEXT("original teaching native actual walking speed remains unchanged"));});
    At(2.5,3,[&](){Body->PressJump();});At(2.7,4,[&](){Body->ReleaseJump();Check(Move->IsFalling(),TEXT("original teaching still jumps through CharacterMovement"));});
    if(Move->IsFalling())P.Apex=FMath::Max(P.Apex,Body->GetActorLocation().Z-P.BaseZ);
    At(4.5,5,[&](){Diagnose(TEXT("trajectory-assertion"));Check(P.Apex>180&&P.Apex<240,TEXT("original teaching 650 actual jump trajectory remains unchanged"));});
    At(5.5,6,[&](){P.Complete=true;Check(!P.Failed,TEXT("COMPLETE"));});
#endif
}
