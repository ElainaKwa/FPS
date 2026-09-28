#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSWeaponData.h"
#include "FPSWeaponPickup.generated.h"

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
    UPROPERTY(ReplicatedUsing=OnRep_Instance) FFPSWeaponInstance Instance;
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<class UTextRenderComponent> Label;
    UFUNCTION() void OnRep_Instance();
    bool bClaimed = false;
};
