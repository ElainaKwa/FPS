#include "FPSWeaponComponent.h"
#include "FPSWeaponShot.h"
#include "FPSMatchFlow.h"
#include "FPSWeaponAbilities.h"
#include "FPSAudioSubsystem.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoAttributeSet.h"
#include "FPSWeaponPickup.h"
#include "FPSMovementComponent.h"
#include "FPSWeaponSmoke.h"
#include "FPSWeaponAbilitySmoke.h"
#include "FPSWeaponDamageSmoke.h"
#include "FPSWeaponPresentationSmoke.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

UFPSWeaponComponent::UFPSWeaponComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
}
AFPSDemoCharacter* UFPSWeaponComponent::Character() const { return Cast<AFPSDemoCharacter>(GetOwner()); }
const UFPSWeaponCatalog* UFPSWeaponComponent::Catalog() const { return GetWorld()->GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>(); }
const FFPSWeaponInstance& UFPSWeaponComponent::Equipped() const { return Loadout.ActiveSlot == EFPSWeaponSlot::Primary ? Loadout.Primary : Loadout.Secondary; }
FFPSWeaponInstance& UFPSWeaponComponent::EquippedMutable() { return Loadout.ActiveSlot == EFPSWeaponSlot::Primary ? Loadout.Primary : Loadout.Secondary; }
const UFPSWeaponDefinition* UFPSWeaponComponent::Definition() const { return Catalog()->Find(Equipped().WeaponId); }
double UFPSWeaponComponent::ServerTime() const
{
    const auto* GS = GetWorld()->GetGameState();
    return GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}
