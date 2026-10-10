#include "FPSActionPolicy.h"
#include "FPSLifeComponent.h"
#include "FPSDemoGameState.h"
#include "FPSMatchFlow.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"
#include "FPSBaseMembership.h"
#include "GameFramework/Pawn.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_BlockMovement, "Character.Restriction.Movement");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_BlockAttack, "Character.Restriction.Attack");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_BlockDrop, "Character.Restriction.Drop");
namespace
{
bool HasRestriction(const AActor* Actor, FGameplayTag Tag)
{
    const auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(Actor));
    return ASC && ASC->HasMatchingGameplayTag(Tag);
}
}
bool UFPSActionPolicy::CanAct() const
{
    const auto* Life = GetOwner()->FindComponentByClass<UFPSLifeComponent>();
    if (!Life || Life->IsDead()) return false;
    const auto* Pawn = Cast<APawn>(GetOwner());
    if (Pawn && !Pawn->GetController()) return false;
    const auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
    if (GS && GS->bStructuredMatch)
    {
        if (!GS->IsCombatActive()) return false;
        if (const auto* PC = Pawn ? Cast<AFPSFrontController>(Pawn->GetController()) : nullptr)
            if (!PC->IsWeaponContentVerified()) return false;
    }
    return true;
}
bool UFPSActionPolicy::CanMove() const { return CanAct() && !HasRestriction(GetOwner(), TAG_BlockMovement); }
bool UFPSActionPolicy::CanAttack() const { return CanAct() && !HasRestriction(GetOwner(), TAG_BlockAttack) && !UFPSBaseMembership::BlocksAttack(GetOwner()); }
bool UFPSActionPolicy::CanDrop() const { return CanAct() && !HasRestriction(GetOwner(), TAG_BlockDrop) && !UFPSBaseMembership::BlocksDrop(GetOwner()); }
