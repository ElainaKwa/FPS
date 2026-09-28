#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSAvatarVisuals.generated.h"

class AFPSDemoCharacter;
class USceneComponent;
class UStaticMeshComponent;

/** Cosmetic rigid-joint rig. Consumes movement/weapon state, never drives gameplay. */
UCLASS()
class MCP_TEST_API UFPSAvatarVisuals : public UActorComponent
{
    GENERATED_BODY()
public:
    void UpdatePose(AFPSDemoCharacter* Character, USceneComponent* Camera,
        USceneComponent* Gun, UStaticMeshComponent* WorldGun, float DeltaSeconds);
private:
    bool Build(AFPSDemoCharacter* Character, USceneComponent* Camera);
    void Segment(const FString& Name, FVector Start, FVector End);
    UStaticMeshComponent* Part(const FString& Name) const;
    UPROPERTY(Transient) TMap<FString, TObjectPtr<UStaticMeshComponent>> Parts;
    bool bBuilt = false;
    bool bBuildFailed = false;
    float Gait = 0;
    float CrouchBlend = 0;
};
