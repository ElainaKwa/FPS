#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffect.h"
#include "FPSBaseMembership.generated.h"

/** One sustained, source-owned effect per valid region. Never removes tags by name. */
UCLASS()
class MCP_TEST_API UFPSBaseProtectionEffect : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UFPSBaseProtectionEffect();
};

/** Per-life permission history; positional protection has a separate lifetime. */
UCLASS()
class MCP_TEST_API UFPSBaseMembership : public UActorComponent
{
    GENERATED_BODY()
public:
    UFPSBaseMembership();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Delta, ELevelTick TickType, FActorComponentTickFunction* Function) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Props) const override;
    void RefreshAuthority();
    bool CanSelect() const;
    bool IsProtected() const;
    FGuid GetLifeId() const { return LifeId; }
    void EnforceMovement(const FVector& Previous);
    static bool BlocksAttack(const AActor* Actor);
    static bool BlocksDrop(const AActor* Actor);
    static bool BlocksHostileDamage(const AActor* Target, const FGameplayEffectContextHandle& Context);
    static bool ClipAttack(UWorld* World, const FVector& Start, FVector& End);
private:
    UPROPERTY(Replicated) FGuid LifeId;
    UPROPERTY(ReplicatedUsing=OnRep_Protection) bool bProtected = false;
    UPROPERTY(Replicated) bool bSelectionAllowed = false;
    UFUNCTION() void OnRep_Protection();
    TWeakObjectPtr<class AFPSBaseRegion> SpawnRegion;
    TMap<TWeakObjectPtr<class AFPSBaseRegion>, FActiveGameplayEffectHandle> Effects;
    bool bInitialized = false, bHasExited = false;
};
