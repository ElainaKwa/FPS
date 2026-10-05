#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FPSWeaponDataCommandlet.generated.h"

/** Editor-only migration, strict content gate and deterministic one-way review export. */
UCLASS()
class MCP_TEST_API UFPSWeaponDataCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UFPSWeaponDataCommandlet();
    virtual int32 Main(const FString& Params) override;
};
