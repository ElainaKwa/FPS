#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "FPSFrontGameInstance.generated.h"

UCLASS()
class MCP_TEST_API UFPSFrontGameInstance : public UGameInstance
{
    GENERATED_BODY()
public:
    virtual void Init() override;
    virtual void Shutdown() override;
    FString ConnectionError;
private:
    void NetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Message);
    void TravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Message);
};
