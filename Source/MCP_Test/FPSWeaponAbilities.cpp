#include "FPSWeaponAbilities.h"
#include "FPSDemoCharacter.h"
#include "FPSWeaponPickup.h"
#include "FPSWeaponCustomization.h"
#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_PickupRequest, "Input.Weapon.Pickup");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ReloadRequest, "Input.Weapon.Reload");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ModifyRequest, "Input.Weapon.Modify");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_MagazineRequest, "Input.Weapon.ModifyMagazine");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_CancelReloadRequest, "Input.Weapon.CancelReload");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Firing, "State.Weapon.Firing");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Reloading, "State.Weapon.Reloading");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Inspecting, "State.Weapon.Inspecting");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Switching, "State.Weapon.Switching");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Aiming, "State.Weapon.Aiming");

UFPSWeaponAbility::UFPSWeaponAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    bReplicateInputDirectly = true;
    NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnlyTermination;
}
UFPSWeaponComponent* UFPSWeaponAbility::Weapon() const
{
    auto* Pawn = Cast<AFPSDemoCharacter>(GetAvatarActorFromActorInfo());
    return Pawn ? Pawn->GetWeapons() : nullptr;
}
bool UFPSWeaponAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    const FGameplayTagContainer* Source, const FGameplayTagContainer* Target, FGameplayTagContainer* Relevant) const
{
    auto* Pawn = Info ? Cast<AFPSDemoCharacter>(Info->AvatarActor.Get()) : nullptr;
    return Pawn && Pawn->GetWeapons()->CanStart(Command) && Super::CanActivateAbility(Handle, Info, Source, Target, Relevant);
}
void UFPSWeaponAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* Event)
{
    Super::ActivateAbility(Handle, Info, ActivationInfo, Event);
    auto* W = Weapon();
    if (!W) return;
    auto Key = ActivationInfo.GetActivationPredictionKey();
    Ticket = W->OpenActivation(this, Command, Key.Current);
    if (!Info->IsNetAuthority() && Key.IsLocalClientKey())
        Key.NewRejectedDelegate().BindWeakLambda(W, [W, Activation = Ticket]() { W->RejectPrediction(Activation); });
}
bool UFPSWeaponAbility::IsCurrentActivation(const FGameplayAbilityActivationInfo& Info) const
{
    return IsActive() && Info.GetActivationPredictionKey() == CurrentActivationInfo.GetActivationPredictionKey();
}

void UFPSWeaponAbility::Complete(bool bCancelled)
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bCancelled);
}
void UFPSWeaponAbility::CancelWeaponAbility(bool bReplicate)
{
    if (IsActive()) CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicate);
}
UFPSFireAbility::UFPSFireAbility() { ActivationOwnedTags.AddTag(TAG_Firing); }
UFPSReloadAbility::UFPSReloadAbility()
{
    Command = EFPSWeaponRequest::Reload; ActivationOwnedTags.AddTag(TAG_Reloading);
    FAbilityTriggerData Trigger; Trigger.TriggerTag = TAG_ReloadRequest; Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent; AbilityTriggers.Add(Trigger);
}
UFPSModifyAttachmentAbility::UFPSModifyAttachmentAbility()
{
    Command = EFPSWeaponRequest::Modify;
    FAbilityTriggerData Trigger; Trigger.TriggerTag = TAG_ModifyRequest; Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent; AbilityTriggers.Add(Trigger);
}
UFPSModifyMagazineAbility::UFPSModifyMagazineAbility()
{
    Command = EFPSWeaponRequest::ModifyMagazine; ActivationOwnedTags.AddTag(TAG_Reloading);
    FAbilityTriggerData Trigger; Trigger.TriggerTag = TAG_MagazineRequest; Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent; AbilityTriggers.Add(Trigger);
}
UFPSCancelReloadAbility::UFPSCancelReloadAbility()
{
    Command = EFPSWeaponRequest::CancelReload;
    FAbilityTriggerData Trigger; Trigger.TriggerTag = TAG_CancelReloadRequest; Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent; AbilityTriggers.Add(Trigger);
}
void UFPSCancelReloadAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* Event)
{
    Super::ActivateAbility(Handle, Info, ActivationInfo, Event);
    auto* W = Weapon();
    Complete(!W || !W->CancelMatchingReload(Ticket, Event ? FFPSWeaponCustomizationRequest::Read(Event->TargetData) : nullptr));
}
UFPSInspectAbility::UFPSInspectAbility() { Command = EFPSWeaponRequest::Inspect; ActivationOwnedTags.AddTag(TAG_Inspecting); }
UFPSPrimaryAbility::UFPSPrimaryAbility() { Command = EFPSWeaponRequest::Primary; ActivationOwnedTags.AddTag(TAG_Switching); }
UFPSSecondaryAbility::UFPSSecondaryAbility() { Command = EFPSWeaponRequest::Secondary; ActivationOwnedTags.AddTag(TAG_Switching); }
UFPSEquipAbility::UFPSEquipAbility() { Command = EFPSWeaponRequest::Equip; NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated; ActivationOwnedTags.AddTag(TAG_Switching); }
UFPSAimAbility::UFPSAimAbility() { Command = EFPSWeaponRequest::AimOn; ActivationOwnedTags.AddTag(TAG_Aiming); }
UFPSPickupAbility::UFPSPickupAbility()
{
    Command = EFPSWeaponRequest::Pickup;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    FAbilityTriggerData Trigger; Trigger.TriggerTag = TAG_PickupRequest; Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(Trigger);
}
UFPSTransferWeaponAbility::UFPSTransferWeaponAbility() { NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly; }

void UFPSFireAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* Event)
{
    Super::ActivateAbility(Handle, Info, ActivationInfo, Event);
    auto* W = Weapon();
    if (!W || !W->Definition()) { Complete(true); return; }
    if (!W->BeginFire(Ticket, this)) { Complete(true); return; }
    bAutomaticIntent = W->Definition()->FireMode == EFPSFireMode::Automatic;
    Cycle = {Ticket.Instance, W->Definition()->FireMode, W->Definition()->BurstCount};
    ReadinessHandle = W->OnFireReadinessChanged.AddUObject(this, &UFPSFireAbility::Wake);
    Step();
}
void UFPSFireAbility::Wake()
{
    if (!IsActive() || bStepping || !Weapon() || Weapon()->IsShuttingDown()) return;
    // Defer until the atomic inventory/action transition is finished. Never shoot in Changed().
    if (auto* World = GetWorld()) World->GetTimerManager().ClearTimer(Timer);
    Timer = GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,
        [Activation = Ticket, this]() { if (IsActive() && Ticket == Activation) Step(); }));
}
void UFPSFireAbility::WaitUntil(double Deadline)
{
    auto* W = Weapon();
    if (!W) return;
    GetWorld()->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this,
        [Activation = Ticket, this]() { if (IsActive() && Ticket == Activation) Step(); }),
        FMath::Max(.001, Deadline - W->ServerTime()), false);
}
void UFPSFireAbility::Step()
{
    TGuardValue<bool> Guard(bStepping, true);
#if !UE_BUILD_SHIPPING
    ++StepCount;
#endif
    auto* W = Weapon();
    if (!W || !W->CanAct()) { Complete(true); return; }
    const auto* Def = W->Definition();
    if (!Def) { if (!bAutomaticIntent) Complete(true); return; }
    if (Cycle.Instance != W->Equipped().InstanceId)
    {
        if (!bAutomaticIntent || !W->RebindAutomaticFire(Ticket)) { Complete(true); return; }
        Cycle = {Ticket.Instance, Def->FireMode, Def->BurstCount};
    }
    // Holding an automatic trigger across a switch never starts a single/burst cycle.
    if (bAutomaticIntent && Def->FireMode != EFPSFireMode::Automatic) return;
    W->InterruptInspect();
    double Deadline = 0;
    const auto Readiness = W->FireReadiness(Ticket, Deadline);
    if (Readiness == EFPSFireReadiness::Invalid) { Complete(true); return; }
    if (Readiness == EFPSFireReadiness::WaitingForState)
    {
        if (!bAutomaticIntent) Complete();
        return;
    }
    if (Readiness == EFPSFireReadiness::WaitingForSprintPreparation)
    {
        WaitUntil(Deadline); return;
    }
    if (Readiness == EFPSFireReadiness::WaitingForDeadline)
    {
        if (!bAutomaticIntent && !Cycle.bHasShot && !W->HasPendingSprintFire()) { Complete(); return; }
        WaitUntil(Deadline); return;
    }
    if (!W->CommitShot(Ticket)) { if (!bAutomaticIntent) Complete(); return; }
    Cycle.bHasShot = true;
    if (!bAutomaticIntent && (Cycle.Mode == EFPSFireMode::Single || --Cycle.Remaining <= 0 || W->Equipped().Magazine <= 0))
    {
        if (Cycle.Mode == EFPSFireMode::Burst) W->FinishBurst(Ticket);
        Complete(); return;
    }
    // Empty magazines and state blockers sleep until an explicit readiness notification.
    if (W->Equipped().Magazine > 0) WaitUntil(W->Equipped().NextShotAt);
}
void UFPSFireAbility::InputReleased(const FGameplayAbilitySpecHandle, const FGameplayAbilityActorInfo*, const FGameplayAbilityActivationInfo Info)
{
    if (IsCurrentActivation(Info) && bAutomaticIntent) Complete();
}
void UFPSFireAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicate, bool bCancelled)
{
    if (!IsCurrentActivation(ActivationInfo)) return;
    if (auto* World = GetWorld()) World->GetTimerManager().ClearTimer(Timer);
    if (auto* W = Weapon()) { W->OnFireReadinessChanged.Remove(ReadinessHandle); W->EndFire(Ticket, bCancelled); }
    Super::EndAbility(Handle, Info, ActivationInfo, bReplicate, bCancelled);
}
void UFPSTimedWeaponAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* Event)
{
    Super::ActivateAbility(Handle, Info, ActivationInfo, Event);
    auto* W = Weapon();
    if (!W || !W->Definition()) { Complete(true); return; }
    float Duration = 0;
    const auto* Request = Event ? FFPSWeaponCustomizationRequest::Read(Event->TargetData) : nullptr;
    const bool Modifying = Command == EFPSWeaponRequest::Modify || Command == EFPSWeaponRequest::ModifyMagazine;
    if (!(Modifying ? W->BeginModification(Ticket, this, Request, Duration) : W->BeginAction(Ticket, this, Duration, Request))) { Complete(true); return; }
    if (Duration <= 0) Elapsed();
    else GetWorld()->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [Action = Ticket, this]() { if (IsActive() && Ticket == Action) Elapsed(); }), Duration, false);
}
void UFPSTimedWeaponAbility::Elapsed() { Complete(); }
void UFPSTimedWeaponAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicate, bool bCancelled)
{
    if (!IsCurrentActivation(ActivationInfo)) return;
    if (auto* World = GetWorld()) World->GetTimerManager().ClearTimer(Timer);
    if (auto* W = Weapon(); W && !W->FinishAction(Ticket, bCancelled)) W->FinishAction(Ticket, true);
    Super::EndAbility(Handle, Info, ActivationInfo, bReplicate, bCancelled);
}
void UFPSAimAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* Event)
{
    Super::ActivateAbility(Handle, Info, ActivationInfo, Event);
    if (auto* W = Weapon(); !W || !W->BeginAim(Ticket, this)) Complete(true);
}
void UFPSAimAbility::InputReleased(const FGameplayAbilitySpecHandle, const FGameplayAbilityActorInfo*, const FGameplayAbilityActivationInfo) { Complete(); }
void UFPSAimAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicate, bool bCancelled)
{
    if (!IsCurrentActivation(ActivationInfo)) return;
    if (auto* W = Weapon()) W->EndAim(Ticket);
    Super::EndAbility(Handle, Info, ActivationInfo, bReplicate, bCancelled);
}
void UFPSTransferWeaponAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* Event)
{
    Super::ActivateAbility(Handle, Info, ActivationInfo, Event);
    auto* W = Weapon();
    if (!Info->IsNetAuthority()) { Complete(); return; }
    const FGuid Previous = W ? W->Equipped().InstanceId : FGuid();
    const bool bSuccess = W && W->Transfer(Ticket, Event ? Cast<AFPSWeaponPickup>(const_cast<UObject*>(Event->OptionalObject.Get())) : nullptr);
    if (bSuccess && W->Equipped().IsValid() && W->Equipped().InstanceId != Previous)
        Info->AbilitySystemComponent->TryActivateAbilityByClass(UFPSEquipAbility::StaticClass());
    Complete(!bSuccess);
}

void UFPSWeaponAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicate, bool bCancelled)
{
    if (!IsCurrentActivation(ActivationInfo)) return;
    if (!Info->IsNetAuthority() && ActivationInfo.ActivationMode == EGameplayAbilityActivationMode::Rejected)
        if (auto* W = Weapon()) W->RejectPrediction(Ticket);
    Super::EndAbility(Handle, Info, ActivationInfo, bReplicate, bCancelled);
}
UFPSCancelWeaponAbility::UFPSCancelWeaponAbility()
{
    Command = EFPSWeaponRequest::Cancel;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}
void UFPSCancelWeaponAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* Event)
{
    Super::ActivateAbility(Handle, Info, ActivationInfo, Event);
    if (auto* W = Weapon()) W->StopAllActions();
    Complete();
}
