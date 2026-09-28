#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPSDemoPlayerController.generated.h"

/** Owner-only hit confirmation contrasts with all-player GameState/PlayerState. */
UCLASS()
class MCP_TEST_API AFPSDemoPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaSeconds) override;
	void ConfirmHit(bool bElimination);
	float GetKillMarkerUntil() const { return KillMarkerUntil; }
	float GetDamageFlash() const;
	float GetHitMarkerUntil() const { return HitMarkerUntil; }
	bool WasElimination() const { return bLastHitEliminated; }

private:
	UFUNCTION(Client, Reliable)
	void ClientConfirmHit(bool bElimination);
	float HitMarkerUntil = 0.0f;
	float KillMarkerUntil = 0.0f;
	float DamageFlashUntil = 0.0f;
	float PreviousHealth = 0.0f;
	TWeakObjectPtr<class AFPSDemoCharacter> ObservedPawn;
	bool bReportedDeath = false;
	bool bAwaitingRespawn = false;
	bool bCapturedDeath = false;
	bool bLastHitEliminated = false;
};
