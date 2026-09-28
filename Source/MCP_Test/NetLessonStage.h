#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetLessonStage.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/** The three physical stations used by the network replication lesson. */
UCLASS()
class MCP_TEST_API ANetLessonStage : public AActor
{
	GENERATED_BODY()

public:
	ANetLessonStage();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ServerIncrementCounter();
	void ServerBroadcastPulse();
	int32 GetCounter() const { return Counter; }
	float GetPulseUntil() const { return PulseUntil; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Floor;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ReplicationStation;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MulticastStation;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> LocalStation;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Title;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> CounterLabel;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> MulticastLabel;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> LocalLabel;

	UPROPERTY(ReplicatedUsing=OnRep_Counter)
	int32 Counter = 0;

	UFUNCTION()
	void OnRep_Counter();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPulse();

private:
	void ResetPulseColor();
	FTimerHandle PulseTimer;
	float PulseUntil = 0.0f;
};
