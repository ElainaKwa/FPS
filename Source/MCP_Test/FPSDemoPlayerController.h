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
    UFUNCTION(Server,Reliable) void ServerSettingsTest(int32 Stage);
	virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void OpenSettings(class UUserWidget* Source=nullptr);
    bool IsSettingsOpen() const { return SettingsUI!=nullptr; }
    void SettingsBack();
    void CloseSettingsForTravel();
    void UseGameplayInput();
    void UseMenuInput(class UUserWidget* Focus);
	void ConfirmHit(bool bElimination, bool bHeadshot = false);
    bool WasHeadshot() const { return bLastHitHeadshot; }
	float GetKillMarkerUntil() const { return KillMarkerUntil; }
	float GetDamageFlash() const;
	float GetHitMarkerUntil() const { return HitMarkerUntil; }
	bool WasElimination() const { return bLastHitEliminated; }

protected:
    virtual bool LockMenuMouseToViewport() const { return false; }
private:
    friend struct FFPSSettingsSmoke;
    UPROPERTY() TObjectPtr<class UFPSSettingsUI> SettingsUI;
    TWeakObjectPtr<class UUserWidget> SettingsSource;
    bool bSettingsWasBlocked=false;
	UFUNCTION(Client, Reliable)
	void ClientConfirmHit(bool bElimination, bool bHeadshot);
    bool bLastHitHeadshot = false;
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
