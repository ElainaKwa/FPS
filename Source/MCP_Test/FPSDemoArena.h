#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSDemoArena.generated.h"

/** Entire lesson arena is modeled in C++ so the map remains easy to inspect. */
UCLASS()
class MCP_TEST_API AFPSDemoArena : public AActor
{
	GENERATED_BODY()

public:
	AFPSDemoArena();
};
