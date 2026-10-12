#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/CharacterMovementReplication.h"
#include "FPSMovementComponent.generated.h"

struct FFPSMoveDataContainer;
struct FFPSMoveResponse;
class FFPSMove;

/** Resource state at a CMC move boundary, restored once before pending moves replay. */
struct FFPSMovementResource
{
    float Stamina = 100.f, RecoveryRemaining = 0.f, JumpRemaining = 0.f, CrouchRemaining = 0.f;
    float SpeedLimit = 480.f, AirSpeedLimit = 480.f;
    uint16 SprintSequence = 0, CrouchSequence = 0, JumpSequence = 0;
    uint32 JumpCharges = 0, CrouchCharges = 0;
    bool SprintIntent = false, CrouchIntent = false, WeakJump = false, Crouched = false, ActualSprint = false;
    bool Serialize(FArchive& Ar);
    bool Matches(const FFPSMovementResource& Other) const;
};

UCLASS()
class MCP_TEST_API UFPSMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()
    friend class FFPSMove;
    friend struct FFPSMoveData;
    friend struct FFPSMoveResponse;
    friend struct FFPSMovementSmoke;
public:
    UFPSMovementComponent();
    virtual ~UFPSMovementComponent() override;
    bool bWantsSprint = false;
    bool UsesMovementRules() const;
    bool UsesStamina() const;
    void RequestSprint(bool Pressed);
    void RequestCrouch(bool Pressed);
    void RequestJump();
    void ClearSprintIntent();
    void ClearMovementIntents();
    bool HasSprintIntent() const { return Resource.SprintIntent; }
    bool IsSprinting() const;
    float GetStamina() const;
    float GetMaxStamina() const;
    bool IsRecovering() const;
    bool IsWeakJump() const;
    bool IsSprintStartBlocked() const;
    float GetJumpCooldownRemaining() const { return Resource.JumpRemaining; }
    float GetCrouchCooldownRemaining() const { return Resource.CrouchRemaining; }
    float GetSprintFirePreparationSeconds() const;
    uint32 DebugReplayCount = 0, DebugCorrectionCount = 0;
    uint32 GetJumpCharges() const { return Resource.JumpCharges; }
    uint32 GetCrouchCharges() const { return Resource.CrouchCharges; }
    float DebugSprintConsumedSeconds = 0.f;
    void SetStaminaForTest(float Value);
    virtual float GetMaxSpeed() const override;
    virtual float GetMaxAcceleration() const override;
    virtual void UpdateFromCompressedFlags(uint8 Flags) override;
    virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
    virtual void PerformMovement(float DeltaSeconds) override;
    virtual void MoveAutonomous(float TimeStamp, float DeltaSeconds, uint8 Flags, const FVector& NewAccel) override;
    virtual void ServerMoveHandleClientError(float TimeStamp, float DeltaTime, const FVector& Accel, const FVector& RelativeLocation,
        FMovementBaseInterfaceData* BaseData, FName BoneName, uint8 Mode) override;
    virtual void Crouch(bool bClientSimulation = false) override;
    virtual bool DoJump(bool bReplayingMoves, float DeltaTime) override;
    virtual void ProcessLanded(const FHitResult& Hit, float RemainingTime, int32 Iterations) override;
    virtual void OnMovementModeChanged(EMovementMode PreviousMode, uint8 PreviousCustomMode) override;
    virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
    virtual void ClientHandleMoveResponse(const FCharacterMoveResponseDataContainer& Response) override;
    virtual bool ClientUpdatePositionAfterServerUpdate() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
private:
    void InitializeRules();
    void ProcessRequests();
    void Spend(float Amount);
    void PublishStamina();
    bool CanGroundSprint() const;
    bool CanRequestAction() const;
    void NotifySprint(bool Actual);
    FFPSMovementResource Resource, PendingCorrection;
    uint16 InputSprintSequence = 0, InputCrouchSequence = 0, InputJumpSequence = 0;
    bool InputSprint = false, InputCrouch = false, InputJumpRequested = false;
    bool bRulesInitialized = false, bActualSprint = false, bNotifiedSprint = false;
    bool bSpentThisMove = false, bHasCorrection = false, bInsideSimulation = false;
    float LastPublishedStamina = 100.f;
    UPROPERTY(Replicated) bool bRemoteSprint = false;
    UPROPERTY(Replicated) bool bRemoteWeakJump = false;
    TUniquePtr<FCharacterNetworkMoveDataContainer> MoveData;
    TUniquePtr<FCharacterMoveResponseDataContainer> MoveResponse;
    float ServerBoundaryTimeStamp = -1;
    FFPSMovementResource ServerBoundaryResource;
    FVector ServerBoundaryLocation = FVector::ZeroVector, ServerBoundaryVelocity = FVector::ZeroVector;
    TWeakObjectPtr<UObject> ServerBoundaryBase;
    FName ServerBoundaryBone;
    uint8 ServerBoundaryMode = 0;
};