bool UFPSWeaponComponent::IsEnabled() const
{
    const auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
    return GS && GS->bStructuredMatch;
}
bool UFPSWeaponComponent::CanAct() const
{
    const auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
    return IsEnabled() && Character() && !Character()->IsEliminated() && Character()->GetController() &&
        (!Cast<AFPSFrontController>(Character()->GetController()) || Cast<AFPSFrontController>(Character()->GetController())->IsWeaponContentVerified()) &&
        GS && GS->WeaponDataError.IsEmpty() && Catalog()->IsReady() && Catalog()->ValidationError.IsEmpty() && !GS->bWaiting && !GS->bFinished &&
        (GS->bTraining || GS->RoundEndsAt <= 0 || ServerTime() < GS->RoundEndsAt);
}
bool UFPSWeaponComponent::CanSprint() const { return CanAct() && !Loadout.bAiming && Loadout.Action == EFPSWeaponAction::Idle; }
bool UFPSWeaponComponent::CanStart(EFPSWeaponRequest Command) const
{
    if (Command == EFPSWeaponRequest::Cancel) return IsEnabled();
    if (!CanAct()) return false;
    const auto* Def = Definition();
    switch (Command)
    {
    case EFPSWeaponRequest::Primary: return Loadout.ActiveSlot != EFPSWeaponSlot::Primary && Loadout.Primary.IsValid();
    case EFPSWeaponRequest::Secondary: return Loadout.ActiveSlot != EFPSWeaponSlot::Secondary && Loadout.Secondary.IsValid();
    case EFPSWeaponRequest::Pickup: return FindPickup() != nullptr;
    case EFPSWeaponRequest::Drop: return Equipped().IsValid();
    case EFPSWeaponRequest::Cancel: return true;
    case EFPSWeaponRequest::AimOff: return false; // input release is not an Ability activation
    case EFPSWeaponRequest::Equip: return true; // internal server-initiated preparation
    case EFPSWeaponRequest::AimOn: return Def && Loadout.Action != EFPSWeaponAction::Switching;
    case EFPSWeaponRequest::Reload: return Def && Equipped().Magazine < Def->MagazineSize && Equipped().Reserve > 0 && Loadout.Action != EFPSWeaponAction::Reloading && Loadout.Action != EFPSWeaponAction::Switching;
    case EFPSWeaponRequest::Inspect: return Def && Loadout.Action == EFPSWeaponAction::Idle;
    case EFPSWeaponRequest::Fire: return Def && (Def->FireMode == EFPSFireMode::Automatic || Loadout.Action == EFPSWeaponAction::Inspecting || (Equipped().Magazine > 0 && ServerTime() + .002 >= Equipped().NextShotAt && Loadout.Action != EFPSWeaponAction::Reloading && Loadout.Action != EFPSWeaponAction::Switching));
    }
    return false;
}
void UFPSWeaponComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner()->HasAuthority() && IsEnabled())
    {
        Loadout.Primary = Catalog()->CreateInstance(Catalog()->StartingPrimary);
        Loadout.Secondary = Catalog()->CreateInstance(Catalog()->StartingSecondary);
        Changed();
        GrantAbilities();
    }
}
void UFPSWeaponComponent::GrantAbilities()
{
    if (bAbilitiesGranted || !GetOwner()->HasAuthority() || !IsEnabled()) return;
    auto* ASC = Character()->GetAbilitySystemComponent();
    const TSubclassOf<UFPSWeaponAbility> Classes[] = {UFPSFireAbility::StaticClass(), UFPSReloadAbility::StaticClass(), UFPSInspectAbility::StaticClass(),
        UFPSPrimaryAbility::StaticClass(), UFPSSecondaryAbility::StaticClass(), UFPSDropAbility::StaticClass(), UFPSPickupAbility::StaticClass(), UFPSAimAbility::StaticClass(), UFPSEquipAbility::StaticClass(), UFPSCancelWeaponAbility::StaticClass()};
    for (const auto Class : Classes)
        ASC->GiveAbility(FGameplayAbilitySpec(Class, 1, Class.GetDefaultObject()->Command == EFPSWeaponRequest::Equip ? INDEX_NONE : int32(Class.GetDefaultObject()->Command), this));
    bAbilitiesGranted = true;
}
void UFPSWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UFPSWeaponComponent, OwnerState, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UFPSWeaponComponent, VisibleState, COND_SkipOwner);
    DOREPLIFETIME_CONDITION(UFPSWeaponComponent, bReloadExitsAim, COND_OwnerOnly);
}
void UFPSWeaponComponent::Changed()
{
    if (GetOwner()->HasAuthority())
    {
        OwnerState = Loadout;
        VisibleState.WeaponId = Equipped().WeaponId; VisibleState.InstanceId = Equipped().InstanceId;
        VisibleState.Action = Loadout.Action; VisibleState.ActionEndsAt = Loadout.ActionEndsAt;
        VisibleState.bAiming = Loadout.bAiming; VisibleState.EquipStartedAt = Loadout.EquipStartedAt;
        if (auto* ASC = Character()->GetAbilitySystemComponent()) ASC->SetNumericAttributeBase(UFPSDemoAttributeSet::GetWeaponDamageAttribute(), Definition() ? Definition()->Damage : 0);
        GetOwner()->ForceNetUpdate();
    }
    RefreshPresentation();
    OnFireReadinessChanged.Broadcast();
}
void UFPSWeaponComponent::OnRep_OwnerState()
{
    Loadout = OwnerState;
    // Keep unacknowledged shot presentation on its local cadence. Final authority acknowledgement
    // (including an interrupted trigger) always wins; reload ammo is accepted during a held trigger.
    if (OwnerState.FirePredictionKey == LastPredictedFireKey && !OwnerState.bFireActive) PredictedShots.Empty();
    for (auto* Gun : {&Loadout.Primary, &Loadout.Secondary})
    {
        if (auto* Predicted = PredictedShots.Find(Gun->InstanceId))
        {
            if (Gun->ShotSerial <= Predicted->ShotSerial)
            {
                if (Gun->ShotSerial == Predicted->ShotSerial && Gun->Reserve != Predicted->Reserve)
                {
                    Predicted->Magazine = Gun->Magazine; Predicted->Reserve = Gun->Reserve;
                }
                *Gun = *Predicted;
            }
            else PredictedShots.Remove(Gun->InstanceId);
        }
    }
    RefreshPresentation();
    OnFireReadinessChanged.Broadcast();
}
void UFPSWeaponComponent::OnRep_VisibleState()
{
    Loadout = FFPSWeaponLoadout();
    Loadout.Primary.WeaponId = VisibleState.WeaponId; Loadout.Primary.InstanceId = VisibleState.InstanceId;
    Loadout.Action = VisibleState.Action; Loadout.ActionEndsAt = VisibleState.ActionEndsAt;
    Loadout.EquipStartedAt = VisibleState.EquipStartedAt; Loadout.bAiming = VisibleState.bAiming;
    RefreshPresentation();
}
void UFPSWeaponComponent::RefreshPresentation()
{
    auto* Pawn = Character();
    if (!Pawn) return;
    // GameplayAbility activation owns and reference-counts tags; replication callbacks never overwrite them.
    Pawn->RefreshWeaponPresentation();
}
void UFPSWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    FFPSWeaponSmoke::Tick(this);
    FFPSWeaponAbilitySmoke::Tick(this);
    FFPSWeaponDamageSmoke::Tick(this);
    FFPSWeaponPresentationSmoke::Tick(this);
    if (IsEnabled() && !CanAct() && (GetOwner()->HasAuthority() || Character()->IsLocallyControlled())) StopAllActions();
}
void UFPSWeaponComponent::StartFire()
{
    if (const auto* PC = Cast<APlayerController>(Character()->GetController()); PC && PC->IsLocalController() && PC->IsMoveInputIgnored()) return;
    if (!CanAct() || bTriggerHeld) return;
    bTriggerHeld = true;
    if (Definition() && Equipped().Magazine <= 0 && ServerTime() - LastEmptyClick > .25)
    {
        LastEmptyClick = ServerTime();
        if (auto* Audio = UFPSAudioSubsystem::Get(this)) Audio->PlaySound(Definition()->EmptySound.Get());
    }
    Character()->GetAbilitySystemComponent()->AbilityLocalInputPressed(int32(EFPSWeaponRequest::Fire));
}
void UFPSWeaponComponent::StopFire()
{
    bTriggerHeld = false;
    Character()->GetAbilitySystemComponent()->AbilityLocalInputReleased(int32(EFPSWeaponRequest::Fire));
}
void UFPSWeaponComponent::AimOn()
{
    if (bAimHeld) return;
    if (const auto* PC = Cast<APlayerController>(Character()->GetController()); PC && PC->IsLocalController() && PC->IsMoveInputIgnored()) return;
    bAimHeld = true;
    Request(EFPSWeaponRequest::AimOn);
}
void UFPSWeaponComponent::AimOff()
{
    bAimHeld = false;
    Character()->GetAbilitySystemComponent()->AbilityLocalInputReleased(int32(EFPSWeaponRequest::AimOn));
}
void UFPSWeaponComponent::Reload() { Request(EFPSWeaponRequest::Reload); }
void UFPSWeaponComponent::Inspect() { Request(EFPSWeaponRequest::Inspect); }
void UFPSWeaponComponent::Primary() { Request(EFPSWeaponRequest::Primary); }
void UFPSWeaponComponent::Secondary() { Request(EFPSWeaponRequest::Secondary); }
void UFPSWeaponComponent::Drop() { Request(EFPSWeaponRequest::Drop); }
void UFPSWeaponComponent::Pickup() { Request(EFPSWeaponRequest::Pickup); }
void UFPSWeaponComponent::Request(EFPSWeaponRequest Command, AFPSWeaponPickup* Target)
{
    if (!IsEnabled() || Command == EFPSWeaponRequest::Equip) return;
    if (Command == EFPSWeaponRequest::AimOff) { AimOff(); return; }
    if (const auto* PC = Cast<APlayerController>(Character()->GetController()); PC && PC->IsLocalController() && PC->IsMoveInputIgnored()) return;
    // Selection is server-derived. Explicit callers cannot nominate a distant or stale pickup.
    if (Command == EFPSWeaponRequest::Pickup && Target && Target != FindPickup()) return;
    auto* ASC = Character()->GetAbilitySystemComponent();
    if (Command == EFPSWeaponRequest::Pickup)
    {
        FGameplayEventData Event; Event.OptionalObject = Target ? Target : FindPickup();
        ASC->HandleGameplayEvent(FGameplayTag::RequestGameplayTag(TEXT("Input.Weapon.Pickup")), &Event);
        return;
    }
    ASC->AbilityLocalInputPressed(int32(Command));
    if (Command != EFPSWeaponRequest::AimOn && Command != EFPSWeaponRequest::Fire) ASC->AbilityLocalInputReleased(int32(Command));
}
void UFPSWeaponComponent::CancelOperation(bool bIncludeAim)
{
    if (ActionAbility) ActionAbility->CancelWeaponAbility();
    if (bIncludeAim && AimAbility) AimAbility->CancelWeaponAbility();
}
void UFPSWeaponComponent::CancelBurst() { if (FireAbility && ActiveFireMode != EFPSFireMode::Automatic) FireAbility->CancelWeaponAbility(); }
void UFPSWeaponComponent::StopAllActions()
{
    const bool bHadAction = bTriggerHeld || bAimHeld || ActionAbility || AimAbility || FireAbility;
    StopFire(); AimOff();
    CancelOperation(true);
    if (FireAbility) FireAbility->CancelWeaponAbility();
    if (bHadAction && !GetOwner()->HasAuthority() && Character()->IsLocallyControlled())
        Character()->GetAbilitySystemComponent()->TryActivateAbilityByClass(UFPSCancelWeaponAbility::StaticClass());
}
void UFPSWeaponComponent::CancelForMenu() { StopAllActions(); }
void UFPSWeaponComponent::SetReloadExitsAim(bool bValue) { bReloadExitsAim = bValue; ServerSetReloadExitsAim(bValue); }
void UFPSWeaponComponent::ServerSetReloadExitsAim_Implementation(bool bValue) { bReloadExitsAim = bValue; }
FFPSWeaponActionTicket UFPSWeaponComponent::OpenActivation(UFPSWeaponAbility* Ability, EFPSWeaponRequest Command, int32 PredictionKey)
{
    OpeningTicket = {};
    OpeningAbility.Reset();
    if (!Ability || !Ability->IsActive() || Ability->Command != Command || Ability->GetAvatarActorFromActorInfo() != GetOwner()) return {};
    OpeningTicket = {++ActionSequence, Equipped().InstanceId, Command, PredictionKey};
    OpeningAbility = Ability;
    if (!GetOwner()->HasAuthority()) LastPredictionSequence = OpeningTicket.Sequence;
    return OpeningTicket;
}
bool UFPSWeaponComponent::ValidOpening(const FFPSWeaponActionTicket& Ticket, const UFPSWeaponAbility* Ability) const
{
    return Ticket == OpeningTicket && Ticket.Instance == Equipped().InstanceId && Ability && OpeningAbility.Get() == Ability && Ability->IsActive() && CanStart(Ticket.Command);
}
void UFPSWeaponComponent::RejectPrediction(const FFPSWeaponActionTicket& Ticket)
{
    if (!GetOwner()->HasAuthority())
        PredictedStarts.RemoveAll([&](const FFPSWeaponActionResult& Start)
        {
            return Start.PredictionKey == Ticket.PredictionKey && Start.Command == Ticket.Command;
        });
    // A late rejection may not roll back an activation that started after it, even on the same Ability object.
    if (GetOwner()->HasAuthority() || !Ticket.Sequence || Ticket.Sequence != LastPredictionSequence) return;
    LastPredictionSequence = 0;
    if (Ticket.Command == EFPSWeaponRequest::Fire && LastPredictedFireKey == Ticket.PredictionKey) PredictedShots.Empty();
    OnRep_OwnerState();
}
bool UFPSWeaponComponent::BeginFire(const FFPSWeaponActionTicket& Ticket, UFPSWeaponAbility* Ability)
{
    if (Ticket.Command != EFPSWeaponRequest::Fire || !ValidOpening(Ticket, Ability) || FireAbility || Ticket.Instance != Equipped().InstanceId || !Definition()) return false;
    OpeningTicket = {};
    FireTicket = Ticket; FireAbility = Ability; ActiveFireMode = Definition()->FireMode;
    Loadout.FirePredictionKey = Ticket.PredictionKey; Loadout.bFireActive = true;
    if (!GetOwner()->HasAuthority()) LastPredictedFireKey = Ticket.PredictionKey;
    return true;
}
bool UFPSWeaponComponent::RebindAutomaticFire(FFPSWeaponActionTicket& Ticket)
{
    if (!(Ticket == FireTicket) || !FireAbility || !FireAbility->IsActive() || ActiveFireMode != EFPSFireMode::Automatic || !CanAct() || !Definition()) return false;
    Ticket.Instance = Equipped().InstanceId; FireTicket = Ticket;
    return true;
}
bool UFPSWeaponComponent::FinishBurst(const FFPSWeaponActionTicket& Ticket)
{
    if (!(Ticket == FireTicket) || !FireAbility || !FireAbility->IsActive() || ActiveFireMode != EFPSFireMode::Burst || Ticket.Instance != Equipped().InstanceId || !CanAct() || !Definition()) return false;
    EquippedMutable().NextShotAt = FMath::Max(Equipped().NextShotAt, ServerTime() + Definition()->BurstInterval);
    if (!GetOwner()->HasAuthority()) PredictedShots.Add(Ticket.Instance, Equipped());
    Changed(); return true;
}
bool UFPSWeaponComponent::EndFire(const FFPSWeaponActionTicket& Ticket, bool bCancelled)
{
    if (!(Ticket == FireTicket)) return false;
    FireAbility = nullptr; FireTicket = {}; Loadout.bFireActive = false;
    if (bCancelled && !GetOwner()->HasAuthority())
    {
        PredictedShots.Remove(Ticket.Instance);
        // Correct only this instance's runtime. Never overwrite later equip/ADS/action state.
        for (auto* Gun : {&Loadout.Primary, &Loadout.Secondary})
            if (Gun->InstanceId == Ticket.Instance)
                for (const auto* Authoritative : {&OwnerState.Primary, &OwnerState.Secondary})
                    if (Authoritative->InstanceId == Ticket.Instance) *Gun = *Authoritative;
    }
    Changed(); return true;
}
bool UFPSWeaponComponent::BeginAction(FFPSWeaponActionTicket& Ticket, UFPSWeaponAbility* Ability, float& Duration)
{
    const auto Command = Ticket.Command;
    const bool bEquip = Command == EFPSWeaponRequest::Primary || Command == EFPSWeaponRequest::Secondary || Command == EFPSWeaponRequest::Equip;
    if ((!bEquip && Command != EFPSWeaponRequest::Reload && Command != EFPSWeaponRequest::Inspect) || !ValidOpening(Ticket, Ability)) return false;
    OpeningTicket = {};
    CancelOperation(bEquip); CancelBurst();
    if (Command == EFPSWeaponRequest::Primary) Loadout.ActiveSlot = EFPSWeaponSlot::Primary;
    if (Command == EFPSWeaponRequest::Secondary) Loadout.ActiveSlot = EFPSWeaponSlot::Secondary;
    if (Command == EFPSWeaponRequest::Reload && ReloadExitsAim() && AimAbility) AimAbility->CancelWeaponAbility();
    const auto* Def = Definition();
    if (!Def) return false;
    Ticket.Instance = Equipped().InstanceId;
    ActionTicket = Ticket; ActionAbility = Ability;
    if (bEquip) Loadout.EquipStartedAt = ServerTime();
    Duration = bEquip ? Def->EquipReadyTime : Command == EFPSWeaponRequest::Reload ? Def->ReloadDuration : Def->InspectDuration;
    Loadout.Action = bEquip ? EFPSWeaponAction::Switching : Command == EFPSWeaponRequest::Reload ? EFPSWeaponAction::Reloading : EFPSWeaponAction::Inspecting;
    ActionDeadline = ServerTime() + Duration;
    Loadout.ActionEndsAt = ActionDeadline; Changed();
    EmitActionResult(Ticket, EFPSWeaponResultPhase::Started); return true;
}
bool UFPSWeaponComponent::FinishAction(const FFPSWeaponActionTicket& Ticket, bool bCancelled)
{
    if (!(Ticket == ActionTicket)) return false;
    if (Ticket.Instance != Equipped().InstanceId)
    {
        // Authority replication may replace the equipped instance before a local timer ends.
        // Retire only the old registration, without altering the new replicated operation.
        ActionAbility = nullptr; ActionTicket = {}; ActionDeadline = 0;
        return false;
    }
    if (!bCancelled && (!CanAct() || ServerTime() + .002 < ActionDeadline)) return false;
    if (!bCancelled && GetOwner()->HasAuthority() && Ticket.Command == EFPSWeaponRequest::Reload && Definition())
    {
        auto& Gun = EquippedMutable();
        const int32 Added = FMath::Clamp(Definition()->MagazineSize - Gun.Magazine, 0, Gun.Reserve);
        Gun.Magazine += Added; Gun.Reserve -= Added;
    }
    EmitActionResult(Ticket, bCancelled ? EFPSWeaponResultPhase::Cancelled : EFPSWeaponResultPhase::Committed);
    ActionAbility = nullptr; ActionTicket = {}; ActionDeadline = 0;
    Loadout.Action = EFPSWeaponAction::Idle; Loadout.ActionEndsAt = 0;
    Changed(); return true;
}
void UFPSWeaponComponent::InterruptInspect()
{
    if (ActionTicket.Command == EFPSWeaponRequest::Inspect && ActionAbility) ActionAbility->CancelWeaponAbility();
}
bool UFPSWeaponComponent::BeginAim(const FFPSWeaponActionTicket& Ticket, UFPSWeaponAbility* Ability)
{
    if (Ticket.Command != EFPSWeaponRequest::AimOn || !ValidOpening(Ticket, Ability) || AimAbility || Ticket.Instance != Equipped().InstanceId) return false;
    OpeningTicket = {}; InterruptInspect();
    AimTicket = Ticket; AimAbility = Ability; Loadout.bAiming = true;
    if (auto* Move = Cast<UFPSMovementComponent>(Character()->GetCharacterMovement())) Move->bWantsSprint = false;
    Changed(); return true;
}
bool UFPSWeaponComponent::EndAim(const FFPSWeaponActionTicket& Ticket)
{
    if (!(Ticket == AimTicket)) return false;
    AimAbility = nullptr; AimTicket = {}; Loadout.bAiming = false;
    Changed(); return true;
}
EFPSFireReadiness UFPSWeaponComponent::FireReadiness(const FFPSWeaponActionTicket& Ticket, double& Deadline) const
{
    Deadline = 0;
    if (!(Ticket == FireTicket) || Ticket.Instance != Equipped().InstanceId || !FireAbility || !FireAbility->IsActive() || !CanAct() || !Definition()) return EFPSFireReadiness::Invalid;
    if (Equipped().Magazine <= 0 || Loadout.Action == EFPSWeaponAction::Reloading || Loadout.Action == EFPSWeaponAction::Switching) return EFPSFireReadiness::WaitingForState;
    if (const auto* Move = Cast<UFPSMovementComponent>(Character()->GetCharacterMovement()); Move && Move->IsSprinting()) return EFPSFireReadiness::WaitingForState;
    Deadline = Equipped().NextShotAt;
    return ServerTime() + .002 < Deadline ? EFPSFireReadiness::WaitingForDeadline : EFPSFireReadiness::Ready;
}
bool UFPSWeaponComponent::CommitShot(const FFPSWeaponActionTicket& Ticket)
{
    // Only an active shared fire Ability can request a shot; clients never supply hit or damage data.
    const auto* Def = Definition();
    if (!(Ticket == FireTicket) || Ticket.Instance != Equipped().InstanceId || !FireAbility || !FireAbility->IsActive() || !CanAct() || !Def || Equipped().Magazine <= 0 || Loadout.Action == EFPSWeaponAction::Reloading || Loadout.Action == EFPSWeaponAction::Switching) return false;
    if (const auto* Move = Cast<UFPSMovementComponent>(Character()->GetCharacterMovement()); Move && Move->IsSprinting()) return false;
    const double Now = ServerTime();
    auto& Gun = EquippedMutable();
    if (Now + .002 < Gun.NextShotAt) return false;
    if (Now - Gun.LastShotAt >= Def->RecoilRecoveryTime) Gun.RecoilIndex = 0;
    const FVector Kick = Def->RecoilAt(Gun.RecoilIndex++);
    Gun.LastShotAt = Now; Gun.NextShotAt = Now + Def->FireInterval; --Gun.Magazine; ++Gun.ShotSerial;
    const FFPSWeaponShotContext Shot = FFPSWeaponShotContext::Capture(Character(), Gun, *Def);
    if (!GetOwner()->HasAuthority()) PredictedShots.Add(Gun.InstanceId, Gun);
    Changed();
    if (Character()->IsLocallyControlled() && Character()->GetController()->IsPlayerController())
    {
        Character()->PlayWeaponShot(Kick * (Loadout.bAiming ? Def->AimRecoilMultiplier : 1.f));
#if !UE_BUILD_SHIPPING
        ++LocalShotPresentations;
#endif
    }
    if (GetOwner()->HasAuthority()) Character()->ResolveWeaponShot(Shot);
    return true;
}
bool UFPSWeaponComponent::Transfer(const FFPSWeaponActionTicket& Ticket, AFPSWeaponPickup* Target)
{
    if (!GetOwner()->HasAuthority() || !ValidOpening(Ticket, OpeningAbility.Get()) || Ticket.Instance != Equipped().InstanceId) return false;
    const auto Command = Ticket.Command;
    OpeningTicket = {};
    if (Command == EFPSWeaponRequest::Drop)
    {
        if (!SpawnDrop(Equipped())) return false;
        CancelOperation(true); CancelBurst();
        EquippedMutable() = FFPSWeaponInstance();
        Loadout.ActiveSlot = Loadout.ActiveSlot == EFPSWeaponSlot::Primary ? EFPSWeaponSlot::Secondary : EFPSWeaponSlot::Primary;
    }
    else if (Command == EFPSWeaponRequest::Pickup)
    {
        if (!IsValid(Target) || !Target->IsAvailable() || Target != FindPickup()) return false;
        const auto* Def = Catalog()->Find(Target->GetInstance().WeaponId);
        if (!Def) return false;
        auto& Destination = Def->Slot == EFPSWeaponSlot::Primary ? Loadout.Primary : Loadout.Secondary;
        auto* Replacement = Destination.IsValid() ? SpawnDrop(Destination) : nullptr;
        if (Destination.IsValid() && !Replacement) return false;
        if (!Target->Claim()) { if (Replacement) Replacement->Destroy(); return false; }
        CancelOperation(true); CancelBurst();
        Destination = Target->GetInstance(); Loadout.ActiveSlot = Def->Slot; Target->Destroy();
    }
    else return false;
    Changed(); return true;
}
AFPSWeaponPickup* UFPSWeaponComponent::FindPickup() const
{
    AFPSWeaponPickup* Best = nullptr;
    float Distance = FMath::Square(200.f);
    for (TActorIterator<AFPSWeaponPickup> It(GetWorld()); It; ++It)
    {
        const float D = FVector::DistSquared(GetOwner()->GetActorLocation(), It->GetActorLocation());
        if (!It->IsAvailable() || D >= Distance) continue;
        FHitResult Hit;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(WeaponPickup), false, GetOwner());
        GetWorld()->LineTraceSingleByChannel(Hit, Character()->GetPawnViewLocation(), It->GetActorLocation(), ECC_Visibility, Query);
        if (Hit.bBlockingHit && Hit.GetActor() != *It) continue;
        Best = *It; Distance = D;
    }
    return Best;
}
AFPSWeaponPickup* UFPSWeaponComponent::SpawnDrop(const FFPSWeaponInstance& Instance)
{
    if (!Instance.IsValid()) return nullptr;
    const FVector Start = GetOwner()->GetActorLocation();
    FVector Position = Start + GetOwner()->GetActorForwardVector() * 90;
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(WeaponDrop), false, GetOwner());
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Position, ECC_Visibility, Query)) Position = Start;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Position, Position - FVector(0, 0, 250), ECC_Visibility, Query)) Position = Hit.ImpactPoint + FVector(0, 0, 20);
    auto* Pickup = GetWorld()->SpawnActor<AFPSWeaponPickup>(Position, GetOwner()->GetActorRotation());
    if (Pickup) { Pickup->Initialize(Instance); Pickup->SetLifeSpan(180); }
    return Pickup;
}
