#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FPSBotSettings.generated.h"

/** AI/map tuning only; weapon ability constraints and difficulty damage remain separate. */
UCLASS(Config=Game, DefaultConfig)
class MCP_TEST_API UFPSBotSettings : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditAnywhere, Category="Navigation") TArray<FVector> PatrolPoints;
    UPROPERTY(Config, EditAnywhere, Category="Sensing") float SenseRange = 3200.f;
    UPROPERTY(Config, EditAnywhere, Category="Combat") float EngageDistance = 1450.f;
    UPROPERTY(Config, EditAnywhere, Category="Navigation") float ArrivalDistance = 180.f;
    UPROPERTY(Config, EditAnywhere, Category="Navigation") float PickupStandOff = 125.f;
    bool IsValid() const
    {
        if (PatrolPoints.IsEmpty()) return false;
        for (const auto& Point : PatrolPoints) if (Point.ContainsNaN()) return false;
        for (float Value : {SenseRange, EngageDistance, ArrivalDistance, PickupStandOff})
            if (!FMath::IsFinite(Value) || Value <= 0) return false;
        return true;
    }
};
