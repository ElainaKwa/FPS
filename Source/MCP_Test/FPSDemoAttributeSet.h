#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "FPSDemoAttributeSet.generated.h"

/** GAS attributes: the server changes Health through GameplayEffects. */
UCLASS()
class MCP_TEST_API UFPSDemoAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UFPSDemoAttributeSet();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="FPS Demo")
	FGameplayAttributeData Health;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UFPSDemoAttributeSet, Health)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(Health)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(Health)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, Category="FPS Demo")
	FGameplayAttributeData MaxHealth;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UFPSDemoAttributeSet, MaxHealth)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxHealth)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(MaxHealth)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxHealth)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_WeaponDamage, Category="FPS Demo")
	FGameplayAttributeData WeaponDamage;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UFPSDemoAttributeSet, WeaponDamage)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(WeaponDamage)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(WeaponDamage)
	UFUNCTION() void OnRep_WeaponDamage(const FGameplayAttributeData& OldValue);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MoveSpeed, Category="FPS Demo")
	FGameplayAttributeData MoveSpeed;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UFPSDemoAttributeSet, MoveSpeed)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(MoveSpeed)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(MoveSpeed)
	UFUNCTION() void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);

protected:

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
};
