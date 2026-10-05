#include "FPSMovementComponent.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoAttributeSet.h"
#include "FPSWeaponComponent.h"

namespace
{
class FFPSMove : public FSavedMove_Character
{
public:
    using Super = FSavedMove_Character;
    bool bSavedSprint = false;
    virtual void Clear() override { Super::Clear(); bSavedSprint = false; }
    virtual uint8 GetCompressedFlags() const override { return Super::GetCompressedFlags() | (bSavedSprint ? FLAG_Custom_0 : 0); }
    virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override
    {
        return bSavedSprint == static_cast<const FFPSMove*>(NewMove.Get())->bSavedSprint && Super::CanCombineWith(NewMove, Character, MaxDelta);
    }
    virtual void SetMoveFor(ACharacter* Character, float Delta, FVector const& Accel, FNetworkPredictionData_Client_Character& ClientData) override
    {
        Super::SetMoveFor(Character, Delta, Accel, ClientData);
        bSavedSprint = CastChecked<UFPSMovementComponent>(Character->GetCharacterMovement())->bWantsSprint;
    }
    virtual void PrepMoveFor(ACharacter* Character) override
    {
        Super::PrepMoveFor(Character);
        CastChecked<UFPSMovementComponent>(Character->GetCharacterMovement())->bWantsSprint = bSavedSprint;
    }
};
class FFPSPrediction : public FNetworkPredictionData_Client_Character
{
public:
    explicit FFPSPrediction(const UCharacterMovementComponent& Movement) : FNetworkPredictionData_Client_Character(Movement) {}
    virtual FSavedMovePtr AllocateNewMove() override { return FSavedMovePtr(new FFPSMove()); }
};
}
bool UFPSMovementComponent::IsSprinting() const
{
    const auto* Pawn = Cast<AFPSDemoCharacter>(CharacterOwner);
    return bWantsSprint && Pawn && !Pawn->bIsCrouched && !IsFalling() && Pawn->GetWeapons()->CanSprint();
}
void UFPSMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
    Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
    const bool bSprinting = IsSprinting();
    if (bWasSprinting != bSprinting)
    {
        bWasSprinting = bSprinting;
        if (auto* Pawn = Cast<AFPSDemoCharacter>(CharacterOwner)) Pawn->GetWeapons()->OnFireReadinessChanged.Broadcast();
    }
}
float UFPSMovementComponent::GetMaxSpeed() const
{
    const auto* Pawn = Cast<AFPSDemoCharacter>(CharacterOwner);
    if (!Pawn || !Pawn->GetWeapons() || !Pawn->GetWeapons()->IsEnabled()) return Super::GetMaxSpeed();
    const float Walk = Pawn->GetAbilitySystemComponent()->GetNumericAttribute(UFPSDemoAttributeSet::GetMoveSpeedAttribute());
    if (IsCrouching()) return Walk * .5f;
    if (Pawn->GetWeapons()->GetLoadout().bAiming) return Walk * .65f;
    return Walk * (IsSprinting() ? 1.5f : 1.f);
}
void UFPSMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
    Super::UpdateFromCompressedFlags(Flags);
    bWantsSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}
FNetworkPredictionData_Client* UFPSMovementComponent::GetPredictionData_Client() const
{
    if (!ClientPredictionData) const_cast<UFPSMovementComponent*>(this)->ClientPredictionData = new FFPSPrediction(*this);
    return ClientPredictionData;
}
