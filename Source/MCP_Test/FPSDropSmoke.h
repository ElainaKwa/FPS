#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSWeaponData.h"
#include "FPSDropSmoke.generated.h"

class AFPSMatchMode;
class AFPSFrontController;
class AFPSWeaponPickup;
class UPrimitiveComponent;

UCLASS()
class AFPSDropSmokeWall : public AActor
{
    GENERATED_BODY()
public:
    AFPSDropSmokeWall();
};

/** Opt-in fixture facts; clients assert their own replicated actors and inventory. */
UCLASS()
class AFPSDropSmokeState : public AActor
{
    GENERATED_BODY()
public:
    AFPSDropSmokeState();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(Replicated) double Epoch = 0;
    UPROPERTY(Replicated) FFPSWeaponInstance CooldownReference;
    UPROPERTY(Replicated) FGuid SettledGuid;
    UPROPERTY(Replicated) FVector SettledPosition = FVector::ZeroVector;
    UPROPERTY(Replicated) bool bSettled = false;
    UPROPERTY(Replicated) bool bWallHit = false;
    UPROPERTY(Replicated) bool bLateReady = false;
    UPROPERTY(Replicated) FGuid RaceGuid;
    UPROPERTY() TObjectPtr<AFPSWeaponPickup> RaceCandidate;
    UPROPERTY(Replicated) bool bRaceResolved = false;
    UPROPERTY(Replicated) bool bClientWonRace = false;
    UPROPERTY(Replicated) FGuid SurvivingGuid;
    UPROPERTY(Replicated) FGuid DyingGuid;
    UPROPERTY(Replicated) FGuid LifetimeGuid;
    UPROPERTY(Replicated) FGuid ReacquireGuid;
    UPROPERTY(Replicated) FFPSWeaponInstance ReacquireDropped;
    UPROPERTY(Replicated) FFPSWeaponInstance ReacquireReference;
    UPROPERTY(Replicated) FFPSWeaponInstance CoalescedReference;
    UPROPERTY(Replicated) uint32 CoalescedRevision = 0;
    UPROPERTY(Replicated) int32 PresetCount = 0;
    UFUNCTION() void OnFixtureHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
};

struct FFPSDropSmoke
{
    static void Start(AFPSMatchMode* Mode);
    static void Tick(AFPSFrontController* Controller);
};
