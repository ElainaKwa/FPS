#include "FPSScoreSet.h"
#include "Net/UnrealNetwork.h"

void UFPSScoreSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION_NOTIFY(UFPSScoreSet, Points, COND_None, REPNOTIFY_Always);
}

void UFPSScoreSet::OnRep_Points(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSScoreSet, Points, OldValue);
}

UFPSKillScoreEffect::UFPSKillScoreEffect()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    FGameplayModifierInfo Modifier;
    Modifier.Attribute = UFPSScoreSet::GetPointsAttribute();
    Modifier.ModifierOp = EGameplayModOp::Additive;
    Modifier.ModifierMagnitude = FScalableFloat(1.f);
    Modifiers.Add(Modifier);
}
