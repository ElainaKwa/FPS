#include "FPSMovementComponent.h"
#include "FPSMovementSettings.h"
#include "FPSDemoCharacter.h"
#include "FPSActionPolicy.h"
#include "FPSDemoGameState.h"
#include "FPSDemoAttributeSet.h"
#include "FPSWeaponComponent.h"
#include "FPSBaseMembership.h"
#include "GameFramework/CharacterMovementReplication.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Net/UnrealNetwork.h"

bool FFPSMovementResource::Serialize(FArchive& Ar)
{
    Ar << Stamina << RecoveryRemaining << JumpRemaining << CrouchRemaining << SpeedLimit << AirSpeedLimit;
    Ar << SprintSequence << CrouchSequence << JumpSequence << JumpCharges << CrouchCharges;
    Ar << SprintIntent << CrouchIntent << WeakJump << Crouched << ActualSprint;
    return !Ar.IsError();
}
bool FFPSMovementResource::Matches(const FFPSMovementResource& B) const
{
    return FMath::Abs(Stamina-B.Stamina)<.08f && FMath::Abs(RecoveryRemaining-B.RecoveryRemaining)<.008f &&
        FMath::Abs(JumpRemaining-B.JumpRemaining)<.008f && FMath::Abs(CrouchRemaining-B.CrouchRemaining)<.008f &&
        FMath::Abs(SpeedLimit-B.SpeedLimit)<.5f && FMath::Abs(AirSpeedLimit-B.AirSpeedLimit)<.5f &&
        SprintSequence==B.SprintSequence && CrouchSequence==B.CrouchSequence && JumpSequence==B.JumpSequence &&
        JumpCharges==B.JumpCharges && CrouchCharges==B.CrouchCharges &&
        SprintIntent==B.SprintIntent && CrouchIntent==B.CrouchIntent && WeakJump==B.WeakJump && Crouched==B.Crouched && ActualSprint==B.ActualSprint;
}

