#include "FPSWeaponComponent.h"
#include "FPSWeaponShot.h"
#include "FPSWeaponCustomization.h"
#include "FPSRecoilSmoke.h"
#include "FPSAttachmentSmoke.h"
#include "Misc/CommandLine.h"
#include "FPSMatchFlow.h"
#include "FPSExtractionMode.h"
#include "FPSWeaponAbilities.h"
#include "FPSAudioSubsystem.h"
#include "FPSDemoCharacter.h"
#include "FPSActionPolicy.h"
#include "FPSModeRules.h"
#include "FPSBaseMembership.h"
#include "FPSDemoPlayerState.h"
#include "FPSDemoGameState.h"
#include "FPSDemoAttributeSet.h"
#include "FPSWeaponPickup.h"
#include "FPSMovementComponent.h"
#include "FPSWeaponSmoke.h"
#include "FPSWeaponAbilitySmoke.h"
#include "FPSWeaponDamageSmoke.h"
#include "FPSWeaponPresentationSmoke.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

UFPSWeaponComponent::UFPSWeaponComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
}
AFPSDemoCharacter* UFPSWeaponComponent::Character() const { return Cast<AFPSDemoCharacter>(GetOwner()); }
const UFPSWeaponCatalog* UFPSWeaponComponent::Catalog() const
{
    const auto* World = GetWorld();
    const auto* GI = World ? World->GetGameInstance() : nullptr;
    return GI ? GI->GetSubsystem<UFPSWeaponCatalog>() : nullptr;
}
bool UFPSWeaponComponent::IsShuttingDown() const
{
    const auto* World = GetWorld();
    return bShuttingDown || !World || World->bIsTearingDown || !IsValid(GetOwner()) || GetOwner()->IsActorBeingDestroyed();
}
void UFPSWeaponComponent::ShutdownActions()
{
    if (bShuttingDown) return;
    bShuttingDown = true;
    // Retire capabilities before cancellation can re-enter the component. No gameplay result during teardown.
    auto* Action = ActionAbility.Get(); auto* Aim = AimAbility.Get(); auto* Fire = FireAbility.Get();
    OpeningTicket = {}; ActionTicket = {}; AimTicket = {}; FireTicket = {};
    OpeningAbility.Reset(); ActionAbility = nullptr; AimAbility = nullptr; FireAbility = nullptr;
    OnFireReadinessChanged.Clear();
    bTriggerHeld = bAimHeld = false; ActionDeadline = 0;
    SprintTransitionReadyAt = SprintFireReadyAt = 0;
    bWasActualSprinting = bSprintFirePending = false;
    PredictedShots.Empty(); PredictedStarts.Empty();
    PendingAttachmentIds.Empty(); PendingAttachmentTicket = {}; bCustomizationOpen = false;
    Loadout.Action = EFPSWeaponAction::Idle; Loadout.ActionEndsAt = 0;
    Loadout.bAiming = false; Loadout.bFireActive = false;
    for (auto* Ability : {Action, Aim, Fire}) if (IsValid(Ability)) Ability->CancelWeaponAbility(false);
}
void UFPSWeaponComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    ShutdownActions();
    Super::EndPlay(Reason);
}
const FFPSWeaponInstance& UFPSWeaponComponent::Equipped() const { return Loadout.ActiveSlot == EFPSWeaponSlot::Primary ? Loadout.Primary : Loadout.Secondary; }
FFPSWeaponInstance& UFPSWeaponComponent::EquippedMutable() { return Loadout.ActiveSlot == EFPSWeaponSlot::Primary ? Loadout.Primary : Loadout.Secondary; }
const UFPSWeaponDefinition* UFPSWeaponComponent::Definition() const { const auto* Data = Catalog(); return Data ? Data->Find(Equipped().WeaponId) : nullptr; }
FFPSResolvedWeaponStats UFPSWeaponComponent::EffectiveStats() const
{
    FFPSResolvedWeaponStats Result; TArray<FString> Errors;
    if (const auto* Data = Catalog()) Data->Resolve(Equipped().WeaponId, Equipped().AttachmentIds, Result, Errors);
    return Result;
}
double UFPSWeaponComponent::ServerTime() const
{
    const auto* GS = GetWorld()->GetGameState();
    return GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}
