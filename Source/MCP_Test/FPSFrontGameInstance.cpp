#include "FPSFrontGameInstance.h"
#include "Engine/Engine.h"

void UFPSFrontGameInstance::Init()
{
    Super::Init();
    GEngine->OnNetworkFailure().AddUObject(this, &UFPSFrontGameInstance::NetworkFailure);
    GEngine->OnTravelFailure().AddUObject(this, &UFPSFrontGameInstance::TravelFailure);
}

void UFPSFrontGameInstance::Shutdown()
{
    GEngine->OnNetworkFailure().RemoveAll(this);
    GEngine->OnTravelFailure().RemoveAll(this);
    Super::Shutdown();
}

void UFPSFrontGameInstance::NetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Message)
{
    if (World && World->GetGameInstance() != this) return;
    ConnectionError = TEXT("连接失败或房主已关闭房间：") + Message;
    // The engine's network failure handler returns clients to GameDefaultMap.
}

void UFPSFrontGameInstance::TravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Message)
{
    if (World && World->GetGameInstance() != this) return;
    ConnectionError = TEXT("无法进入地图：") + Message;
}
