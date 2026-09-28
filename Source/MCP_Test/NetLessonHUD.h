#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NetLessonHUD.generated.h"

UCLASS()
class MCP_TEST_API ANetLessonHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
