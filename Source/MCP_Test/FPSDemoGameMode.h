#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPSDemoGameMode.generated.h"

class UFPSLifeComponent;
struct FFPSDeathContext;

/** Exists only on the server: rules, score decisions and respawn. */
UCLASS()
class MCP_TEST_API AFPSDemoGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPSDemoGameMode();
	virtual void HandleLifeEnded(UFPSLifeComponent* Life, const FFPSDeathContext& Context);
};
