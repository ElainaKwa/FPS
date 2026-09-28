#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FPSMovementComponent.generated.h"

UCLASS()
class MCP_TEST_API UFPSMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()
public:
    bool bWantsSprint = false;
    bool IsSprinting() const;
    virtual float GetMaxSpeed() const override;
    virtual void UpdateFromCompressedFlags(uint8 Flags) override;
    virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
};
