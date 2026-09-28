// GameMode for the platformer: spawns APlatformerCharacter as the default pawn.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PlatformerGameMode.generated.h"

UCLASS()
class MCP_TEST_API APlatformerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APlatformerGameMode();
};
