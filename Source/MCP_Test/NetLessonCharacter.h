#pragma once

#include "CoreMinimal.h"
#include "PlatformerCharacter.h"
#include "NetLessonCharacter.generated.h"

class UInputAction;
class ANetLessonStage;

/** The owning client sends requests through this replicated Character. */
UCLASS()
class MCP_TEST_API ANetLessonCharacter : public APlatformerCharacter
{
	GENERATED_BODY()

public:
	ANetLessonCharacter();
	float GetLocalEffectUntil() const { return LocalEffectUntil; }

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY()
	TObjectPtr<UInputAction> CounterAction;
	UPROPERTY()
	TObjectPtr<UInputAction> PulseAction;
	UPROPERTY()
	TObjectPtr<UInputAction> LocalAction;

	UFUNCTION(Server, Reliable)
	void ServerRequestCounter();
	UFUNCTION(Server, Reliable)
	void ServerRequestPulse();

private:
	ANetLessonStage* FindStage() const;
	void PressCounter();
	void PressPulse();
	void PressLocal();
	float LocalEffectUntil = 0.0f;
};
