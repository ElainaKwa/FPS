#include "FPSDemoEffects.h"
#include "FPSDemoAttributeSet.h"

UFPSWeaponDamageEffect::UFPSWeaponDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UFPSDemoAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	FSetByCallerFloat Caller;
	Caller.DataName = TEXT("WeaponDamage");
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Caller);
	Modifiers.Add(Modifier);
}

UFPSDemoDamageEffect::UFPSDemoDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UFPSDemoAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-25.0f));
	Modifiers.Add(Modifier);
}

UFPSDemoHealEffect::UFPSDemoHealEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UFPSDemoAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(35.0f));
	Modifiers.Add(Modifier);
}
