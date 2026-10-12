#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FPSInputSettings.generated.h"

/** Local control preferences. No gameplay state or server-owned values are saved. */
UCLASS(Config=GameUserSettings)
class MCP_TEST_API UFPSInputSettings : public UObject
{
    GENERATED_BODY()
public:
    static UFPSInputSettings* Get();
    UPROPERTY(Config) bool bSprintToggle = false;
    UPROPERTY(Config) bool bCrouchToggle = false;
    void SetSprintToggle(bool bEnabled);
    void SetCrouchToggle(bool bEnabled);
private:
    void SaveLocal();
};
