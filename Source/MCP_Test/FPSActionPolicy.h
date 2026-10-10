#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSActionPolicy.generated.h"

/** Shared life/phase/tag permission; weapon operations add their own readiness constraints. */
UCLASS(ClassGroup=(FPS), meta=(BlueprintSpawnableComponent))
class MCP_TEST_API UFPSActionPolicy : public UActorComponent
{
    GENERATED_BODY()
public:
    bool CanAct() const;
    bool CanMove() const;
    bool CanAttack() const;
    bool CanDrop() const;
};
