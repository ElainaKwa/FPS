#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "FPSDemoPlayerState.generated.h"

UCLASS()
class MCP_TEST_API AFPSDemoPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AFPSDemoPlayerState();
	virtual void BeginPlay() override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 TeamId = -1;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void AddElimination();
	void AddDeath();
	int32 GetEliminationCount() const;
	int32 GetDeathCount() const { return DeathCount; }

private:
	UPROPERTY() TObjectPtr<UAbilitySystemComponent> ScoreAbilitySystem;
	UPROPERTY() TObjectPtr<class UFPSScoreSet> ScoreAttributes;
	UPROPERTY(Replicated)
	int32 DeathCount = 0;
};
