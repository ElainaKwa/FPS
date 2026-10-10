#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSWeaponVisualTiming.h"
#include "FPSCharacterPresentation.generated.h"
class AFPSDemoCharacter;
/** Cosmetic orchestrator: camera and first-person gun; delegates third-person pose to AvatarVisuals. */
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
private:
    FPSWeaponVisualTiming::FTransition AimVisualBlend, CameraFOVBlend{92.f}, SprintVisualBlend;
    float RecoilKick = 0;
    double RecoilStartedAt = -100;
    float Recoil = 0;
    double LocalFlashUntil = 0;
    double RemoteFlashUntil = 0;
};
