#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSBaseRegion.generated.h"

class UBoxComponent;
class APlayerStart;
/** Geometry, ownership and server spawn validation; membership owns effect sources. */
UCLASS()
class MCP_TEST_API AFPSBaseRegion : public AActor
{
    GENERATED_BODY()
public:
    AFPSBaseRegion();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UBoxComponent> Bounds;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TeamId = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TObjectPtr<APlayerStart>> Starts;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<APlayerStart> FallbackStart;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="128")) int32 SpawnAttempts = 32;
    bool Contains(const FVector& Location, float Tolerance = 0) const;
    bool SegmentEntry(const FVector& Start, const FVector& End, const FVector& Padding, float& Entry) const;
    FVector OutsidePosition(const FVector& Location, float Padding) const;
    bool FindSpawn(AController* Controller, FTransform& Result) const;
    static bool ValidateWorld(UWorld* World, TArray<FString>& Errors);
private:
    UPROPERTY() TObjectPtr<class UTextRenderComponent> Label;
    UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> BoundaryMarkers;
};
