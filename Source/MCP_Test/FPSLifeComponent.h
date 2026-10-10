#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "FPSLifeComponent.generated.h"

class AController;
class APlayerState;
class UFPSLifeComponent;

/** Snapshot at health exhaustion; a causer need not be a pawn or have a controller. */
struct FFPSDeathContext
{
    TWeakObjectPtr<AActor> Instigator;
    TWeakObjectPtr<AActor> Causer;
    TWeakObjectPtr<AController> Attacker;
    TWeakObjectPtr<APlayerState> AttackerState;
    FString SourceName = TEXT("World");
    static FFPSDeathContext FromEffect(const FGameplayEffectContextHandle& Effect);
};
DECLARE_MULTICAST_DELEGATE_TwoParams(FFPSLifeEnded, UFPSLifeComponent*, const FFPSDeathContext&);

/** Target-side GAS health/death contract. No inventory, input, humanoid or scoring dependency. */
UCLASS(ClassGroup=(FPS), meta=(BlueprintSpawnableComponent))
class MCP_TEST_API UFPSLifeComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UFPSLifeComponent();
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    bool IsDead() const { return bDead; }
    void NotifyHealthExhausted(const FGameplayEffectContextHandle& Effect);
    FFPSLifeEnded OnLifeEnded;
private:
    UPROPERTY(Replicated) bool bDead = false;
};
