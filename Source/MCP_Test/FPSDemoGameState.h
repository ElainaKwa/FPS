#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "FPSDemoGameState.generated.h"

USTRUCT()
struct FFPSResult
{
	GENERATED_BODY()
	UPROPERTY() FString Name;
	UPROPERTY() int32 Kills = 0;
	UPROPERTY() int32 Deaths = 0;
	UPROPERTY() int32 PlayerId = 0;
};

UCLASS()
class MCP_TEST_API AFPSDemoGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	// Used by FPSMatchMode only. The teaching map leaves bStructuredMatch false.
	UPROPERTY(Replicated) bool bStructuredMatch = false;
	UPROPERTY(Replicated) bool bTraining = false;
	UPROPERTY(Replicated) bool bWaiting = false;
	UPROPERTY(Replicated) bool bFinished = false;
	UPROPERTY(Replicated) double RoundEndsAt = 0;
	UPROPERTY(Replicated) FString RoomName;
	UPROPERTY(Replicated) FString WeaponDataError;
	UPROPERTY(Replicated) TArray<FFPSResult> Podium;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void RecordElimination(const FString& AttackerName, const FString& VictimName);
	int32 GetEliminations() const { return Eliminations; }
	const FString& GetLastEvent() const { return LastEvent; }

private:
	UPROPERTY(Replicated)
	int32 Eliminations = 0;
	UPROPERTY(Replicated)
	FString LastEvent;
};
