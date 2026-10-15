#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSProceduralAnimation.h"
#include "FPSAvatarVisuals.generated.h"

class AFPSDemoCharacter;
class USceneComponent;
class UStaticMeshComponent;

/** Shared rigid pose: visual meshes on rendered worlds, lightweight pose nodes on dedicated servers. */
UCLASS()
class MCP_TEST_API UFPSAvatarVisuals : public UActorComponent
{
    GENERATED_BODY()
public:
    void UpdatePose(AFPSDemoCharacter* Character, USceneComponent* Camera,
        USceneComponent* Gun, UStaticMeshComponent* WorldGun, float DeltaSeconds,
        const FPSProceduralAnimation::FActionPose& ActionPose);
private:
    bool Build(AFPSDemoCharacter* Character, USceneComponent* Camera);
    void Segment(const FString& Name, FVector Start, FVector End, FVector Up = FVector::ZeroVector);
    USceneComponent* Part(const FString& Name) const;
    UPROPERTY(Transient) TMap<FString, TObjectPtr<USceneComponent>> Parts;
    bool bBuilt = false;
    bool bBuildFailed = false;
    float Gait = 0;
    float CrouchBlend = 0;
    float SprintBlend = 0;
    FPSProceduralAnimation::FSpring BodyMotion;
    float AirBlend = 0;
};
