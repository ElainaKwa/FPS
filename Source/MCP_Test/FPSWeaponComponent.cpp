#include "FPSWeaponComponent.h"
#include "FPSAudioSubsystem.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoAttributeSet.h"
#include "FPSWeaponPickup.h"
#include "FPSMovementComponent.h"
#include "FPSWeaponSmoke.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Reloading, "State.Weapon.Reloading");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Inspecting, "State.Weapon.Inspecting");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Switching, "State.Weapon.Switching");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Aiming, "State.Weapon.Aiming");

UFPSWeaponComponent::UFPSWeaponComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
}
AFPSDemoCharacter* UFPSWeaponComponent::Character() const { return Cast<AFPSDemoCharacter>(GetOwner()); }
const UFPSWeaponCatalog* UFPSWeaponComponent::Catalog() const
{
    return GetWorld()->GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
}
const FFPSWeaponInstance& UFPSWeaponComponent::Equipped() const
{
    return Loadout.ActiveSlot == EFPSWeaponSlot::Primary ? Loadout.Primary : Loadout.Secondary;
}
FFPSWeaponInstance& UFPSWeaponComponent::EquippedMutable()
{
    return Loadout.ActiveSlot == EFPSWeaponSlot::Primary ? Loadout.Primary : Loadout.Secondary;
}
const UFPSWeaponDefinition* UFPSWeaponComponent::Definition() const { return Catalog()->Find(Equipped().Model); }
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
        GS && !GS->bWaiting && !GS->bFinished && (GS->bTraining || GS->RoundEndsAt <= 0 || ServerTime() < GS->RoundEndsAt);
}
bool UFPSWeaponComponent::CanSprint() const
{
    return CanAct() && !Loadout.bAiming && Loadout.Action == EFPSWeaponAction::Idle;
}
void UFPSWeaponComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner()->HasAuthority() && IsEnabled())
    {
        Loadout.Primary = Catalog()->CreateInstance(EFPSWeaponModel::M4);
        Loadout.Secondary = Catalog()->CreateInstance(EFPSWeaponModel::M9);
        Changed();
    }
}
void UFPSWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UFPSWeaponComponent, Loadout);
}
void UFPSWeaponComponent::Changed()
{
    OnRep_Loadout();
    if (auto* ASC = Character()->GetAbilitySystemComponent())
        ASC->SetNumericAttributeBase(UFPSDemoAttributeSet::GetWeaponDamageAttribute(), Definition() ? Definition()->Damage : 0);
    GetOwner()->ForceNetUpdate();
}
void UFPSWeaponComponent::OnRep_Loadout()
{
    auto* Pawn = Character();
    if (!Pawn) return;
    if (Pawn->IsLocallyControlled() && Pawn->GetController() && Pawn->GetController()->IsPlayerController())
    {
        if (auto* Audio = UFPSAudioSubsystem::Get(this))
        {
            if (Loadout.Action != LastAudioAction)
            {
                if (Loadout.Action == EFPSWeaponAction::Reloading) Audio->Play(TEXT("ReloadStart"));
                else if (Loadout.Action == EFPSWeaponAction::Switching) Audio->Play(TEXT("Equip"));
                else if (Loadout.Action == EFPSWeaponAction::Inspecting) Audio->Play(TEXT("Inspect"));
                else if (LastAudioAction == EFPSWeaponAction::Reloading && LastAudioInstance == Equipped().InstanceId && Equipped().Magazine > LastAudioMagazine)
                    Audio->Play(TEXT("ReloadEnd"));
            }
            else if (LastAudioInstance.IsValid() && Equipped().IsValid() && LastAudioInstance != Equipped().InstanceId)
                Audio->Play(TEXT("Equip"));
        }
    }
    LastAudioAction = Loadout.Action;
    LastAudioInstance = Equipped().InstanceId;
    LastAudioMagazine = Equipped().Magazine;
    if (auto* ASC = Pawn->GetAbilitySystemComponent())
    {
        ASC->SetLooseGameplayTagCount(TAG_Reloading, Loadout.Action == EFPSWeaponAction::Reloading ? 1 : 0);
        ASC->SetLooseGameplayTagCount(TAG_Inspecting, Loadout.Action == EFPSWeaponAction::Inspecting ? 1 : 0);
        ASC->SetLooseGameplayTagCount(TAG_Switching, Loadout.Action == EFPSWeaponAction::Switching ? 1 : 0);
        ASC->SetLooseGameplayTagCount(TAG_Aiming, Loadout.bAiming ? 1 : 0);
    }
    Pawn->RefreshWeaponPresentation();
}
void UFPSWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    FFPSWeaponSmoke::Tick(this);
    if (!IsEnabled()) return;
    if (!CanAct())
    {
        StopFire();
        if (GetOwner()->HasAuthority() && (Loadout.Action != EFPSWeaponAction::Idle || Loadout.bAiming)) StopAllActions();
        return;
    }
    if (GetOwner()->HasAuthority() && Loadout.Action != EFPSWeaponAction::Idle && ServerTime() >= Loadout.ActionEndsAt)
    {
        if (Loadout.Action == EFPSWeaponAction::Reloading && Equipped().InstanceId == ActionWeaponId && Definition())
        {
            auto& Gun = EquippedMutable();
            const int32 Added = FMath::Min(Definition()->MagazineSize - Gun.Magazine, Gun.Reserve);
            Gun.Magazine += Added; Gun.Reserve -= Added;
        }
        Loadout.Action = EFPSWeaponAction::Idle;
        Loadout.ActionEndsAt = 0;
        Changed();
    }
    if (bTriggerHeld && Character()->IsLocallyControlled() && Definition() && Definition()->bAutomatic && GetWorld()->GetTimeSeconds() >= NextLocalShot)
        ShootLocal();
}
void UFPSWeaponComponent::StartFire()
{
    if (const auto* PC = Cast<APlayerController>(Character()->GetController()); PC && PC->IsLocalController() && PC->IsMoveInputIgnored()) return;
    if (!CanAct() || bTriggerHeld) return;
    if (Equipped().IsValid() && Equipped().Magazine <= 0 && Loadout.Action == EFPSWeaponAction::Idle && GetWorld()->GetTimeSeconds() - LastEmptyClick > .25)
    {
        LastEmptyClick = GetWorld()->GetTimeSeconds();
        if (auto* Audio = UFPSAudioSubsystem::Get(this)) Audio->Play(TEXT("Empty"));
    }
    bTriggerHeld = true;
    LocalShotIndex = 0;
    ShootLocal();
}
void UFPSWeaponComponent::StopFire() { bTriggerHeld = false; LocalShotIndex = 0; }
void UFPSWeaponComponent::ShootLocal()
{
    const auto* Def = Definition();
    if (!CanAct() || !Def || Equipped().Magazine <= 0 || Loadout.Action == EFPSWeaponAction::Reloading || Loadout.Action == EFPSWeaponAction::Switching) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextLocalShot) return;
    auto* Movement = Cast<UFPSMovementComponent>(Character()->GetCharacterMovement());
    if (Movement && Movement->IsSprinting()) return;
    NextLocalShot = Now + Def->FireInterval;
    ServerShoot();
    const FVector Kick = Def->RecoilCurve ? Def->RecoilCurve->GetVectorValue(LocalShotIndex++) : FVector(1, 0, 1);
    Character()->PlayWeaponShot(Kick * (Loadout.bAiming ? .65f : 1.f));
}
void UFPSWeaponComponent::ServerShoot_Implementation() { FireAuthority(); }
bool UFPSWeaponComponent::FireAuthority()
{
    const auto* Def = Definition();
    if (!GetOwner()->HasAuthority() || !CanAct() || !Def || !Equipped().IsValid() || Equipped().Magazine <= 0) return false;
    if (Loadout.Action == EFPSWeaponAction::Reloading || Loadout.Action == EFPSWeaponAction::Switching) return false;
    const auto* Movement = Cast<UFPSMovementComponent>(Character()->GetCharacterMovement());
    if (Movement && Movement->IsSprinting()) return false;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - LastServerShot + .002 < Def->FireInterval) return false;
    LastServerShot = Now;
    --EquippedMutable().Magazine;
    Loadout.Action = EFPSWeaponAction::Idle;
    Changed();
    Character()->ResolveWeaponShot(Def->Range);
    return true;
}
void UFPSWeaponComponent::BeginAction(EFPSWeaponAction Action, float Duration)
{
    Loadout.Action = Action;
    Loadout.ActionEndsAt = ServerTime() + Duration;
    Loadout.bAiming = false;
    ActionWeaponId = Equipped().InstanceId;
    Changed();
}
void UFPSWeaponComponent::StopAllActions()
{
    StopFire();
    if (!GetOwner()->HasAuthority()) return;
    Loadout.Action = EFPSWeaponAction::Idle;
    Loadout.ActionEndsAt = 0; Loadout.bAiming = false;
    Changed();
}
void UFPSWeaponComponent::CancelForMenu() { StopFire(); AimOff(); }
void UFPSWeaponComponent::Reload() { StopFire(); Request(EFPSWeaponRequest::Reload); }
void UFPSWeaponComponent::Inspect() { StopFire(); Request(EFPSWeaponRequest::Inspect); }
void UFPSWeaponComponent::Primary() { StopFire(); Request(EFPSWeaponRequest::Primary); }
void UFPSWeaponComponent::Secondary() { StopFire(); Request(EFPSWeaponRequest::Secondary); }
void UFPSWeaponComponent::Drop() { StopFire(); Request(EFPSWeaponRequest::Drop); }
void UFPSWeaponComponent::Pickup() { StopFire(); Request(EFPSWeaponRequest::Pickup, FindPickup()); }
void UFPSWeaponComponent::AimOn()
{
    if (auto* Movement = Cast<UFPSMovementComponent>(Character()->GetCharacterMovement())) Movement->bWantsSprint = false;
    Request(EFPSWeaponRequest::AimOn);
}
void UFPSWeaponComponent::AimOff() { Request(EFPSWeaponRequest::AimOff); }
void UFPSWeaponComponent::Request(EFPSWeaponRequest Command, AFPSWeaponPickup* Target)
{
    if (Command != EFPSWeaponRequest::AimOff)
        if (const auto* PC = Cast<APlayerController>(Character()->GetController()); PC && PC->IsLocalController() && PC->IsMoveInputIgnored()) return;
    if (IsEnabled()) ServerRequest(Command, Target);
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
void UFPSWeaponComponent::ServerRequest_Implementation(EFPSWeaponRequest Command, AFPSWeaponPickup* Target)
{
    if (Command == EFPSWeaponRequest::AimOff) { Loadout.bAiming = false; Changed(); return; }
    if (!CanAct()) return;
    if (Command == EFPSWeaponRequest::AimOn)
    {
        if (!Equipped().IsValid() || Loadout.Action != EFPSWeaponAction::Idle) return;
        if (auto* Movement = Cast<UFPSMovementComponent>(Character()->GetCharacterMovement())) Movement->bWantsSprint = false;
        Loadout.bAiming = true; Changed(); return;
    }
    if (Command == EFPSWeaponRequest::Reload)
    {
        if (!Definition() || Loadout.Action == EFPSWeaponAction::Reloading || Loadout.Action == EFPSWeaponAction::Switching || Equipped().Reserve <= 0 || Equipped().Magazine >= Definition()->MagazineSize) return;
        BeginAction(EFPSWeaponAction::Reloading, Definition()->ReloadDuration); return;
    }
    if (Command == EFPSWeaponRequest::Inspect)
    {
        if (Equipped().IsValid() && Loadout.Action == EFPSWeaponAction::Idle) BeginAction(EFPSWeaponAction::Inspecting, 2.f);
        return;
    }
    if (Command == EFPSWeaponRequest::Primary || Command == EFPSWeaponRequest::Secondary)
    {
        const auto Slot = Command == EFPSWeaponRequest::Primary ? EFPSWeaponSlot::Primary : EFPSWeaponSlot::Secondary;
        if (Slot == Loadout.ActiveSlot || !(Slot == EFPSWeaponSlot::Primary ? Loadout.Primary : Loadout.Secondary).IsValid()) return;
        Loadout.ActiveSlot = Slot;
        BeginAction(EFPSWeaponAction::Switching, .35f); return;
    }
    if (Command == EFPSWeaponRequest::Drop)
    {
        if (!Equipped().IsValid() || !SpawnDrop(Equipped())) return;
        EquippedMutable() = FFPSWeaponInstance();
        Loadout.ActiveSlot = Loadout.ActiveSlot == EFPSWeaponSlot::Primary ? EFPSWeaponSlot::Secondary : EFPSWeaponSlot::Primary;
        BeginAction(EFPSWeaponAction::Switching, .35f); return;
    }
    if (Command == EFPSWeaponRequest::Pickup)
    {
        if (!IsValid(Target) || !Target->IsAvailable() || Target != FindPickup()) return;
        const auto* Def = Catalog()->Find(Target->GetInstance().Model);
        if (!Def) return;
        auto& Destination = Def->Slot == EFPSWeaponSlot::Primary ? Loadout.Primary : Loadout.Secondary;
        // Both requests execute on the game thread: claim is the single authority gate.
        AFPSWeaponPickup* Replacement = Destination.IsValid() ? SpawnDrop(Destination) : nullptr;
        if (Destination.IsValid() && !Replacement) return;
        if (!Target->Claim()) { if (Replacement) Replacement->Destroy(); return; }
        Destination = Target->GetInstance();
        Loadout.ActiveSlot = Def->Slot;
        Target->Destroy();
        BeginAction(EFPSWeaponAction::Switching, .35f);
    }
}
