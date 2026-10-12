#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSNavigationRegion.generated.h"

/** Optional per-level configuration for the existing single ground-plane bot navigation. */
UCLASS()
class MCP_TEST_API AFPSNavigationRegion : public AActor
{
    GENERATED_BODY()
public:
    AFPSNavigationRegion();
    UPROPERTY(EditAnywhere, Category="Navigation") FVector2D GridOrigin = FVector2D(-2300, -1500);
    UPROPERTY(EditAnywhere, Category="Navigation") FIntPoint GridSize = FIntPoint(47, 31);
    UPROPERTY(EditAnywhere, Category="Navigation") TArray<FVector> PatrolPoints;
    bool IsValidLayout() const;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    bool bChecked = false;
    bool bFailed = false;
    int32 CaptureIndex = 0;
    TMap<TWeakObjectPtr<AController>, FVector> InitialPositions;
    TSet<TWeakObjectPtr<AController>> ActiveBots;
};
