#include "FPSWeaponAbilitySmoke.h"
#include "FPSDemoEffects.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponAbilities.h"
#include "AbilitySystemComponent.h"
#include "FPSWeaponPickup.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSMatchFlow.h"
#include "FPSMovementComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void FFPSWeaponAbilitySmoke::Tick(UFPSWeaponComponent* W)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSWeaponAbilitySmoke")) || !W->IsEnabled()) return;
    auto* Pawn = W->Character();
    const bool Server = Pawn->HasAuthority();
    if (Server == Pawn->IsLocallyControlled()) return;
    const auto* GS = Pawn->GetWorld()->GetGameState<AFPSDemoGameState>();
    if (!GS || GS->bWaiting || GS->bFinished) return;
    const double Time = W->ServerTime() - (GS->RoundEndsAt - 300);
    struct FProgress { TSet<int32> Done; bool Failed = false; FGuid Id; double Cooldown = 0; int32 Recoil = 0; int32 DeathAmmo = 0; uint32 BeforeDeathSerial = 0; FFPSWeaponActionTicket OldAction, OldAim, OldFire; uint32 SleepingSteps = 0; int32 SecondaryAmmo = 0;
        double SprintFireDeadline = 0; uint32 SprintFireSerial = 0;
        bool SprintPreparationObserved = false, SprintFirstShotObserved = false, SprintEarlyShotReported = false; };
    static TMap<TWeakObjectPtr<UFPSWeaponComponent>, FProgress> Progress;
    // A real GAS death now destroys this fixture body and respawns it after 2.5 seconds.
    // A replacement body must not replay the old life's timed fixture.
    if (Time > 50 && !Progress.Contains(W)) return;
    auto& P = Progress.FindOrAdd(W);
    auto Check = [&](bool Pass, const TCHAR* What)
    {
        P.Failed |= !Pass;
        UE_LOG(LogTemp, Display, TEXT("FPS_ABILITY_SMOKE %s %s %s mag=%d reserve=%d recoil=%d action=%d ads=%d"), Pass ? TEXT("PASS") : TEXT("FAIL"), Server ? TEXT("SERVER") : TEXT("CLIENT"), What, W->Equipped().Magazine, W->Equipped().Reserve, W->Equipped().RecoilIndex, int32(W->Loadout.Action), W->Loadout.bAiming);
    };
    auto At = [&](int32 Tenth, TFunction<void()> Action)
    {
        if (Time >= Tenth * .1 && !P.Done.Contains(Tenth)) { P.Done.Add(Tenth); Action(); }
    };
    auto* Def = const_cast<UFPSWeaponDefinition*>(W->Catalog()->Find(TEXT("Weapon.M4")));
    At(0, [&]()
    {
        // Process-local fixture only: no seventh asset and no package writes.
        Def->FireMode = EFPSFireMode::Burst; Def->FireInterval = .12f; Def->BurstInterval = .55f;
        Def->RecoilRecoveryTime = 1.5f; Def->ReloadDuration = 1.f; Def->EquipReadyTime = .4f; Def->EquipBlendTime = .1f;
        Check(Pawn->GetAbilitySystemComponent()->GetActivatableAbilities().Num() >= 10, TEXT("shared GAS abilities granted"));
        if (Server)
        {
            W->Loadout.Primary.Magazine = 10; W->Loadout.Primary.Reserve = 20;
            Pawn->SetActorLocation(FVector(400,0,100)); W->Changed();
        }
        else { W->LocalShotPresentations = 0; Pawn->GetController()->SetControlRotation(FRotator(60,90,0)); }
    });
    // Capture observed activations, not a wall-clock instant that can precede RPC arrival.
    if (!P.OldFire.Sequence && Time < 2 && W->FireTicket.Sequence) P.OldFire = W->FireTicket;
    if (Time > 3.3 && Time < 4)
    {
        if (!P.OldAction.Sequence && W->ActionTicket.Sequence) P.OldAction = W->ActionTicket;
        if (!P.OldAim.Sequence && W->AimTicket.Sequence) P.OldAim = W->AimTicket;
    }
    At(35, [&]()
    {
        const auto Current = W->ActionTicket;
        const auto Before = W->GetLoadout();
        auto WrongInstance = Current; WrongInstance.Instance = FGuid::NewGuid();
        Check(Current.Sequence && !W->FinishAction(Current, false) && !W->FinishAction(WrongInstance, true) &&
            W->GetLoadout().Action == Before.Action && W->Equipped().Magazine == Before.Primary.Magazine,
            TEXT("R1 early completion and wrong-instance cancellation rejected"));
    });
    At(61, [&]()
    {
        const bool Aiming = W->GetLoadout().bAiming;
        Check(P.OldAim.Sequence && W->AimTicket.Sequence != P.OldAim.Sequence && !W->EndAim(P.OldAim) && W->GetLoadout().bAiming == Aiming,
            TEXT("R1 old ADS end cannot cancel a later activation"));
    });
    At(79, [&]()
    {
        const auto Current = W->ActionTicket;
        const auto Before = W->GetLoadout();
        Check(P.OldAction.Sequence && Current.Sequence != P.OldAction.Sequence && Current.Instance == P.OldAction.Instance &&
            !W->FinishAction(P.OldAction, false) && !W->FinishAction(P.OldAction, true) &&
            W->GetLoadout().Action == Before.Action && W->Equipped().Magazine == Before.Primary.Magazine,
            TEXT("R1 stale same-instance reload completion/cancel cannot affect next reload"));
        if (!Server)
        {
            W->RejectPrediction(P.OldAction);
            Check(W->ActionTicket == Current && W->GetLoadout().Action == Before.Action && W->GetLoadout().bAiming == Before.bAiming,
                TEXT("R1 delayed prediction rejection preserves later operation"));
        }
    });
    At(102, [&]()
    {
        const auto Before = W->Equipped();
        Check(P.OldFire.Sequence && !W->CommitShot(P.OldFire) && !W->FinishBurst(P.OldFire) && !W->EndFire(P.OldFire, true) &&
            W->Equipped().Magazine == Before.Magazine && W->Equipped().NextShotAt == Before.NextShotAt,
            TEXT("R1 stale fire ticket cannot shoot or alter cooldown"));
    });
    At(150, [&]() { Def->FireMode = EFPSFireMode::Automatic; });
    At(250, [&]() { Def->FireMode = EFPSFireMode::Burst; Def->RecoilRecoveryTime = 10.f; });
    At(348, [&]() { Def->EquipReadyTime = 0; Def->EquipBlendTime = 1.f; Def->RecoilRecoveryTime = .2f; });
    At(360, [&]() { Def->FireMode = EFPSFireMode::Automatic; });
    At(540, [&]() { Def->FireMode = EFPSFireMode::Burst; });
    auto Steps = [&]() { const auto* Fire = Cast<UFPSFireAbility>(W->FireAbility); return Fire ? Fire->GetStepCount() : 0u; };
    auto* Movement = CastChecked<UFPSMovementComponent>(Pawn->GetCharacterMovement());
    if (Time >= 50.6 && Time < 54)
    {
        if (Server && !P.SprintPreparationObserved && W->HasPendingSprintFire())
        {
            P.SprintPreparationObserved = true; P.SprintFireDeadline = W->GetSprintFireReadyAt();
            Check(W->Equipped().Magazine == 5 && W->Equipped().ShotSerial == P.SprintFireSerial &&
                !Movement->HasSprintIntent() && !Movement->IsSprinting() && P.SprintFireDeadline > W->ServerTime(),
                TEXT("R2 authority observes sprint interruption and unspent preparation"));
            UE_LOG(LogTemp, Display, TEXT("FPS_ABILITY_SPRINT_SAMPLE SERVER prepare now=%.6f deadline=%.6f serial=%u"),
                W->ServerTime(), P.SprintFireDeadline, P.SprintFireSerial);
        }
        if (P.SprintPreparationObserved && W->ServerTime() < P.SprintFireDeadline &&
            (W->Equipped().Magazine != 5 || W->Equipped().ShotSerial != P.SprintFireSerial) && !P.SprintEarlyShotReported)
        {
            P.SprintEarlyShotReported = true;
            Check(false, TEXT("R2 no shot may be committed before sprint preparation deadline"));
        }
        if (P.SprintPreparationObserved && !P.SprintFirstShotObserved && W->Equipped().ShotSerial > P.SprintFireSerial)
        {
            P.SprintFirstShotObserved = true;
            Check(W->Equipped().LastShotAt >= P.SprintFireDeadline && !Movement->HasSprintIntent() && !Movement->IsSprinting(),
                TEXT("R2 first automatic shot respects sprint preparation and does not restore sprint"));
            UE_LOG(LogTemp, Display, TEXT("FPS_ABILITY_SPRINT_SAMPLE %s first-shot last=%.6f deadline=%.6f serial=%u"),
                Server ? TEXT("SERVER") : TEXT("CLIENT"), W->Equipped().LastShotAt, P.SprintFireDeadline, W->Equipped().ShotSerial);
        }
    }
    At(380, [&]() { P.SleepingSteps = Steps(); Check(W->FireAbility != nullptr, TEXT("R2 empty automatic retains held input")); });
    At(400, [&]() { Check(W->FireAbility && Steps() <= P.SleepingSteps + 2 && W->Equipped().Magazine == 0, *FString::Printf(TEXT("R2 empty hold sleeps without periodic fire retry steps=%u"), Steps() - P.SleepingSteps)); });
    At(425, [&]() { Check(W->Equipped().Magazine == 0 && W->Equipped().Reserve == 0, TEXT("R2 reload event wakes held empty automatic")); });
    At(450, [&]() { P.SecondaryAmmo = W->Equipped().Magazine; P.SleepingSteps = Steps(); });
    At(465, [&]() { Check(W->Equipped().WeaponId == FName(TEXT("Weapon.M9")) && W->Equipped().Magazine == P.SecondaryAmmo && W->FireAbility && Steps() <= P.SleepingSteps + 2, *FString::Printf(TEXT("R2 held automatic sleeps on single-fire weapon without firing steps=%u"), Steps() - P.SleepingSteps)); });
    At(490, [&]() { Check(W->Loadout.Primary.Magazine == 0, TEXT("R2 switching back wakes automatic without new press")); });
    At(525, [&]() { Check(P.SprintPreparationObserved && P.SprintFirstShotObserved && W->Equipped().Magazine == 0 &&
        !Movement->HasSprintIntent() && !Movement->IsSprinting(), TEXT("R2 sprint preparation completes into held automatic without another sprint release")); });
    if (Server)
    {
        At(500, [&]() { Check(!W->FireAbility, TEXT("R2 prior trigger ended before sprint fixture refill")); W->EquippedMutable().Magazine = 5; P.SprintFireSerial = W->Equipped().ShotSerial; W->Changed(); });
        At(505, [&]() { Check(Movement->IsSprinting() && Pawn->GetVelocity().Size2D() > 600,
            TEXT("R2 actual forward sprint reaches authority before fire")); });
        At(361, [&]() { W->EquippedMutable().Magazine = 0; W->EquippedMutable().Reserve = 3; W->Changed(); });
        At(435, [&]() { W->EquippedMutable().Magazine = 5; W->Changed(); });
    }
    else
    {
        At(498, [&]()
        {
            Pawn->GetController()->SetControlRotation(FRotator(60,-90,0));
            Pawn->SetActorRotation(FRotator(0,-90,0));
            Movement->RequestSprint(true);
        });
        // Use the existing clear southern corridor and CMC RPCs, not stationary intent injection.
        if (Time >= 49.8 && Time < 51.2) Pawn->AddMovementInput(FVector(0,-1,0),1);
        At(506, [&]()
        {
            Check(!W->FireAbility && W->Equipped().Magazine == 5 && Movement->IsSprinting() && Pawn->GetVelocity().Size2D() > 600,
                TEXT("R2 actual sprint and refill ready before new trigger"));
            P.SprintFireSerial = W->Equipped().ShotSerial;
            const double PressedAt = W->ServerTime();
            W->StartFire();
            P.SprintFireDeadline = W->GetSprintFireReadyAt(); P.SprintPreparationObserved = W->HasPendingSprintFire();
            Check(P.SprintPreparationObserved && !Movement->HasSprintIntent() && !Movement->IsSprinting() &&
                W->Equipped().Magazine == 5 && W->Equipped().ShotSerial == P.SprintFireSerial &&
                P.SprintFireDeadline - PressedAt >= Movement->GetSprintFirePreparationSeconds() - .001,
                TEXT("R2 fire immediately interrupts sprint and preserves configured preparation"));
            UE_LOG(LogTemp, Display, TEXT("FPS_ABILITY_SPRINT_SAMPLE CLIENT prepare now=%.6f deadline=%.6f serial=%u"),
                PressedAt, P.SprintFireDeadline, P.SprintFireSerial);
        });
        At(530, [&]() { W->StopFire(); });
        At(365, [&]() { W->StartFire(); });
        At(401, [&]()
        {
            const auto Before = W->GetLoadout();
            W->Request(EFPSWeaponRequest::Equip);
            Check(W->GetLoadout().Action == Before.Action, TEXT("R2 internal Equip cannot be invoked as player input"));
        });
        At(405, [&]() { W->Reload(); });
        At(430, [&]() { W->StopFire(); });
        At(440, [&]() { W->StartFire(); W->Secondary(); });
        At(470, [&]() { W->Primary(); });
        At(495, [&]() { W->StopFire(); });
    }
    if (Server)
    {
        At(20, [&]() { Check(W->Equipped().Magazine == 7 && W->Equipped().RecoilIndex == 3, TEXT("release completes burst; busy/cooldown clicks not queued")); });
        At(31, [&]() { Check(W->Equipped().Magazine == 4 && W->Equipped().RecoilIndex == 6, TEXT("fresh press starts burst; recoil survives quick release")); });
        At(39, [&]() { Check(W->Loadout.Action == EFPSWeaponAction::Reloading && W->Loadout.bAiming && W->Equipped().Magazine == 4, TEXT("reload and ADS coexist; ammo not committed early")); });
        At(45, [&]() { Check(W->Equipped().Magazine == 24 && W->Equipped().Reserve == 0 && W->Loadout.bAiming, TEXT("magazine reload commits exactly once and retains ADS")); });
        At(54, [&]() { Check(W->Equipped().WeaponId == FName(TEXT("Weapon.M9")) && W->Equipped().Magazine == 15 && !W->Loadout.bAiming && W->Loadout.Action == EFPSWeaponAction::Idle, TEXT("switch readiness blocks fire/ADS; repeated slot does not extend it")); });
        At(60, [&]() { Check(W->Loadout.bAiming, TEXT("release/repress restores ADS")); });
        At(70, [&]() { Check(!W->Loadout.bAiming, TEXT("held RMB cannot resume ADS after switch")); });
        At(71, [&]() { W->Loadout.Primary.Magazine = 21; W->Loadout.Primary.Reserve = 9; W->Changed(); });
        At(78, [&]() { Check(W->Loadout.Action == EFPSWeaponAction::Reloading && !W->Loadout.bAiming, TEXT("reload preference exits ADS once")); });
        At(83, [&]() { Check(W->Loadout.Action == EFPSWeaponAction::Reloading && W->Loadout.bAiming, TEXT("ADS can restart during reload after release")); });
        At(90, [&]() { Check(W->Equipped().Magazine == 30 && W->Loadout.bAiming, TEXT("reload preference does not force ADS off repeatedly")); W->EquippedMutable().Magazine = 2; W->Changed(); });
        At(106, [&]() { Check(W->Equipped().Magazine == 0, TEXT("short magazine fires only remaining burst rounds")); });
        At(110, [&]() { W->EquippedMutable().Magazine = 10; W->EquippedMutable().Reserve = 20; W->Changed(); });
        At(125, [&]() { Check(W->Loadout.Primary.Magazine == 9, TEXT("valid switch interrupts unfinished burst")); });
        At(141, [&]() { Check(W->Loadout.Primary.Magazine == 8 && W->Loadout.Action == EFPSWeaponAction::Reloading, TEXT("reload interrupts unfinished burst without catch-up shots")); });
        At(149, [&]() { Check(W->Equipped().Magazine == 28 && W->Equipped().Reserve == 0, TEXT("cancelled burst reload preserves ammunition")); });
        At(151, [&]() { W->EquippedMutable().Magazine = 0; W->EquippedMutable().Reserve = 5; W->Changed(); });
        At(180, [&]() { Check(W->Equipped().Magazine == 0 && W->Equipped().Reserve == 0, TEXT("held automatic trigger resumes after reload")); });
        At(182, [&]() { W->EquippedMutable().Magazine = 5; W->Changed(); });
        At(209, [&]() { Check(W->Loadout.Primary.Magazine == 0, TEXT("automatic press during preparation resumes when ready")); });
        At(211, [&]() { W->Loadout.Secondary = FFPSWeaponInstance(); W->EquippedMutable().Magazine = 5; W->EquippedMutable().Reserve = 5; W->Changed(); });
        At(224, [&]() { Check(W->Loadout.Action == EFPSWeaponAction::Reloading && W->Equipped().Magazine == 5, TEXT("empty slot and current slot requests cannot cancel reload")); });
        At(238, [&]() { Check(W->Loadout.Action == EFPSWeaponAction::Idle && !W->Loadout.bAiming && !W->FireAbility && !W->ActionAbility && !W->AimAbility, TEXT("menu cancels server operations and intent")); });
        At(252, [&]() { W->Loadout.Secondary = W->Catalog()->CreateInstance(TEXT("Weapon.M9")); W->EquippedMutable().Magazine = 10; W->Changed(); });
        At(269, [&]() { P.Id = W->Equipped().InstanceId; P.Cooldown = W->Equipped().NextShotAt; P.Recoil = W->Equipped().RecoilIndex; });
        At(281, [&]() { Check(!W->Loadout.Primary.IsValid(), TEXT("drop relinquishes instance")); });
        At(282, [&]()
        {
            for (TActorIterator<AFPSWeaponPickup> It(Pawn->GetWorld()); It; ++It)
                if (It->GetInstance().InstanceId == P.Id)
                    Pawn->SetActorLocation(It->GetActorLocation() + FVector(0,0,90), false, nullptr, ETeleportType::TeleportPhysics);
        });
        At(296, [&]() { Check(W->Loadout.Primary.InstanceId == P.Id && W->Loadout.Primary.NextShotAt == P.Cooldown && W->Loadout.Primary.RecoilIndex == P.Recoil, TEXT("drop/pickup retains cooldown and recoil state")); });
        At(307, [&]() { Check(W->Equipped().RecoilIndex == P.Recoil + 3, TEXT("pickup does not reset recoil before recovery wait")); });
        At(320, [&]() { W->EquippedMutable().NextShotAt = W->ServerTime() + 2; W->Changed(); });
        At(335, [&]() { Check(W->Equipped().Magazine == 4, TEXT("authority rejects falsified local cooldown")); });
        At(357, [&]() { Check(W->Equipped().Magazine == 1 && W->Loadout.Action == EFPSWeaponAction::Idle, TEXT("zero readiness permits shots during independent visual blend")); });
        At(541, [&]() { W->EquippedMutable().Magazine = 10; P.BeforeDeathSerial = W->Equipped().ShotSerial; W->Changed(); });
        if (Time >= 55 && W->Equipped().ShotSerial > P.BeforeDeathSerial && !Pawn->IsEliminated())
        {
            Check(W->Equipped().RecoilIndex == 1, TEXT("recoil resets only after recovery wait"));
            auto* ASC = Pawn->GetAbilitySystemComponent();
            const auto Spec = ASC->MakeOutgoingSpec(UFPSDemoDamageEffect::StaticClass(), 1, ASC->MakeEffectContext());
            for (int32 Hit = 0; Hit < 4; ++Hit) ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
            P.DeathAmmo = W->Equipped().Magazine;
        }
        At(565, [&]() { Check(Pawn->IsEliminated() && W->Equipped().Magazine == P.DeathAmmo && !W->FireAbility && !W->ActionAbility && !W->AimAbility, TEXT("death cancels remaining burst and abilities")); });
    }
    else
    {
        At(10, [&]() { W->StartFire(); Check(W->Equipped().Magazine == 9, TEXT("first burst round predicted immediately")); W->StopFire(); });
        At(12, [&]() { W->StartFire(); W->StopFire(); });
        At(15, [&]() { W->Inspect(); W->StartFire(); W->StopFire(); });
        At(19, [&]() { Check(W->LocalShotPresentations == 3, TEXT("all burst rounds predicted locally despite delayed authority snapshots")); });
        At(22, [&]() { Check(W->Equipped().Magazine == 7, TEXT("owner burst ammo converges")); });
        At(24, [&]() { W->StartFire(); W->StopFire(); });
        At(33, [&]() { W->Reload(); W->AimOn(); Check(W->Loadout.bAiming && W->Loadout.Action == EFPSWeaponAction::Reloading, TEXT("reload and ADS predicted together")); });
        At(46, [&]() { W->Secondary(); Check(W->Equipped().WeaponId == FName(TEXT("Weapon.M9")) && !W->Loadout.bAiming, TEXT("switch identity immediately predicted")); });
        At(47, [&]() { W->Secondary(); });
        At(48, [&]() { W->StartFire(); W->StopFire(); W->AimOn(); });
        At(56, [&]() { W->AimOff(); W->AimOn(); });
        At(62, [&]() { W->Primary(); });
        At(69, [&]() { W->AimOn(); });
        At(72, [&]() { W->AimOff(); W->AimOn(); W->SetReloadExitsAim(true); });
        At(74, [&]() { W->Reload(); });
        At(77, [&]() { W->AimOn(); });
        At(80, [&]() { W->AimOff(); W->AimOn(); });
        At(97, [&]() { W->StartFire(); W->StopFire(); });
        At(117, [&]() { W->StartFire(); W->StopFire(); });
        At(118, [&]() { W->Secondary(); });
        At(127, [&]() { W->Primary(); });
        At(135, [&]() { W->StartFire(); W->StopFire(); });
        At(136, [&]() { W->Reload(); });
        At(157, [&]() { W->StartFire(); W->Reload(); });
        At(175, [&]() { W->StopFire(); });
        At(188, [&]() { W->Secondary(); });
        At(194, [&]() { W->Primary(); W->StartFire(); });
        At(206, [&]() { W->StopFire(); });
        At(217, [&]() { W->Reload(); });
        At(218, [&]() { W->Secondary(); W->Primary(); });
        At(232, [&]() { W->AimOff(); W->AimOn(); W->Inspect(); W->CancelForMenu(); });
        At(245, [&]()
        {
            auto* ASC = Pawn->GetAbilitySystemComponent();
            Check(!ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Weapon.Aiming"))) && !ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Weapon.Reloading"))), TEXT("cancel removes owned GAS tags"));
            bool Hidden = true;
            for (TActorIterator<AFPSDemoCharacter> It(W->GetWorld()); It; ++It)
                if (*It != Pawn) Hidden &= It->GetWeapons()->GetLoadout().Secondary.WeaponId.IsNone() && It->GetWeapons()->Equipped().Reserve == 0 && It->GetWeapons()->Equipped().Magazine == 0;
            Check(Hidden, TEXT("remote snapshot hides inventory and ammunition"));
        });
        At(261, [&]() { W->StartFire(); W->StopFire(); });
        At(273, [&]() { W->Drop(); });
        At(286, [&]() { W->Pickup(); });
        At(293, [&]() { W->Primary(); }); // Stored-slot pickup no longer switches the held weapon.
        At(300, [&]() { W->StartFire(); W->StopFire(); });
        At(327, [&]() { W->EquippedMutable().NextShotAt = 0; W->StartFire(); W->StopFire(); });
        At(340, [&]() { Check(W->Equipped().Magazine == 4 && !W->FireAbility, TEXT("GAS rejection rolls back predicted ammunition")); });
        At(346, [&]() { W->AimOff(); W->AimOn(); W->CancelForMenu(); });
        At(347, [&]()
        {
            Check(W->Loadout.Action == EFPSWeaponAction::Idle && !W->Loadout.bAiming && !W->FireAbility && !W->ActionAbility && !W->AimAbility, TEXT("owner converges with no residual action or ADS"));

        });
        At(349, [&]() { W->Secondary(); W->Primary(); });
        At(350, [&]() { W->StartFire(); W->StopFire(); });
        At(551, [&]() { W->StartFire(); W->StopFire(); });
        At(566, [&]() { Check(Pawn->IsEliminated() && !W->Loadout.bAiming && !W->FireAbility && !W->bTriggerHeld && !W->bAimHeld, TEXT("death clears owner intent and predicted abilities")); });
    }
    At(568, [&]() { if (!Server && FParse::Param(FCommandLine::Get(), TEXT("FPSWeaponCapture"))) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/WindowsEditor/WeaponAbilities.png"), false, false); });
    At(570, [&]() { Check(!P.Failed, TEXT("COMPLETE")); });
#endif
}
