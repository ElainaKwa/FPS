#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSHitDamage.h"
#include "FPSHitRegions.generated.h"
class UBoxComponent;

/** Query-only bodies consume the same rigid pose as the third-person avatar. */
UCLASS()
class MCP_TEST_API UFPSHitRegions : public UActorComponent
{
    GENERATED_BODY()
public:
    static constexpr ECollisionChannel ShotChannel = ECC_GameTraceChannel1;
    void BindPose(const TMap<FString, TObjectPtr<USceneComponent>>& Nodes);
    void SetEnabled(bool bEnabled);
    EFPSHitRegion Classify(const FHitResult& Hit) const;
    UBoxComponent* FindBody(FName Name) const;
private:
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UBoxComponent>> Bodies;
    TMap<FName, EFPSHitRegion> Regions;
};
