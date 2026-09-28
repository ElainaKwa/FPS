#pragma once
#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "FPSScoreSet.generated.h"

/** Owned by PlayerState: score persists across pawn death and respawn. */
UCLASS()
class MCP_TEST_API UFPSScoreSet : public UAttributeSet
{
    GENERATED_BODY()
public:
    UPROPERTY(ReplicatedUsing=OnRep_Points, BlueprintReadOnly) FGameplayAttributeData Points;
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UFPSScoreSet, Points)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(Points)
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION() void OnRep_Points(const FGameplayAttributeData& OldValue);
};

UCLASS()
class MCP_TEST_API UFPSKillScoreEffect : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UFPSKillScoreEffect();
};
