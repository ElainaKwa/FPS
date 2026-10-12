#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FPSStaminaPresentation.h"
#include "FPSDemoHUD.generated.h"

UCLASS()
class MCP_TEST_API AFPSDemoHUD : public AHUD
{
	GENERATED_BODY()
public:
    AFPSDemoHUD();
	virtual void DrawHUD() override;
private:
    void DrawMovementHUD(const class AFPSDemoCharacter* Pawn);
    FFPSStaminaPresentation StaminaPresentation;
    TWeakObjectPtr<const class AFPSDemoCharacter> StaminaOwner;
    UPROPERTY() TObjectPtr<class UFPSInterface> HUDAsset;
    UPROPERTY() TObjectPtr<class UFont> RuntimeFont;
    UPROPERTY() TObjectPtr<class UFont> MovementFont;
};
