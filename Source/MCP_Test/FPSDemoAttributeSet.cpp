#include "FPSDemoAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UFPSDemoAttributeSet::UFPSDemoAttributeSet()
{
	InitMaxHealth(100.0f);
	InitHealth(100.0f);
	InitWeaponDamage(25.f);
	InitMoveSpeed(570.f);
}

void UFPSDemoAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSDemoAttributeSet, WeaponDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSDemoAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSDemoAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSDemoAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UFPSDemoAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetWeaponDamageAttribute()) NewValue = FMath::Clamp(NewValue, 0.f, 1000.f);
	if (Attribute == GetMoveSpeedAttribute()) NewValue = FMath::Clamp(NewValue, 0.f, 1500.f);
	if (Attribute == GetHealthAttribute()) NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	if (Attribute == GetMaxHealthAttribute()) NewValue = FMath::Max(1.0f, NewValue);
}

void UFPSDemoAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
}

void UFPSDemoAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSDemoAttributeSet, Health, OldHealth);
}

void UFPSDemoAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSDemoAttributeSet, MaxHealth, OldMaxHealth);
}

void UFPSDemoAttributeSet::OnRep_WeaponDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSDemoAttributeSet, WeaponDamage, OldValue);
}

void UFPSDemoAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSDemoAttributeSet, MoveSpeed, OldValue);
}
