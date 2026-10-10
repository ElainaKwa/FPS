#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FPSDynamicDrops.generated.h"

class AFPSWeaponPickup;

/** Server-only lifetime/cap ownership. Level stands never enter this registry. */
UCLASS()
class MCP_TEST_API UFPSDynamicDrops : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    bool Register(AFPSWeaponPickup* Drop);
    void Unregister(AFPSWeaponPickup* Drop);
    void ClearRound();
    int32 Count() const;
private:
    TArray<TWeakObjectPtr<AFPSWeaponPickup>> Ordered;
};
