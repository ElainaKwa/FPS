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
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
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

	// Current equipment display cache only. Effects modifying this attribute do not modify shot damage.
	// Authoritative shots capture definition damage and apply it through a GAS SetByCaller effect.
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

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Stamina, Category="FPS Demo")
	FGameplayAttributeData Stamina;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UFPSDemoAttributeSet, Stamina)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(Stamina)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(Stamina)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(Stamina)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxStamina, Category="FPS Demo")
	FGameplayAttributeData MaxStamina;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UFPSDemoAttributeSet, MaxStamina)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxStamina)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(MaxStamina)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxStamina)
	UFUNCTION() void OnRep_Stamina(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);

protected:

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
};
