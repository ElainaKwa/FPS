#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FPSMovementSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="FPS Movement"))
class MCP_TEST_API UFPSMovementSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditAnywhere) float WalkSpeed = 480.f;
    UPROPERTY(Config, EditAnywhere) float SprintSpeed = 720.f;
    UPROPERTY(Config, EditAnywhere) float CrouchSpeed = 240.f;
    UPROPERTY(Config, EditAnywhere) float AimSpeed = 312.f;
    UPROPERTY(Config, EditAnywhere) float SprintAccelerationTime = .25f;
    UPROPERTY(Config, EditAnywhere) float SprintDecelerationTime = .15f;
    UPROPERTY(Config, EditAnywhere) float WalkAcceleration = 9600.f;
    UPROPERTY(Config, EditAnywhere) float AirAcceleration = 1200.f;
    UPROPERTY(Config, EditAnywhere) float JumpVelocity = 500.f;
    UPROPERTY(Config, EditAnywhere) float WeakJumpHeightRatio = .5f;
    UPROPERTY(Config, EditAnywhere) float LandingJumpInterval = .2f;
    UPROPERTY(Config, EditAnywhere) float MaxStamina = 100.f;
    UPROPERTY(Config, EditAnywhere) float SprintDrain = 6.f;
    UPROPERTY(Config, EditAnywhere) float SprintStartThreshold = 20.f;
    UPROPERTY(Config, EditAnywhere) float JumpCost = 20.f;
    UPROPERTY(Config, EditAnywhere) float CrouchCost = 10.f;
    UPROPERTY(Config, EditAnywhere) float CrouchInterval = .35f;
    UPROPERTY(Config, EditAnywhere) float LowCrouchInterval = .8f;
    UPROPERTY(Config, EditAnywhere) float RecoveryDelay = 1.5f;
    UPROPERTY(Config, EditAnywhere) float RecoveryRate = 25.f;
    UPROPERTY(Config, EditAnywhere) float SprintFirePreparation = .15f;
};
