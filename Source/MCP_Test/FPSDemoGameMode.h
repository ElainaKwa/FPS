#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPSDemoGameMode.generated.h"

class AFPSDemoCharacter;

/** Exists only on the server: rules, score decisions and respawn. */
UCLASS()
class MCP_TEST_API AFPSDemoGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPSDemoGameMode();
	void HandleElimination(AFPSDemoCharacter* Victim, AController* Attacker);
};
