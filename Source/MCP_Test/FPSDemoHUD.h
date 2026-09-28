#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FPSDemoHUD.generated.h"

UCLASS()
class MCP_TEST_API AFPSDemoHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
};
