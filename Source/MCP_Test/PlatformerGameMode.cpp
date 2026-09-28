#include "PlatformerGameMode.h"

#include "PlatformerCharacter.h"

APlatformerGameMode::APlatformerGameMode()
{
	DefaultPawnClass = APlatformerCharacter::StaticClass();
}
