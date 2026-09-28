#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FPSBotNavigation.generated.h"

/** Shared ground-plane navigation for the authored flat FPS maps. Server only. */
UCLASS()
class MCP_TEST_API UFPSBotNavigation : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    bool FindRoute(const FVector& Start, const FVector& Goal, TArray<FVector>& OutPoints);
    bool ClearSegment(const FVector& Start, const FVector& End) const;
private:
    void BuildGrid();
    int32 NearestCell(const FVector& Point) const;
    FVector Cell(int32 Index) const;
    TArray<uint8> Walkable;
    bool bBuilt = false;
    static constexpr int32 Width = 47;
    static constexpr int32 Height = 31;
};
