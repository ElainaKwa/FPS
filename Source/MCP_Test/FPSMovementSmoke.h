#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSMovementSmoke.generated.h"

class AFPSMatchMode;
class AFPSFrontController;
class AFPSDemoCharacter;
class UBoxComponent;

/** Opt-in, replicated collision fixture. No saved map or asset is changed. */
UCLASS()
class AFPSMovementSmokeBox : public AActor
{
    GENERATED_BODY()
public:
    AFPSMovementSmokeBox();
    void Configure(const FVector& Extent);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY(ReplicatedUsing=OnRep_Extent) FVector BoxExtent = FVector(100);
    UPROPERTY() TObjectPtr<UBoxComponent> Box;
    UFUNCTION() void OnRep_Extent();
};

USTRUCT()
struct FFPSMovementSmokeReference
{
    GENERATED_BODY()
    UPROPERTY() int32 PlayerId = -1;
    UPROPERTY() float Stamina = 0;
    UPROPERTY() float MaxStamina = 100;
    UPROPERTY() bool Recovering = false;
    UPROPERTY() float RecoveryRemaining = 0;
    UPROPERTY() float NativeMoveTimeStamp = -1;
    UPROPERTY() bool SprintIntent = false;
    UPROPERTY() uint32 JumpCharges = 0;
    UPROPERTY() uint32 CrouchCharges = 0;
    UPROPERTY() double SprintSeconds = 0;
};

/** Facts only: each endpoint measures its own movement, inputs and GAS inventory. */
UCLASS()
class AFPSMovementSmokeState : public AActor
{
    GENERATED_BODY()
public:
    AFPSMovementSmokeState();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(Replicated) int32 Scene = -1;
    UPROPERTY(Replicated) double StartsAt = 0;
    UPROPERTY(Replicated) int32 ReferenceScene = -1;
    UPROPERTY(Replicated) double ReferenceAt = 0;
    UPROPERTY(Replicated) TArray<FFPSMovementSmokeReference> References;
    UPROPERTY() TObjectPtr<AFPSMovementSmokeBox> Floor;
    UPROPERTY() TObjectPtr<AFPSMovementSmokeBox> Wall;
    UPROPERTY() TObjectPtr<AFPSMovementSmokeBox> Roof;
};

struct FFPSMovementSmoke
{
    static void Start(AFPSMatchMode* Mode);
    static void Tick(AFPSFrontController* Controller, float DeltaSeconds);
    static void TickTeaching(AFPSDemoCharacter* Character, float DeltaSeconds);
    static void TickInputUI(AFPSFrontController* Controller);
};