class FFPSMove : public FSavedMove_Character
{
public:
    using Super=FSavedMove_Character;
    uint16 SprintSequence=0, CrouchSequence=0, JumpSequence=0;
    bool Sprint=false, CrouchInput=false, JumpRequested=false;
    FFPSMovementResource EndResource;
    virtual void Clear() override { Super::Clear(); SprintSequence=CrouchSequence=JumpSequence=0; Sprint=CrouchInput=JumpRequested=false; EndResource={}; }
    virtual bool CanCombineWith(const FSavedMovePtr&, ACharacter*, float) const override { return false; }
    virtual bool IsImportantMove(const FSavedMovePtr& Ack) const override
    {
        const auto* Other=static_cast<const FFPSMove*>(Ack.Get());
        return !Other || SprintSequence!=Other->SprintSequence || CrouchSequence!=Other->CrouchSequence ||
            JumpSequence!=Other->JumpSequence || Super::IsImportantMove(Ack);
    }
    virtual uint8 GetCompressedFlags() const override
    { return Super::GetCompressedFlags() | (Sprint ? FLAG_Custom_0 : 0) | (CrouchInput ? FLAG_Custom_1 : 0) | (JumpRequested ? FLAG_Custom_2 : 0); }
    virtual void SetMoveFor(ACharacter* C, float D, const FVector& A, FNetworkPredictionData_Client_Character& P) override
    {
        Super::SetMoveFor(C,D,A,P);
        const auto* M=CastChecked<UFPSMovementComponent>(C->GetCharacterMovement());
        SprintSequence=M->InputSprintSequence; CrouchSequence=M->InputCrouchSequence; JumpSequence=M->InputJumpSequence;
        Sprint=M->InputSprint; CrouchInput=M->InputCrouch; JumpRequested=M->InputJumpRequested;
        bForceNoCombine=true;
    }
    virtual void PrepMoveFor(ACharacter* C) override
    {
        Super::PrepMoveFor(C);
        auto* M=CastChecked<UFPSMovementComponent>(C->GetCharacterMovement());
        M->InputSprintSequence=SprintSequence; M->InputCrouchSequence=CrouchSequence; M->InputJumpSequence=JumpSequence;
        M->InputSprint=Sprint; M->InputCrouch=CrouchInput; M->InputJumpRequested=JumpRequested;
    }
    virtual void PostUpdate(ACharacter* C, EPostUpdateMode Mode) override
    { Super::PostUpdate(C,Mode); EndResource=CastChecked<UFPSMovementComponent>(C->GetCharacterMovement())->Resource; }
};
struct FFPSMoveData : FCharacterNetworkMoveData
{
    uint16 SprintSequence=0, CrouchSequence=0, JumpSequence=0;
    virtual void ClientFillNetworkMoveData(const FSavedMove_Character& Base, ENetworkMoveType Type) override
    {
        Super::ClientFillNetworkMoveData(Base,Type);
        const auto& M=static_cast<const FFPSMove&>(Base);
        SprintSequence=M.SprintSequence; CrouchSequence=M.CrouchSequence; JumpSequence=M.JumpSequence;
    }
    virtual bool Serialize(UCharacterMovementComponent& C, FArchive& Ar, UPackageMap* P, ENetworkMoveType T) override
    { const bool OK=Super::Serialize(C,Ar,P,T); Ar << SprintSequence << CrouchSequence << JumpSequence; return OK && !Ar.IsError(); }
    using Super=FCharacterNetworkMoveData;
};
struct FFPSMoveDataContainer : FCharacterNetworkMoveDataContainer
{
    FFPSMoveData Data[3];
    FFPSMoveDataContainer() { NewMoveData=&Data[0]; PendingMoveData=&Data[1]; OldMoveData=&Data[2]; }
};
struct FFPSMoveResponse : FCharacterMoveResponseDataContainer
{
    FFPSMovementResource Resource;
    FVector_NetQuantize100 Position, Velocity;
    TObjectPtr<UObject> BaseOwner;
    FName BoneName;
    uint8 Mode=0;
    bool Enabled=false;
    virtual void ServerFillResponseData(const UCharacterMovementComponent& Base, const FClientAdjustment& A) override
    {
        FCharacterMoveResponseDataContainer::ServerFillResponseData(Base,A);
        const auto& M=static_cast<const UFPSMovementComponent&>(Base);
        Enabled=M.UsesMovementRules(); Resource=M.Resource;
        Position=M.GetOwner()->GetActorLocation(); Velocity=M.Velocity; Mode=M.PackNetworkMovementMode();
        BaseOwner=M.GetMovementBaseInterfaceData()->PhysicsObjectOwner.Get();
        BoneName=M.GetCharacterOwner()->GetBasedMovement().BoneName;
        if (M.ServerBoundaryTimeStamp==A.TimeStamp)
        {
            // Sending may occur after a forced server update. Capture the acknowledged
            // simulation boundary, not whatever resource/position happens to be current.
            Resource=M.ServerBoundaryResource; Position=M.ServerBoundaryLocation; Velocity=M.ServerBoundaryVelocity;
            BaseOwner=M.ServerBoundaryBase.Get(); BoneName=M.ServerBoundaryBone; Mode=M.ServerBoundaryMode;
        }
    }
    virtual bool Serialize(UCharacterMovementComponent& M, FArchive& Ar, UPackageMap* P) override
    {
        const bool OK=FCharacterMoveResponseDataContainer::Serialize(M,Ar,P);
        Ar << Enabled;
        if (Enabled)
        {
            Resource.Serialize(Ar); bool Out=true;
            Position.NetSerialize(Ar,P,Out); Velocity.NetSerialize(Ar,P,Out); Ar << Mode << BaseOwner << BoneName;
        }
        return OK && !Ar.IsError();
    }
};
class FFPSPrediction : public FNetworkPredictionData_Client_Character
{
public:
    explicit FFPSPrediction(const UCharacterMovementComponent& M) : FNetworkPredictionData_Client_Character(M) {}
    virtual FSavedMovePtr AllocateNewMove() override { return FSavedMovePtr(new FFPSMove()); }
};