bool UFPSWeaponComponent::IsEnabled() const
{
    if (IsShuttingDown()) return false;
    const auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
    return GS && GS->bStructuredMatch;
}
bool UFPSWeaponComponent::CanAct() const
{
    return !IsShuttingDown() && Catalog() && IsEnabled() && Character() && !Character()->IsEliminated() && Character()->GetActionPolicy()->CanAct() &&
        Catalog()->IsReady() && Catalog()->ValidationError.IsEmpty();
}
bool UFPSWeaponComponent::CanSprint() const { return !IsShuttingDown() && !Loadout.bAiming && !Loadout.bFireActive && Loadout.Action == EFPSWeaponAction::Idle; }
void UFPSWeaponComponent::NotifySprintStateChanged(bool bSprinting)
{
    if (IsShuttingDown()) return;
    const auto* Move = Character() ? Cast<UFPSMovementComponent>(Character()->GetCharacterMovement()) : nullptr;
    if (!Move || !Move->UsesMovementRules()) return;
    if (bSprinting) { bWasActualSprinting = true; return; }
    if (!bWasActualSprinting) return;
    bWasActualSprinting = false;
    // A stop move can reach the server before the GAS Fire activation. Keep its real transition.
    SprintTransitionReadyAt = ServerTime() + Move->GetSprintFirePreparationSeconds();
    OnFireReadinessChanged.Broadcast();
}
void UFPSWeaponComponent::InterruptSprintForWeapon()
{
    auto* Move = Character() ? Cast<UFPSMovementComponent>(Character()->GetCharacterMovement()) : nullptr;
    if (!Move || !Move->UsesMovementRules()) return;
    if (Move->IsSprinting()) NotifySprintStateChanged(true);
    Move->ClearSprintIntent();
    NotifySprintStateChanged(false);
}
void UFPSWeaponComponent::CancelPendingSprintFire()
{
    if (HasPendingSprintFire()) FireAbility->CancelWeaponAbility();
}
bool UFPSWeaponComponent::CanStart(EFPSWeaponRequest Command) const
{
    if (Command == EFPSWeaponRequest::Cancel) return IsEnabled();
    if (!CanAct()) return false;
    if(bMenuInputBlocked && Command!=EFPSWeaponRequest::Equip && !(bCustomizationOpen &&
        (Command==EFPSWeaponRequest::Modify||Command==EFPSWeaponRequest::ModifyMagazine||Command==EFPSWeaponRequest::CancelReload||Command==EFPSWeaponRequest::Reload||Command==EFPSWeaponRequest::Primary||Command==EFPSWeaponRequest::Secondary)))return false;
    if (GetWorld()->GetAuthGameMode<AFPSExtractionMode>())
    {
        // Protected expedition equipment: free world guns cannot enter the persistent economy.
        if (Command == EFPSWeaponRequest::Drop || Command == EFPSWeaponRequest::Pickup) return false;
        if (const auto* PC = Cast<AFPSExtractionController>(Character()->GetController()); PC && PC->IsExtractionPanelOpen())
            return Command == EFPSWeaponRequest::Equip;
    }
    const auto* Def = Definition();
    if (bCustomizationOpen && (Command == EFPSWeaponRequest::Fire || Command == EFPSWeaponRequest::AimOn || Command == EFPSWeaponRequest::Inspect)) return false;
    switch (Command)
    {
    case EFPSWeaponRequest::Primary: return Loadout.ActiveSlot != EFPSWeaponSlot::Primary && Loadout.Primary.IsValid();
    case EFPSWeaponRequest::Secondary: return Loadout.ActiveSlot != EFPSWeaponSlot::Secondary && Loadout.Secondary.IsValid();
    case EFPSWeaponRequest::Pickup: return FindPickup() != nullptr;
    case EFPSWeaponRequest::Drop: return Character()->GetActionPolicy()->CanDrop() && Equipped().IsValid();
    case EFPSWeaponRequest::Cancel: return true;
    case EFPSWeaponRequest::AimOff: return false; // input release is not an Ability activation
    case EFPSWeaponRequest::Equip: return true; // internal server-initiated preparation
    case EFPSWeaponRequest::Modify:
    case EFPSWeaponRequest::ModifyMagazine: return bCustomizationOpen && IsTrainingCustomizationAllowed() && Def && Loadout.Action == EFPSWeaponAction::Idle;
    case EFPSWeaponRequest::CancelReload: return bCustomizationOpen && IsTrainingCustomizationAllowed() && Loadout.Action == EFPSWeaponAction::Reloading;
    case EFPSWeaponRequest::AimOn: return Def && Loadout.Action != EFPSWeaponAction::Switching;
    case EFPSWeaponRequest::Reload: return Def && Equipped().Magazine < EffectiveStats().MagazineSize && Equipped().Reserve > 0 && Loadout.Action != EFPSWeaponAction::Reloading && Loadout.Action != EFPSWeaponAction::Switching;
    case EFPSWeaponRequest::Inspect: return Def && Loadout.Action == EFPSWeaponAction::Idle;
    case EFPSWeaponRequest::Fire: return Character()->GetActionPolicy()->CanAttack() && Def && (Def->FireMode == EFPSFireMode::Automatic || Loadout.Action == EFPSWeaponAction::Inspecting || (Equipped().Magazine > 0 && ServerTime() + .002 >= Equipped().NextShotAt && Loadout.Action != EFPSWeaponAction::Reloading && Loadout.Action != EFPSWeaponAction::Switching));
    }
    return false;
}
void UFPSWeaponComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner()->HasAuthority() && IsEnabled())
    {
        InitializeSpawnLoadout();
        GrantAbilities();
    }
}
void UFPSWeaponComponent::InitializeSpawnLoadout()
{
    if (!GetOwner()->HasAuthority() || !IsEnabled()) return;
    const auto* Rules = UFPSModeRules::ForWorld(GetWorld());
    if (!Rules || !Rules->DefaultLoadout || !Catalog()->IsReady()) return;
    auto* PS = Character()->GetPlayerState<AFPSDemoPlayerState>();
    if (PS) PS->EnsureLoadout(Rules);
    const auto* Primary = Rules->DefaultLoadout->Primary.LoadSynchronous();
    const auto* Secondary = Rules->DefaultLoadout->Secondary.LoadSynchronous();
    if (!Primary || !Secondary) return;
    const auto NewPrimary = Catalog()->CreateInstance(PS ? PS->SpawnPrimary() : Primary->WeaponId);
    const auto NewSecondary = Catalog()->CreateInstance(PS ? PS->SpawnSecondary() : Secondary->WeaponId);
    if (!NewPrimary.IsValid() || !NewSecondary.IsValid()) return;
    Loadout.Primary = NewPrimary; Loadout.Secondary = NewSecondary;
    ++Loadout.PrimaryInventoryRevision; ++Loadout.SecondaryInventoryRevision;
    Changed();
    if (bAbilitiesGranted) Character()->GetAbilitySystemComponent()->TryActivateAbilityByClass(UFPSEquipAbility::StaticClass());
}
bool UFPSWeaponComponent::SelectDefinition(FName Id, FString& Failure)
{
    auto Fail = [&](const TCHAR* Reason) { Failure = Reason; return false; };
    if (GetWorld()->GetAuthGameMode<AFPSExtractionMode>()) return Fail(TEXT("实验出征使用基地仓库中的装备。"));
    auto* Membership = Character()->FindComponentByClass<UFPSBaseMembership>();
    if (!GetOwner()->HasAuthority() || !CanAct() || !Membership) return Fail(TEXT("角色或阶段已失效。"));
    Membership->RefreshAuthority();
    if (!Membership->CanSelect()) return Fail(TEXT("本次生命已离开基地，或当前无法选枪。"));
    const auto* Rules = UFPSModeRules::ForWorld(GetWorld());
    const auto* Def = Catalog()->Find(Id);
    if (!Rules || !Def || !Catalog()->RequiredIds().Contains(Id)) return Fail(TEXT("武器未通过本局内容准入。"));
    bool Allowed = false;
    for (const auto& Ref : Rules->SelectableWeapons) if (Ref.Get() && Ref->WeaponId == Id) Allowed = true;
    if (!Allowed) return Fail(TEXT("该武器不在本局可选列表中。"));
    auto* PS = Character()->GetPlayerState<AFPSDemoPlayerState>();
    if (!PS || !Character()->GetController()->IsPlayerController()) return Fail(TEXT("缺少玩家身份。"));
    const auto NewInstance = Catalog()->CreateInstance(Id);
    if (!NewInstance.IsValid()) return Fail(TEXT("创建武器失败。"));
    auto& Destination = Def->Slot == EFPSWeaponSlot::Primary ? Loadout.Primary : Loadout.Secondary;
    // All fallible work precedes cancellation/commit. One game-thread operation owns this transition.
    if (Rules->Replacement == EFPSReplacementPolicy::Drop && Destination.IsValid() && !SpawnDrop(Destination)) return Fail(TEXT("旧枪掉落创建失败；配装未变。"));
    StopAllActions();
    Destination = NewInstance; Loadout.ActiveSlot = Def->Slot;
    if (Def->Slot == EFPSWeaponSlot::Primary) ++Loadout.PrimaryInventoryRevision; else ++Loadout.SecondaryInventoryRevision;
    if (Rules->Kind == EFPSGameplayKind::TeamMatch)
    {
        if (Def->Slot == EFPSWeaponSlot::Primary) PS->SelectedPrimary = Id; else PS->SelectedSecondary = Id;
        PS->ForceNetUpdate();
    }
    Changed();
    Character()->GetAbilitySystemComponent()->TryActivateAbilityByClass(UFPSEquipAbility::StaticClass());
    return true;
}
void UFPSWeaponComponent::CancelAttack()
{
    StopFire();
    if (FireAbility) FireAbility->CancelWeaponAbility();
}
void UFPSWeaponComponent::GrantAbilities()
{
    if (bAbilitiesGranted || !GetOwner()->HasAuthority() || !IsEnabled()) return;
    auto* ASC = Character()->GetAbilitySystemComponent();
    const TSubclassOf<UFPSWeaponAbility> Classes[] = {UFPSFireAbility::StaticClass(), UFPSReloadAbility::StaticClass(), UFPSInspectAbility::StaticClass(),
        UFPSPrimaryAbility::StaticClass(), UFPSSecondaryAbility::StaticClass(), UFPSDropAbility::StaticClass(), UFPSPickupAbility::StaticClass(), UFPSAimAbility::StaticClass(), UFPSEquipAbility::StaticClass(), UFPSCancelWeaponAbility::StaticClass(),
        UFPSModifyAttachmentAbility::StaticClass(), UFPSModifyMagazineAbility::StaticClass(), UFPSCancelReloadAbility::StaticClass()};
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
    DOREPLIFETIME_CONDITION(UFPSWeaponComponent, bCustomizationOpen, COND_OwnerOnly);
}
void UFPSWeaponComponent::Changed()
{
    if (IsShuttingDown()) return;
    if (GetOwner()->HasAuthority())
    {
        OwnerState = Loadout;
        VisibleState.WeaponId = Equipped().WeaponId; VisibleState.InstanceId = Equipped().InstanceId;
        VisibleState.AttachmentIds = Equipped().AttachmentIds; VisibleState.ConfigurationRevision = Equipped().ConfigurationRevision;
        VisibleState.Action = Loadout.Action; VisibleState.ActionEndsAt = Loadout.ActionEndsAt;
        VisibleState.ActionStartedAt = Loadout.ActionStartedAt;
        VisibleState.bAiming = Loadout.bAiming; VisibleState.EquipStartedAt = Loadout.EquipStartedAt;
        if (auto* ASC = Character()->GetAbilitySystemComponent()) ASC->SetNumericAttributeBase(UFPSDemoAttributeSet::GetWeaponDamageAttribute(), Definition() ? Definition()->Damage : 0);
        GetOwner()->ForceNetUpdate();
    }
    RefreshPresentation();
    OnFireReadinessChanged.Broadcast();
}
void UFPSWeaponComponent::OnRep_OwnerState()
{
    if (IsShuttingDown()) return;
    const auto& AuthoritativeEquipped = OwnerState.ActiveSlot == EFPSWeaponSlot::Primary ? OwnerState.Primary : OwnerState.Secondary;
    const bool bFireOwnershipChanged =
        (FireTicket.Instance == Loadout.Primary.InstanceId && Loadout.PrimaryInventoryRevision != OwnerState.PrimaryInventoryRevision) ||
        (FireTicket.Instance == Loadout.Secondary.InstanceId && Loadout.SecondaryInventoryRevision != OwnerState.SecondaryInventoryRevision);
    if (HasPendingSprintFire() && (FireTicket.Instance != AuthoritativeEquipped.InstanceId || bFireOwnershipChanged))
        CancelPendingSprintFire();
    // Replication may coalesce a drop and pickup of the same GUID into one snapshot.
    // Invalidate only that slot's prior ownership predictions before merging shot state.
    if (Loadout.PrimaryInventoryRevision != OwnerState.PrimaryInventoryRevision)
    {
        PredictedShots.Remove(Loadout.Primary.InstanceId);
        PredictedShots.Remove(OwnerState.Primary.InstanceId);
    }
    if (Loadout.SecondaryInventoryRevision != OwnerState.SecondaryInventoryRevision)
    {
        PredictedShots.Remove(Loadout.Secondary.InstanceId);
        PredictedShots.Remove(OwnerState.Secondary.InstanceId);
    }
    Loadout = OwnerState;
    // Prediction belongs to an instance while it is in this owner's inventory.
    // A later pickup of the same GUID must start from its transferred authority state.
    for (auto It = PredictedShots.CreateIterator(); It; ++It)
        if (It.Key() != OwnerState.Primary.InstanceId && It.Key() != OwnerState.Secondary.InstanceId) It.RemoveCurrent();
    // Keep unacknowledged shot presentation on its local cadence. Final authority acknowledgement
    // (including an interrupted trigger) always wins; reload ammo is accepted during a held trigger.
    if (OwnerState.FirePredictionKey == LastPredictedFireKey && !OwnerState.bFireActive) PredictedShots.Empty();
    for (auto* Gun : {&Loadout.Primary, &Loadout.Secondary})
    {
        if (auto* Predicted = PredictedShots.Find(Gun->InstanceId))
        {
            if (Predicted->ConfigurationRevision != Gun->ConfigurationRevision) { PredictedShots.Remove(Gun->InstanceId); continue; }
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
    if (IsShuttingDown()) return;
    Loadout = FFPSWeaponLoadout();
    Loadout.Primary.WeaponId = VisibleState.WeaponId; Loadout.Primary.InstanceId = VisibleState.InstanceId;
    Loadout.Primary.AttachmentIds = VisibleState.AttachmentIds; Loadout.Primary.ConfigurationRevision = VisibleState.ConfigurationRevision;
    Loadout.Action = VisibleState.Action; Loadout.ActionEndsAt = VisibleState.ActionEndsAt;
    Loadout.ActionStartedAt = VisibleState.ActionStartedAt;
    Loadout.EquipStartedAt = VisibleState.EquipStartedAt; Loadout.bAiming = VisibleState.bAiming;
    RefreshPresentation();
}
void UFPSWeaponComponent::RefreshPresentation()
{
    if (IsShuttingDown()) return;
    auto* Pawn = Character();
    if (!Pawn) return;
    // GameplayAbility activation owns and reference-counts tags; replication callbacks never overwrite them.
    Pawn->RefreshWeaponPresentation();
}
void UFPSWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    FFPSWeaponSmoke::Tick(this);
    FFPSAttachmentSmoke::Tick(this);
    FFPSRecoilSmoke::Tick(this);
    FFPSWeaponAbilitySmoke::Tick(this);
    FFPSWeaponDamageSmoke::Tick(this);
    FFPSWeaponPresentationSmoke::Tick(this);
    if (IsEnabled() && !CanAct() && (GetOwner()->HasAuthority() || Character()->IsLocallyControlled())) StopAllActions();
}
void UFPSWeaponComponent::StartFire()
{
    if (const auto* PC = Cast<APlayerController>(Character()->GetController()); PC && PC->IsLocalController() && PC->IsMoveInputIgnored()) return;
    if (!CanAct() || bCustomizationOpen || !Character()->GetActionPolicy()->CanAttack() || bTriggerHeld) return;
    InterruptSprintForWeapon();
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
    if (bAimHeld || bCustomizationOpen) return;
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
    if (const auto* PC = Cast<APlayerController>(Character()->GetController()); PC && PC->IsLocalController() && PC->IsMoveInputIgnored() && !(bCustomizationOpen && (Command == EFPSWeaponRequest::Primary || Command == EFPSWeaponRequest::Secondary || Command == EFPSWeaponRequest::Reload))) return;
    // Selection is server-derived. Explicit callers cannot nominate a distant or stale pickup.
    if (Command == EFPSWeaponRequest::Pickup && Target && !CanPickup(Target)) return;
    if ((Command == EFPSWeaponRequest::Reload || Command == EFPSWeaponRequest::Inspect ||
        Command == EFPSWeaponRequest::Primary || Command == EFPSWeaponRequest::Secondary || Command == EFPSWeaponRequest::AimOn) && CanStart(Command))
        InterruptSprintForWeapon();
    auto* ASC = Character()->GetAbilitySystemComponent();
    if (Command == EFPSWeaponRequest::Reload)
    {
        FGameplayEventData Event; Event.TargetData.Add(new FFPSWeaponCustomizationRequest(NewCustomizationRequest()));
        ASC->HandleGameplayEvent(FGameplayTag::RequestGameplayTag(TEXT("Input.Weapon.Reload")), &Event);
        return;
    }
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
    InterruptSprintForWeapon();
    StopFire(); AimOff();
    CancelOperation(true);
    if (FireAbility) FireAbility->CancelWeaponAbility();
    SprintTransitionReadyAt = SprintFireReadyAt = 0;
    bWasActualSprinting = bSprintFirePending = false;
    if (bHadAction && !GetOwner()->HasAuthority() && Character()->IsLocallyControlled())
        Character()->GetAbilitySystemComponent()->TryActivateAbilityByClass(UFPSCancelWeaponAbility::StaticClass());
}
void UFPSWeaponComponent::CancelForMenu()
{
    // A released trigger stops automatic intent; an already started burst owns its remaining shots.
    CancelPendingSprintFire();StopFire();AimOff();
    if(AimAbility)AimAbility->CancelWeaponAbility();
}
void UFPSWeaponComponent::SetMenuInputBlocked(bool Blocked)
{
    if(bMenuInputBlocked==Blocked)return;bMenuInputBlocked=Blocked;
    if(Blocked)CancelForMenu();
    if(!GetOwner()->HasAuthority()&&Character()->IsLocallyControlled())ServerMenuInputBlocked(Blocked);
}
void UFPSWeaponComponent::ServerMenuInputBlocked_Implementation(bool Blocked){bMenuInputBlocked=Blocked;if(Blocked)CancelForMenu();}
void UFPSWeaponComponent::SetReloadExitsAim(bool bValue) { bReloadExitsAim = bValue; ServerSetReloadExitsAim(bValue); }
void UFPSWeaponComponent::ServerSetReloadExitsAim_Implementation(bool bValue) { bReloadExitsAim = bValue; }
FFPSWeaponActionTicket UFPSWeaponComponent::OpenActivation(UFPSWeaponAbility* Ability, EFPSWeaponRequest Command, int32 PredictionKey)
{
    OpeningTicket = {};
    OpeningAbility.Reset();
    if (IsShuttingDown() || !Ability || !Ability->IsActive() || Ability->Command != Command || Ability->GetAvatarActorFromActorInfo() != GetOwner()) return {};
    OpeningTicket = {++ActionSequence, Equipped().InstanceId, Command, PredictionKey, Equipped().ConfigurationRevision};
    OpeningAbility = Ability;
    if (!GetOwner()->HasAuthority()) LastPredictionSequence = OpeningTicket.Sequence;
    return OpeningTicket;
}
bool UFPSWeaponComponent::ValidOpening(const FFPSWeaponActionTicket& Ticket, const UFPSWeaponAbility* Ability) const
{
    return Ticket == OpeningTicket && Ticket.Instance == Equipped().InstanceId && Ticket.ConfigurationRevision == Equipped().ConfigurationRevision && Ability && OpeningAbility.Get() == Ability && Ability->IsActive() && CanStart(Ticket.Command);
}
void UFPSWeaponComponent::RejectPrediction(const FFPSWeaponActionTicket& Ticket)
{
    if (IsShuttingDown()) return;
    if (!GetOwner()->HasAuthority())
        PredictedStarts.RemoveAll([&](const FFPSWeaponActionResult& Start)
        {
            return Start.PredictionKey == Ticket.PredictionKey && Start.Command == Ticket.Command;
        });
    // A late rejection may not roll back an activation that started after it, even on the same Ability object.
    if (GetOwner()->HasAuthority() || !Ticket.Sequence || Ticket.Sequence != LastPredictionSequence) return;
    if (Ticket.Command == EFPSWeaponRequest::Modify || Ticket.Command == EFPSWeaponRequest::ModifyMagazine || Ticket.Command == EFPSWeaponRequest::CancelReload)
        ModificationFeedback = TEXT("服务器拒绝了请求，已恢复权威状态。");
    LastPredictionSequence = 0;
    if (Ticket.Command == EFPSWeaponRequest::Fire && LastPredictedFireKey == Ticket.PredictionKey) PredictedShots.Empty();
    OnRep_OwnerState();
}
bool UFPSWeaponComponent::BeginFire(const FFPSWeaponActionTicket& Ticket, UFPSWeaponAbility* Ability)
{
    if (Ticket.Command != EFPSWeaponRequest::Fire || !ValidOpening(Ticket, Ability) || FireAbility || Ticket.Instance != Equipped().InstanceId || !Definition()) return false;
    InterruptSprintForWeapon();
    OpeningTicket = {};
    FireTicket = Ticket; FireAbility = Ability; ActiveFireMode = Definition()->FireMode;
    SprintFireReadyAt = SprintTransitionReadyAt > ServerTime() ? SprintTransitionReadyAt : 0;
    // Only a valid single/burst press receives this one exception to the usual no-queue rule.
    bSprintFirePending = ServerTime() < SprintFireReadyAt &&
        (ActiveFireMode == EFPSFireMode::Automatic || (Equipped().Magazine > 0 && ServerTime() + .002 >= Equipped().NextShotAt));
    Loadout.FirePredictionKey = Ticket.PredictionKey; Loadout.bFireActive = true;
    if (!GetOwner()->HasAuthority()) LastPredictedFireKey = Ticket.PredictionKey;
    Changed();
    return true;
}
bool UFPSWeaponComponent::RebindAutomaticFire(FFPSWeaponActionTicket& Ticket)
{
    if (!(Ticket == FireTicket) || !FireAbility || !FireAbility->IsActive() || bSprintFirePending || ActiveFireMode != EFPSFireMode::Automatic || !CanAct() || !Definition()) return false;
    Ticket.Instance = Equipped().InstanceId; Ticket.ConfigurationRevision = Equipped().ConfigurationRevision; FireTicket = Ticket;
    return true;
}
bool UFPSWeaponComponent::FinishBurst(const FFPSWeaponActionTicket& Ticket)
{
    if (!(Ticket == FireTicket) || !FireAbility || !FireAbility->IsActive() || ActiveFireMode != EFPSFireMode::Burst || Ticket.Instance != Equipped().InstanceId || Ticket.ConfigurationRevision != Equipped().ConfigurationRevision || !CanAct() || !Definition()) return false;
    EquippedMutable().NextShotAt = FMath::Max(Equipped().NextShotAt, ServerTime() + Definition()->BurstInterval);
    if (!GetOwner()->HasAuthority()) PredictedShots.Add(Ticket.Instance, Equipped());
    Changed(); return true;
}
bool UFPSWeaponComponent::EndFire(const FFPSWeaponActionTicket& Ticket, bool bCancelled)
{
    if (!(Ticket == FireTicket)) return false;
    FireAbility = nullptr; FireTicket = {}; Loadout.bFireActive = false;
    SprintFireReadyAt = 0; bSprintFirePending = false;
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
bool UFPSWeaponComponent::BeginAction(FFPSWeaponActionTicket& Ticket, UFPSWeaponAbility* Ability, float& Duration, const FFPSWeaponCustomizationRequest* Request)
{
    const auto Command = Ticket.Command;
    const bool bEquip = Command == EFPSWeaponRequest::Primary || Command == EFPSWeaponRequest::Secondary || Command == EFPSWeaponRequest::Equip;
    if ((!bEquip && Command != EFPSWeaponRequest::Reload && Command != EFPSWeaponRequest::Inspect) || !ValidOpening(Ticket, Ability)) return false;
    if (Request && (Request->Instance != Equipped().InstanceId || Request->Revision != Equipped().ConfigurationRevision || !Request->Operation.IsValid())) return false;
    InterruptSprintForWeapon();
    if (bEquip) CancelPendingSprintFire();
    OpeningTicket = {};
    CancelOperation(bEquip); CancelBurst();
    if (Command == EFPSWeaponRequest::Primary) Loadout.ActiveSlot = EFPSWeaponSlot::Primary;
    if (Command == EFPSWeaponRequest::Secondary) Loadout.ActiveSlot = EFPSWeaponSlot::Secondary;
    if (Command == EFPSWeaponRequest::Reload && ReloadExitsAim() && AimAbility) AimAbility->CancelWeaponAbility();
    const auto* Def = Definition();
    if (!Def) return false;
    Ticket.Instance = Equipped().InstanceId; Ticket.ConfigurationRevision = Equipped().ConfigurationRevision;
    ActionTicket = Ticket; ActionAbility = Ability;
    if (bEquip) Loadout.EquipStartedAt = ServerTime();
    Duration = bEquip ? Def->EquipReadyTime : Command == EFPSWeaponRequest::Reload ? EffectiveStats().ReloadDuration : Def->InspectDuration;
    Loadout.Action = bEquip ? EFPSWeaponAction::Switching : Command == EFPSWeaponRequest::Reload ? EFPSWeaponAction::Reloading : EFPSWeaponAction::Inspecting;
    ActionDeadline = ServerTime() + Duration;
    Loadout.ActionStartedAt = ServerTime();
    Loadout.ActionToken = Request ? Request->Operation : FGuid::NewGuid();
    Loadout.ActionEndsAt = ActionDeadline; Changed();
    EmitActionResult(Ticket, EFPSWeaponResultPhase::Started); return true;
}
bool UFPSWeaponComponent::FinishAction(const FFPSWeaponActionTicket& Ticket, bool bCancelled)
{
    if (IsShuttingDown() || !(Ticket == ActionTicket)) return false;
    if (Ticket.Instance != Equipped().InstanceId || Ticket.ConfigurationRevision != Equipped().ConfigurationRevision)
    {
        // Authority replication may replace the equipped instance before a local timer ends.
        // Retire only the old registration, without altering the new replicated operation.
        if (PendingAttachmentTicket == Ticket) { PendingAttachmentTicket = {}; PendingAttachmentIds.Empty(); PendingModificationSequence = 0; }
        ActionAbility = nullptr; ActionTicket = {}; ActionDeadline = 0;
        return false;
    }
    if (!bCancelled && (!CanAct() || ServerTime() + .002 < ActionDeadline)) return false;
    const bool Modifying = Ticket == PendingAttachmentTicket;
    if (Modifying && !bCancelled && GetOwner()->HasAuthority())
    {
        TArray<FString> Errors; FFPSResolvedWeaponStats Stats;
        auto& Gun = EquippedMutable();
        if (!Catalog()->Resolve(Gun.WeaponId, PendingAttachmentIds, Stats, Errors) || Gun.ConfigurationRevision == MAX_uint32) return false;
        if (Ticket.Command == EFPSWeaponRequest::ModifyMagazine)
        {
            const int64 Total = int64(Gun.Magazine) + Gun.Reserve;
            const int32 Loaded = int32(FMath::Min<int64>(Total, Stats.MagazineSize));
            if (Total - Loaded > MAX_int32) return false;
            Gun.Magazine = Loaded; Gun.Reserve = int32(Total - Loaded);
        }
        Gun.AttachmentIds = PendingAttachmentIds; ++Gun.ConfigurationRevision;
    }
    if (!bCancelled && GetOwner()->HasAuthority() && Ticket.Command == EFPSWeaponRequest::Reload && Definition())
    {
        auto& Gun = EquippedMutable();
        const int32 Added = FMath::Clamp(EffectiveStats().MagazineSize - Gun.Magazine, 0, Gun.Reserve);
        Gun.Magazine += Added; Gun.Reserve -= Added;
    }
    EmitActionResult(Ticket, bCancelled ? EFPSWeaponResultPhase::Cancelled : EFPSWeaponResultPhase::Committed);
    if (Modifying)
    {
        PendingAttachmentIds.Empty(); PendingAttachmentTicket = {};
        if (GetOwner()->HasAuthority()) ClientModificationResult(PendingModificationSequence, bCancelled ? TEXT("改装已取消，原装配保留。") : TEXT("改装已提交。"));
        PendingModificationSequence = 0;
    }
    ActionAbility = nullptr; ActionTicket = {}; ActionDeadline = 0;
    Loadout.Action = EFPSWeaponAction::Idle; Loadout.ActionEndsAt = 0; Loadout.ActionToken = {};
    Changed(); return true;
}
void UFPSWeaponComponent::InterruptInspect()
{
    if (ActionTicket.Command == EFPSWeaponRequest::Inspect && ActionAbility) ActionAbility->CancelWeaponAbility();
}
bool UFPSWeaponComponent::BeginAim(const FFPSWeaponActionTicket& Ticket, UFPSWeaponAbility* Ability)
{
    if (Ticket.Command != EFPSWeaponRequest::AimOn || !ValidOpening(Ticket, Ability) || AimAbility || Ticket.Instance != Equipped().InstanceId) return false;
    InterruptSprintForWeapon();
    OpeningTicket = {}; InterruptInspect();
    AimTicket = Ticket; AimAbility = Ability; Loadout.bAiming = true;
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
    if (!(Ticket == FireTicket) || Ticket.Instance != Equipped().InstanceId || Ticket.ConfigurationRevision != Equipped().ConfigurationRevision || !FireAbility || !FireAbility->IsActive() || !CanAct() || !Character()->GetActionPolicy()->CanAttack() || !Definition()) return EFPSFireReadiness::Invalid;
    if (Equipped().Magazine <= 0 || Loadout.Action == EFPSWeaponAction::Reloading || Loadout.Action == EFPSWeaponAction::Switching) return EFPSFireReadiness::WaitingForState;
    if (const auto* Move = Cast<UFPSMovementComponent>(Character()->GetCharacterMovement()); Move && Move->IsSprinting()) return EFPSFireReadiness::WaitingForState;
    Deadline = FMath::Max(Equipped().NextShotAt, SprintFireReadyAt);
    if (bSprintFirePending && ServerTime() < SprintFireReadyAt) return EFPSFireReadiness::WaitingForSprintPreparation;
    return ServerTime() + .002 < Deadline ? EFPSFireReadiness::WaitingForDeadline : EFPSFireReadiness::Ready;
}
bool UFPSWeaponComponent::CommitShot(const FFPSWeaponActionTicket& Ticket)
{
    // Only an active shared fire Ability can request a shot; clients never supply hit or damage data.
    const auto* Def = Definition();
    if (!(Ticket == FireTicket) || Ticket.Instance != Equipped().InstanceId || !FireAbility || !FireAbility->IsActive() || !CanAct() || !Character()->GetActionPolicy()->CanAttack() || !Def || Equipped().Magazine <= 0 || Loadout.Action == EFPSWeaponAction::Reloading || Loadout.Action == EFPSWeaponAction::Switching) return false;
    if (const auto* Move = Cast<UFPSMovementComponent>(Character()->GetCharacterMovement()); Move && Move->IsSprinting()) return false;
    const double Now = ServerTime();
    auto& Gun = EquippedMutable();
    if (Now + .002 < Gun.NextShotAt || Now < SprintFireReadyAt) return false;
    bSprintFirePending = false; SprintFireReadyAt = 0;
    if (Now - Gun.LastShotAt >= Def->RecoilRecoveryTime) Gun.RecoilIndex = 0;
    const auto Stats = EffectiveStats();
    if (Stats.MagazineSize <= 0 || Ticket.ConfigurationRevision != Gun.ConfigurationRevision) return false;
    FVector Kick = Def->RecoilAt(Gun.RecoilIndex++);
    Kick.X *= Stats.VerticalRecoilMultiplier; Kick.Y *= Stats.HorizontalRecoilMultiplier;
    Gun.LastShotAt = Now; Gun.NextShotAt = Now + Def->FireInterval; --Gun.Magazine; ++Gun.ShotSerial;
    const FFPSWeaponShotContext Shot = FFPSWeaponShotContext::Capture(Character(), Gun, *Def);
    if (!GetOwner()->HasAuthority()) PredictedShots.Add(Gun.InstanceId, Gun);
    Changed();
    if (Character()->IsLocallyControlled() && Character()->GetController()->IsPlayerController())
    {
        const FRotator Before = Character()->GetController()->GetControlRotation();
        Character()->PlayWeaponShot(Kick * (Loadout.bAiming ? Def->AimRecoilMultiplier : 1.f));
#if !UE_BUILD_SHIPPING
        if (FParse::Param(FCommandLine::Get(), TEXT("FPSRecoilSmoke")))
        {
            const auto After = Character()->GetController()->GetControlRotation();
            UE_LOG(LogTemp, Display, TEXT("FPS_RECOIL_SAMPLE PRED serial=%u time=%.6f beforePitch=%.6f beforeYaw=%.6f pitch=%.6f yaw=%.6f kickPitch=%.6f kickYaw=%.6f"),
                Gun.ShotSerial, ServerTime(), Before.Pitch, Before.Yaw, After.Pitch, After.Yaw, Kick.X, Kick.Y);
        }
#endif
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
        if (Loadout.ActiveSlot == EFPSWeaponSlot::Primary) ++Loadout.PrimaryInventoryRevision; else ++Loadout.SecondaryInventoryRevision;
        Loadout.ActiveSlot = Loadout.ActiveSlot == EFPSWeaponSlot::Primary ? EFPSWeaponSlot::Secondary : EFPSWeaponSlot::Primary;
    }
    else if (Command == EFPSWeaponRequest::Pickup)
    {
        if (!CanPickup(Target)) return false;
        const auto* Def = Catalog()->Find(Target->GetInstance().WeaponId);
        if (!Def) return false;
        if (!Target->Claim()) return false; // reserve before cap enforcement can retire other drops
        auto& Destination = Def->Slot == EFPSWeaponSlot::Primary ? Loadout.Primary : Loadout.Secondary;
        const bool ShouldEquip = Loadout.ActiveSlot == Def->Slot || !Equipped().IsValid();
        auto* Replacement = Destination.IsValid() ? SpawnDrop(Destination) : nullptr;
        if (Destination.IsValid() && !Replacement) { Target->ReleaseClaim(); return false; }
        CancelOperation(true); CancelBurst();
        Destination = Target->GetInstance();
        if (Def->Slot == EFPSWeaponSlot::Primary) ++Loadout.PrimaryInventoryRevision; else ++Loadout.SecondaryInventoryRevision;
        if (ShouldEquip) Loadout.ActiveSlot = Def->Slot;
        Target->Destroy();
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
        if (D >= Distance || !CanPickup(*It)) continue;
        Best = *It; Distance = D;
    }
    return Best;
}
bool UFPSWeaponComponent::CanPickup(const AFPSWeaponPickup* Target) const
{
    if (!IsValid(Target) || !Target->IsAvailable() || FVector::DistSquared(GetOwner()->GetActorLocation(), Target->GetActorLocation()) >= FMath::Square(200.f)) return false;
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(WeaponPickup), false, GetOwner());
    Query.AddIgnoredActor(Target);
    GetWorld()->LineTraceSingleByChannel(Hit, Character()->GetPawnViewLocation(), Target->GetActorLocation(), ECC_Visibility, Query);
    return !Hit.bBlockingHit;
}
AFPSWeaponPickup* UFPSWeaponComponent::SpawnDrop(const FFPSWeaponInstance& Instance)
{
    if (!GetOwner()->HasAuthority() || !Instance.IsValid()) return nullptr;
    const auto* Rules = UFPSModeRules::ForWorld(GetWorld());
    if (!Rules || Rules->DynamicDropLimit < 1 || Rules->DynamicDropLifetime <= 0) return nullptr;
    const FVector Start = GetOwner()->GetActorLocation() + FVector(0,0,20);
    FVector Position = Start + GetOwner()->GetActorForwardVector() * 75;
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(WeaponDrop), false, GetOwner());
    FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_WorldStatic); Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    const auto* Def = Catalog()->Find(Instance.WeaponId);
    if (!Def || !Def->Pickup.Mesh.Get()) return nullptr;
    const FVector Extent = Def->Pickup.Mesh.Get()->GetBoundingBox().TransformBy(Def->Pickup.Transform).GetExtent().ComponentMax(FVector(3));
    const auto Shape = FCollisionShape::MakeBox(Extent + FVector(2));
    const FQuat Rotation = GetOwner()->GetActorQuat();
    if (GetWorld()->SweepSingleByObjectType(Hit, Start, Position, Rotation, Objects, Shape, Query)) Position = Hit.Location;
    if (GetWorld()->OverlapAnyTestByObjectType(Position, Rotation, Objects, Shape, Query)) Position = Start;
    if (GetWorld()->OverlapAnyTestByObjectType(Position, Rotation, Objects, Shape, Query)) return nullptr;
    auto* Pickup = GetWorld()->SpawnActor<AFPSWeaponPickup>(Position, GetOwner()->GetActorRotation());
    if (Pickup)
    {
        Pickup->Initialize(Instance);
        const FVector Velocity = GetOwner()->GetActorForwardVector() * 160 + FVector(0,0,110) + GetOwner()->GetVelocity().GetClampedToMaxSize(600) * .35;
        if (!Pickup->LaunchDynamic(Velocity, FVector(90,140,50))) { Pickup->Destroy(); return nullptr; }
    }
    return Pickup;
}
