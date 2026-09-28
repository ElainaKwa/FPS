#include "NetLessonGameMode.h"
#include "NetLessonCharacter.h"
#include "NetLessonGameState.h"
#include "NetLessonHUD.h"
#include "NetLessonPlayerController.h"
#include "NetLessonPlayerState.h"

ANetLessonGameMode::ANetLessonGameMode()
{
	DefaultPawnClass = ANetLessonCharacter::StaticClass();
	HUDClass = ANetLessonHUD::StaticClass();
	GameStateClass = ANetLessonGameState::StaticClass();
	PlayerControllerClass = ANetLessonPlayerController::StaticClass();
	PlayerStateClass = ANetLessonPlayerState::StaticClass();
}
