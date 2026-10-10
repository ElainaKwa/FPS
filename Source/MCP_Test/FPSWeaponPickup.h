#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSWeaponData.h"
#include "FPSWeaponPickup.generated.h"

USTRUCT()
struct FFPSPickupDisplay
{
    GENERATED_BODY()
    UPROPERTY() FGuid InstanceId;
    UPROPERTY() FName WeaponId;
    UPROPERTY() int32 Magazine = 0;
    UPROPERTY() int32 Reserve = 0;
};

UCLASS()
class MCP_TEST_API AFPSWeaponPickup : public AActor
{
    GENERATED_BODY()
public:
    AFPSWeaponPickup();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    void Initialize(const FFPSWeaponInstance& Value);
    bool LaunchDynamic(const FVector& Velocity, const FVector& AngularVelocity);
    bool IsDynamic() const { return bDynamic; }
    const FFPSWeaponInstance& GetInstance() const { return Instance; }
    bool IsAvailable() const { return !bClaimed && Instance.IsValid(); }
    bool Claim();
    void ReleaseClaim();
private:
    friend struct FFPSDropSmoke;
    UPROPERTY() FFPSWeaponInstance Instance;
    UPROPERTY(ReplicatedUsing=OnRep_Instance) FFPSPickupDisplay Display;
    UPROPERTY(ReplicatedUsing=OnRep_Instance) bool bDynamic = false;
    UPROPERTY() TObjectPtr<class UBoxComponent> Body;
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<class UTextRenderComponent> Label;
    UFUNCTION() void OnRep_Instance();
    bool bClaimed = false;
    double SleepingSince = -1;
};
