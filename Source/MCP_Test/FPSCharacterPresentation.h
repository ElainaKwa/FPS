#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSWeaponVisualTiming.h"
#include "FPSProceduralAnimation.h"
#include "FPSCharacterPresentation.generated.h"
class AFPSDemoCharacter;
/** Camera/weapon presentation plus shared action-pose input for the avatar and authority hit bodies. */
UCLASS()
class MCP_TEST_API UFPSCharacterPresentation : public UActorComponent
{
    GENERATED_BODY()
public:
    void Initialize(AFPSDemoCharacter* Character);
    void Update(AFPSDemoCharacter* Character, float DeltaSeconds);
    void Refresh(AFPSDemoCharacter* Character);
    void LocalShot(double Now) { Recoil = 1.f; LocalFlashUntil = Now + .07; }
    void RemoteShot(double Now) { Recoil = 1.f; RemoteFlashUntil = Now + .08; }
    void WeaponKick(float Kick, double Now) { Recoil = RecoilKick = Kick; RecoilStartedAt = Now; }
    float GetRecoil() const { return Recoil; }
    float GetAimBlend(double Now) const { return AimVisualBlend.Value(Now); }
    FVector GetAmbientOffset() const { return AmbientOffset; }
private:
    FPSProceduralAnimation::FActionPose ActionPoseFor(const UFPSWeaponDefinition* Definition, const FFPSWeaponLoadout& State, double Now, float AimAlpha);
    UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> FirstPersonAttachments;
    UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> ThirdPersonAttachments;
    UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> FirstPersonReloadParts;
    UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> ThirdPersonReloadParts;
    FPSWeaponVisualTiming::FTransition AimVisualBlend, CameraFOVBlend{92.f}, SprintVisualBlend;
    FPSWeaponVisualTiming::FTransition AmbientBlend{1.f};
    FPSProceduralAnimation::FLocomotion Locomotion;
    FPSProceduralAnimation::FActionBlend ActionBlend;
    FVector AmbientOffset = FVector::ZeroVector;
    FGuid PresentedInstance;
    float RecoilKick = 0;
    double RecoilStartedAt = -100;
    float Recoil = 0;
    double LocalFlashUntil = 0;
    double RemoteFlashUntil = 0;
};