UFPSMovementComponent::UFPSMovementComponent()
{
    SetIsReplicatedByDefault(true);
    MoveData=MakeUnique<FFPSMoveDataContainer>(); MoveResponse=MakeUnique<FFPSMoveResponse>();
    SetNetworkMoveDataContainer(*MoveData); SetMoveResponseDataContainer(*MoveResponse);
}
UFPSMovementComponent::~UFPSMovementComponent() = default;
bool UFPSMovementComponent::UsesMovementRules() const
{ const auto* GS=GetWorld() ? GetWorld()->GetGameState<AFPSDemoGameState>() : nullptr; return GS && GS->bStructuredMatch; }
bool UFPSMovementComponent::UsesStamina() const
{ return UsesMovementRules() && CharacterOwner && CharacterOwner->IsPlayerControlled(); }
bool UFPSMovementComponent::CanRequestAction() const
{
    const auto* P=Cast<AFPSDemoCharacter>(CharacterOwner);
    const auto* PC=P ? Cast<APlayerController>(P->GetController()) : nullptr;
    return P && P->GetActionPolicy()->CanMove() && !P->IsEliminated() && !(PC && PC->IsMoveInputIgnored());
}
void UFPSMovementComponent::InitializeRules()
{
    if (bRulesInitialized || !UsesMovementRules()) return;
    bRulesInitialized=true;
    const auto* S=GetDefault<UFPSMovementSettings>();
    Resource.Stamina=S->MaxStamina; Resource.SpeedLimit=Resource.AirSpeedLimit=S->WalkSpeed;
    MaxWalkSpeed=S->WalkSpeed; MaxWalkSpeedCrouched=S->CrouchSpeed; JumpZVelocity=S->JumpVelocity;
    CharacterOwner->JumpMaxHoldTime=0; AirControl=S->AirControl; FallingLateralFriction=0; BrakingDecelerationFalling=0;
    bUseSeparateBrakingFriction=true; BrakingFriction=0;
    if (auto* P=Cast<AFPSDemoCharacter>(CharacterOwner); P && P->HasAuthority())
    {
        P->GetAbilitySystemComponent()->SetNumericAttributeBase(UFPSDemoAttributeSet::GetMoveSpeedAttribute(),S->WalkSpeed);
        P->GetAbilitySystemComponent()->SetNumericAttributeBase(UFPSDemoAttributeSet::GetMaxStaminaAttribute(),S->MaxStamina);
        PublishStamina();
    }
}
void UFPSMovementComponent::RequestSprint(bool Pressed)
{
    if (!UsesMovementRules()) return;
    InputSprint=Pressed; ++InputSprintSequence;
    if (!Pressed) ClearSprintIntent();
}
void UFPSMovementComponent::RequestCrouch(bool Pressed)
{
    if (!UsesMovementRules()) return;
    InputCrouch=Pressed; ++InputCrouchSequence;
}
void UFPSMovementComponent::RequestJump()
{ if (UsesMovementRules()) { ++InputJumpSequence; InputJumpRequested=true; } }
void UFPSMovementComponent::ClearSprintIntent()
{
    InputSprint=false; Resource.SprintIntent=false; bWantsSprint=false; bActualSprint=false;
    if (!CharacterOwner || !CharacterOwner->bClientUpdating) NotifySprint(false);
}
void UFPSMovementComponent::ClearMovementIntents()
{
    ClearSprintIntent(); InputCrouch=false; Resource.CrouchIntent=false; bWantsToCrouch=false;
    InputJumpRequested=false; Resource.JumpSequence=InputJumpSequence;
}
void UFPSMovementComponent::NotifySprint(bool Actual)
{
    if (Actual==bNotifiedSprint) return;
    bNotifiedSprint=Actual;
    if (auto* P=Cast<AFPSDemoCharacter>(CharacterOwner)) P->GetWeapons()->NotifySprintStateChanged(Actual);
}
float UFPSMovementComponent::GetMaxStamina() const
{
    if (const auto* P=Cast<AFPSDemoCharacter>(CharacterOwner))
        return P->GetAbilitySystemComponent()->GetNumericAttribute(UFPSDemoAttributeSet::GetMaxStaminaAttribute());
    return GetDefault<UFPSMovementSettings>()->MaxStamina;
}
float UFPSMovementComponent::GetStamina() const
{
    if (!CharacterOwner || CharacterOwner->GetLocalRole()!=ROLE_SimulatedProxy) return Resource.Stamina;
    const auto* P=Cast<AFPSDemoCharacter>(CharacterOwner);
    return P ? P->GetAbilitySystemComponent()->GetNumericAttribute(UFPSDemoAttributeSet::GetStaminaAttribute()) : GetMaxStamina();
}
bool UFPSMovementComponent::IsRecovering() const
{ return UsesStamina() && Resource.Stamina<GetMaxStamina()-.01f && Resource.RecoveryRemaining<=0 && !bActualSprint; }
bool UFPSMovementComponent::IsSprintStartBlocked() const
{ return UsesStamina() && !HasSprintIntent() && GetStamina()<GetDefault<UFPSMovementSettings>()->SprintStartThreshold; }
float UFPSMovementComponent::GetSprintFirePreparationSeconds() const
{ return GetDefault<UFPSMovementSettings>()->SprintFirePreparation; }
bool UFPSMovementComponent::IsSprinting() const
{ return CharacterOwner && CharacterOwner->GetLocalRole()==ROLE_SimulatedProxy ? bRemoteSprint : bActualSprint; }
bool UFPSMovementComponent::IsWeakJump() const
{ return CharacterOwner && CharacterOwner->GetLocalRole()==ROLE_SimulatedProxy ? bRemoteWeakJump : Resource.WeakJump; }
bool UFPSMovementComponent::CanGroundSprint() const
{
    const auto* P=Cast<AFPSDemoCharacter>(CharacterOwner);
    return Resource.SprintIntent && P && !P->bIsCrouched && IsMovingOnGround() && CanRequestAction() &&
        P->GetWeapons()->CanSprint() && (!UsesStamina() || Resource.Stamina>0) &&
        FVector::DotProduct(Acceleration.GetSafeNormal2D(),P->GetActorForwardVector())>.05f;
}
void UFPSMovementComponent::Spend(float Amount)
{
    if (!UsesStamina()) return;
    Resource.Stamina=FMath::Max(0.f,Resource.Stamina-FMath::Max(0.f,Amount));
    Resource.RecoveryRemaining=GetDefault<UFPSMovementSettings>()->RecoveryDelay;
    bSpentThisMove=true;
    if (Resource.Stamina<=0) ClearSprintIntent();
}
void UFPSMovementComponent::ProcessRequests()
{
    auto* P=Cast<AFPSDemoCharacter>(CharacterOwner);
    const auto* S=GetDefault<UFPSMovementSettings>();
    const auto Newer=[](uint16 A,uint16 B) { return static_cast<int16>(A-B)>0; };
    if (Newer(InputSprintSequence,Resource.SprintSequence))
    {
        Resource.SprintSequence=InputSprintSequence;
        Resource.SprintIntent=InputSprint && CanRequestAction() && !P->bIsCrouched && P->GetWeapons()->CanSprint() &&
            (!UsesStamina() || Resource.Stamina>=S->SprintStartThreshold);
    }
    if (!InputSprint || !CanRequestAction() || !P->GetWeapons()->CanSprint() || P->bIsCrouched)
        ClearSprintIntent();
    const float Forward=FVector::DotProduct(Acceleration,P->GetActorForwardVector());
    if (!Acceleration.IsNearlyZero() && Forward<=.05f) ClearSprintIntent();
    if (Newer(InputCrouchSequence,Resource.CrouchSequence))
    {
        Resource.CrouchSequence=InputCrouchSequence;
        if (!InputCrouch) Resource.CrouchIntent=false;
        else if (CanRequestAction() && (P->bIsCrouched || Resource.CrouchRemaining<=UE_KINDA_SMALL_NUMBER))
        {
            ClearSprintIntent(); bWantsToCrouch=true;
            Crouch(false);
            Resource.CrouchIntent=P->bIsCrouched;
        }
        // Failure is consumed and never queued for a later frame.
    }
    if (!InputCrouch) Resource.CrouchIntent=false;
    bWantsToCrouch=Resource.CrouchIntent;
    if (!bWantsToCrouch && P->bIsCrouched) UnCrouch(false);
    if (Newer(InputJumpSequence,Resource.JumpSequence))
    {
        Resource.JumpSequence=InputJumpSequence;
        if (InputJumpRequested && CanRequestAction() && IsMovingOnGround() && Resource.JumpRemaining<=UE_KINDA_SMALL_NUMBER)
        {
            if (P->bIsCrouched) { bWantsToCrouch=false; UnCrouch(false); }
            if (!P->bIsCrouched && DoJump(P->bClientUpdating,0.f))
            { Resource.CrouchIntent=false; bWantsToCrouch=false; }
            else bWantsToCrouch=Resource.CrouchIntent;
        }
    }
    bWantsSprint=Resource.SprintIntent;
}
bool UFPSMovementComponent::DoJump(bool Replaying, float DeltaTime)
{
    if (!UsesMovementRules()) return Super::DoJump(Replaying,DeltaTime);
    if (!CanRequestAction() || !IsMovingOnGround() || Resource.JumpRemaining>UE_KINDA_SMALL_NUMBER) return false;
    const auto* S=GetDefault<UFPSMovementSettings>();
    const bool Weak=UsesStamina() && Resource.Stamina<S->JumpCost;
    JumpZVelocity=S->JumpVelocity * (Weak ? FMath::Sqrt(S->WeakJumpHeightRatio) : 1.f);
    Resource.AirSpeedLimit=FMath::Max(S->WalkSpeed,Velocity.Size2D());
    if (!Super::DoJump(Replaying,DeltaTime)) return false;
    if (!CharacterOwner->bClientUpdating) CharacterOwner->OnJumped();
    Resource.WeakJump=Weak; ++Resource.JumpCharges; Spend(S->JumpCost);
    return true;
}
void UFPSMovementComponent::Crouch(bool ClientSimulation)
{
    if (!UsesMovementRules() || ClientSimulation) { Super::Crouch(ClientSimulation); return; }
    if (!CanRequestAction() || (!CharacterOwner->bIsCrouched && Resource.CrouchRemaining>UE_KINDA_SMALL_NUMBER)) return;
    const bool Was=CharacterOwner->bIsCrouched;
    Super::Crouch(false);
    if (!Was && CharacterOwner->bIsCrouched)
    {
        const auto* S=GetDefault<UFPSMovementSettings>();
        Resource.CrouchRemaining=(!UsesStamina() || Resource.Stamina>=S->CrouchCost) ? S->CrouchInterval : S->LowCrouchInterval;
        ++Resource.CrouchCharges; Spend(S->CrouchCost); ClearSprintIntent();
    }
}
void UFPSMovementComponent::ProcessLanded(const FHitResult& H,float R,int32 I)
{
    if (UsesMovementRules()) { Resource.JumpRemaining=GetDefault<UFPSMovementSettings>()->LandingJumpInterval; Resource.WeakJump=false; }
    Super::ProcessLanded(H,R,I);
}
void UFPSMovementComponent::OnMovementModeChanged(EMovementMode PreviousMode,uint8 PreviousCustomMode)
{
    Super::OnMovementModeChanged(PreviousMode,PreviousCustomMode);
    if (UsesMovementRules() && PreviousMode==MOVE_Falling && IsMovingOnGround())
    {
        // Native falling may continue ground physics in the same move. Restore
        // ground input strength for that remainder instead of carrying air input.
        Acceleration=ConstrainInputAcceleration(Acceleration).GetSafeNormal()*
            GetDefault<UFPSMovementSettings>()->WalkAcceleration*FMath::Clamp(AnalogInputModifier,0.f,1.f);
        AnalogInputModifier=ComputeAnalogInputModifier();
    }
    if (UsesMovementRules() && IsFalling() && PreviousMode!=MOVE_Falling)
    {
        Resource.AirSpeedLimit=FMath::Max(GetDefault<UFPSMovementSettings>()->WalkSpeed,Velocity.Size2D());
        if (PreviousMode==MOVE_Walking || PreviousMode==MOVE_NavWalking)
        {
            // Jump requests run after CMC scales this frame's ground input. Preserve
            // its analog fraction before native AirControl, including repeated
            // ground/air transitions within one movement's collision substeps.
            const auto* S=GetDefault<UFPSMovementSettings>();
            Acceleration=ConstrainInputAcceleration(Acceleration).GetSafeNormal()*
                S->AirAcceleration*FMath::Clamp(AnalogInputModifier,0.f,1.f);
            AnalogInputModifier=ComputeAnalogInputModifier();
#if !UE_BUILD_SHIPPING
            if (FParse::Param(FCommandLine::Get(),TEXT("FPSMovementSmoke")))
                UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_SMOKE MEASURE air_entry local=%d acceleration=%.3f native_air_acceleration=%.3f analog=%.3f previous_mode=%d"),
                    CharacterOwner->IsLocallyControlled(),Acceleration.Size(),GetFallingLateralAcceleration(0.f).Size(),AnalogInputModifier,int32(PreviousMode));
#endif
        }
    }
}
float UFPSMovementComponent::GetMaxAcceleration() const
{
    if (!UsesMovementRules()) return Super::GetMaxAcceleration();
    const auto* S=GetDefault<UFPSMovementSettings>();
    if (IsFalling()) return S->AirAcceleration;
    // SpeedLimit supplies the sprint ramp. Reducing acceleration here also slows
    // steering/reversals, and CMC can clamp a new input using the previous input.
    return S->WalkAcceleration;
}
float UFPSMovementComponent::GetMaxSpeed() const
{
    if (!UsesMovementRules()) return Super::GetMaxSpeed();
    const auto* P=Cast<AFPSDemoCharacter>(CharacterOwner);
    if (!P || !P->GetActionPolicy()->CanMove() || P->IsEliminated()) return 0;
    const auto* S=GetDefault<UFPSMovementSettings>();
    if (IsFalling()) return Resource.AirSpeedLimit;
    if (IsCrouching()) return S->CrouchSpeed;
    if (P->GetWeapons()->GetLoadout().bAiming) return S->AimSpeed;
    return Resource.SpeedLimit;
}
void UFPSMovementComponent::PerformMovement(float D)
{
    InitializeRules();
    if (!UsesMovementRules()) { Super::PerformMovement(D); return; }
    if (!HasValidData() || D<=0) return;
    const auto* S=GetDefault<UFPSMovementSettings>();
    if (CharacterOwner->HasAuthority())
    {
        const auto* P=CastChecked<AFPSDemoCharacter>(CharacterOwner);
        const float External=P->GetAbilitySystemComponent()->GetNumericAttribute(UFPSDemoAttributeSet::GetStaminaAttribute());
        if (!FMath::IsNearlyEqual(External,LastPublishedStamina,.001f)) Resource.Stamina=FMath::Clamp(External,0.f,GetMaxStamina());
    }
    bInsideSimulation=true; bSpentThisMove=false;
    Resource.JumpRemaining=FMath::Max(0.f,Resource.JumpRemaining-D);
    Resource.CrouchRemaining=FMath::Max(0.f,Resource.CrouchRemaining-D);
    ProcessRequests();
    const bool WantsRun=CanGroundSprint();
    const float Target=WantsRun ? S->SprintSpeed : S->WalkSpeed;
    const float Rate=(S->SprintSpeed-S->WalkSpeed)/FMath::Max(.01f,WantsRun ? S->SprintAccelerationTime : S->SprintDecelerationTime);
    // AirSpeedLimit preserves takeoff momentum. Keep the corresponding ground
    // limit while the run intent survives so landing does not briefly clamp to walk.
    if (!IsFalling() || !Resource.SprintIntent)
        Resource.SpeedLimit=FMath::FInterpConstantTo(Resource.SpeedLimit,Target,D,Rate);
    // Native CMC friction steers velocity toward input; keep it during sprinting.
    // The speed limit still eases a forward sprint back to walking in 0.15 s,
    // while stopping, reversing and stance changes use responsive ground braking.
    GroundFriction=8.f;
    BrakingDecelerationWalking=S->WalkAcceleration;
    const FVector Old=UpdatedComponent->GetComponentLocation();
    const bool StartedOnGround=IsMovingOnGround();
    Super::PerformMovement(D);
    if (!HasValidData()) { bInsideSimulation=false; return; }
    const FVector Displacement=UpdatedComponent->GetComponentLocation()-Old;
    bActualSprint=WantsRun && StartedOnGround && IsMovingOnGround() && FVector::DotProduct(Displacement,CharacterOwner->GetActorForwardVector())>.05f;
    if (bActualSprint && UsesStamina())
    {
        Spend(S->SprintDrain*D);
        if (!CharacterOwner->bClientUpdating) DebugSprintConsumedSeconds+=D;
        if (Resource.Stamina<=0) ClearSprintIntent();
    }
    if (!bSpentThisMove && UsesStamina())
    {
        const float RecoverTime=FMath::Max(0.f,D-Resource.RecoveryRemaining);
        Resource.RecoveryRemaining=FMath::Max(0.f,Resource.RecoveryRemaining-D);
        Resource.Stamina=FMath::Min(GetMaxStamina(),Resource.Stamina+RecoverTime*S->RecoveryRate);
    }
    if (!CanRequestAction()) ClearMovementIntents();
    Resource.Crouched=CharacterOwner->bIsCrouched; Resource.ActualSprint=bActualSprint;
    bRemoteSprint=bActualSprint; bRemoteWeakJump=Resource.WeakJump;
    if (!CharacterOwner->bClientUpdating) NotifySprint(bActualSprint);
    PublishStamina(); bInsideSimulation=false;
}
void UFPSMovementComponent::PublishStamina()
{
    if (auto* P=Cast<AFPSDemoCharacter>(CharacterOwner); P && P->HasAuthority())
    {
        P->GetAbilitySystemComponent()->SetNumericAttributeBase(UFPSDemoAttributeSet::GetStaminaAttribute(),Resource.Stamina);
        LastPublishedStamina=Resource.Stamina;
    }
}
void UFPSMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
    Super::UpdateFromCompressedFlags(Flags);
    InputSprint=(Flags & FSavedMove_Character::FLAG_Custom_0)!=0;
    InputCrouch=(Flags & FSavedMove_Character::FLAG_Custom_1)!=0;
    InputJumpRequested=(Flags & FSavedMove_Character::FLAG_Custom_2)!=0;
    if (UsesMovementRules()) bWantsToCrouch=Resource.CrouchIntent;
}
void UFPSMovementComponent::MoveAutonomous(float T,float D,uint8 F,const FVector& A)
{
    if (const auto* Data=GetCurrentNetworkMoveData())
    {
        const auto& M=static_cast<const FFPSMoveData&>(*Data);
        InputSprintSequence=M.SprintSequence; InputCrouchSequence=M.CrouchSequence; InputJumpSequence=M.JumpSequence;
    }
    Super::MoveAutonomous(T,D,F,A);
}
void UFPSMovementComponent::ServerMoveHandleClientError(float T,float D,const FVector& A,const FVector& R,
    FMovementBaseInterfaceData* B,FName Bone,uint8 Mode)
{
    Super::ServerMoveHandleClientError(T,D,A,R,B,Bone,Mode);
    if (GetPredictionData_Server_Character()->PendingAdjustment.TimeStamp==T)
    {
        ServerBoundaryTimeStamp=T; ServerBoundaryResource=Resource;
        ServerBoundaryLocation=GetOwner()->GetActorLocation(); ServerBoundaryVelocity=Velocity;
        ServerBoundaryBase=GetMovementBaseInterfaceData()->PhysicsObjectOwner;
        ServerBoundaryBone=CharacterOwner->GetBasedMovement().BoneName; ServerBoundaryMode=PackNetworkMovementMode();
    }
}
FNetworkPredictionData_Client* UFPSMovementComponent::GetPredictionData_Client() const
{
    if (!ClientPredictionData) const_cast<UFPSMovementComponent*>(this)->ClientPredictionData=new FFPSPrediction(*this);
    return ClientPredictionData;
}
void UFPSMovementComponent::ClientHandleMoveResponse(const FCharacterMoveResponseDataContainer& Base)
{
    const auto& R=static_cast<const FFPSMoveResponse&>(Base);
    auto* P=GetPredictionData_Client_Character();
    const int32 Index=P->GetSavedMoveIndex(Base.ClientAdjustment.TimeStamp);
    // Late/out-of-order/expired snapshots cannot overwrite newer predicted resources.
    if (R.Enabled && Index!=INDEX_NONE)
    {
        const auto& Saved=static_cast<const FFPSMove&>(*P->SavedMoves[Index]);
        if (bHasCorrection || Base.IsCorrection() || !Saved.EndResource.Matches(R.Resource))
        {
            if (Base.IsGoodMove())
            {
                // Good position acknowledgement with resource disagreement: use CMC's normal
                // position correction and replay from the same acknowledged move boundary.
                FMovementBaseInterfaceData BaseData(R.BaseOwner);
                ClientAdjustPosition_Implementation(Base.ClientAdjustment.TimeStamp,R.Position,R.Velocity,
                    &BaseData,R.BoneName,BaseData.IsValid(),false,R.Mode,TOptional<FRotator>());
            }
            else Super::ClientHandleMoveResponse(Base);
            // Native CMC may reject a correction (expired move, missing relative base,
            // inactive body). Never roll resources back unless that boundary was acked.
            if (P->bUpdatePosition && P->LastAckedMove.IsValid() && P->LastAckedMove->TimeStamp==Base.ClientAdjustment.TimeStamp)
            {
                PendingCorrection=R.Resource; bHasCorrection=true; ++DebugCorrectionCount;
                if (CharacterOwner->bIsCrouched!=R.Resource.Crouched)
                {
                    CharacterOwner->SetIsCrouched(R.Resource.Crouched);
                    if (R.Resource.Crouched) Super::Crouch(true); else Super::UnCrouch(true);
                }
            }
            return;
        }
    }
    Super::ClientHandleMoveResponse(Base);
}
bool UFPSMovementComponent::ClientUpdatePositionAfterServerUpdate()
{
    const uint16 Sprint=InputSprintSequence,CrouchSeq=InputCrouchSequence,Jump=InputJumpSequence;
    const bool SprintInput=InputSprint,CrouchInput=InputCrouch,JumpRequested=InputJumpRequested;
    if (bHasCorrection && GetPredictionData_Client_Character()->bUpdatePosition)
    { Resource=PendingCorrection; bWantsSprint=Resource.SprintIntent; bActualSprint=Resource.ActualSprint; }
    bHasCorrection=false;
    const bool Replayed=Super::ClientUpdatePositionAfterServerUpdate();
    if (Replayed) ++DebugReplayCount;
    InputSprintSequence=Sprint; InputCrouchSequence=CrouchSeq; InputJumpSequence=Jump;
    InputSprint=SprintInput; InputCrouch=CrouchInput; InputJumpRequested=JumpRequested;
    return Replayed;
}
void UFPSMovementComponent::OnMovementUpdated(float D,const FVector& Old,const FVector& V)
{
    Super::OnMovementUpdated(D,Old,V);
    if (const auto* P=Cast<AFPSDemoCharacter>(CharacterOwner))
        if (auto* B=P->FindComponentByClass<UFPSBaseMembership>()) B->EnforceMovement(Old);
}
void UFPSMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME_CONDITION(UFPSMovementComponent,bRemoteSprint,COND_SimulatedOnly); DOREPLIFETIME_CONDITION(UFPSMovementComponent,bRemoteWeakJump,COND_SimulatedOnly); }
void UFPSMovementComponent::SetStaminaForTest(float Value)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(),TEXT("FPSMovementSmoke"))) return;
    InitializeRules(); Resource.Stamina=FMath::Clamp(Value,0.f,GetMaxStamina());
    Resource.RecoveryRemaining=0; PublishStamina();
    if (CharacterOwner && CharacterOwner->HasAuthority()) ForceClientAdjustment();
#endif
}
