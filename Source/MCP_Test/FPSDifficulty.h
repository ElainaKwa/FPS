#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FPSDifficulty.generated.h"

UENUM()
enum class EFPSDifficulty : uint8 { Easy, Normal, Hard };

/** Match rule configuration, independent of weapon definitions and instances. */
UCLASS(Config=Game, DefaultConfig)
class MCP_TEST_API UFPSDifficultySettings : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(Config) float EasyMultiplier = .5f;
    UPROPERTY(Config) float NormalMultiplier = .65f;
    UPROPERTY(Config) float HardMultiplier = .85f;
    float Multiplier(EFPSDifficulty Value) const
    {
        return Value == EFPSDifficulty::Easy ? EasyMultiplier : Value == EFPSDifficulty::Hard ? HardMultiplier : NormalMultiplier;
    }
};
