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
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    void Initialize(const FFPSWeaponInstance& Value);
    const FFPSWeaponInstance& GetInstance() const { return Instance; }
    bool IsAvailable() const { return !bClaimed && Instance.IsValid(); }
    bool Claim();
private:
    UPROPERTY() FFPSWeaponInstance Instance;
    UPROPERTY(ReplicatedUsing=OnRep_Instance) FFPSPickupDisplay Display;
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<class UTextRenderComponent> Label;
    UFUNCTION() void OnRep_Instance();
    bool bClaimed = false;
};
