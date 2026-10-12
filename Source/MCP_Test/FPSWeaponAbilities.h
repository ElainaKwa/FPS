#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponAbilities.generated.h"

UCLASS(Abstract)
class MCP_TEST_API UFPSWeaponAbility : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UFPSWeaponAbility();
    EFPSWeaponRequest Command = EFPSWeaponRequest::Fire;
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
    void CancelWeaponAbility(bool bReplicate = true);
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
protected:
    UFPSWeaponComponent* Weapon() const;
    void Complete(bool bCancelled = false);
    FFPSWeaponActionTicket Ticket;
    bool IsCurrentActivation(const FGameplayAbilityActivationInfo& Info) const;
};

/** One trigger cycle. Timer cadence and burst cancellation belong to GAS, not the inventory tick. */
UCLASS()
class MCP_TEST_API UFPSFireAbility : public UFPSWeaponAbility
{
    GENERATED_BODY()
public:
    UFPSFireAbility();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
    bool IsAutomatic() const { return bAutomaticIntent; }
#if !UE_BUILD_SHIPPING
    uint32 GetStepCount() const { return StepCount; }
#endif
private:
    void Step();
    void Wake();
    void WaitUntil(double Deadline);
    FTimerHandle Timer;
    FDelegateHandle ReadinessHandle;
    bool bAutomaticIntent = false;
    bool bStepping = false;
    struct FShotCycle { FGuid Instance; EFPSFireMode Mode = EFPSFireMode::Single; int32 Remaining = 0; bool bHasShot = false; } Cycle;
#if !UE_BUILD_SHIPPING
    uint32 StepCount = 0;
#endif
};

/** Shared timing/atomic completion for magazine reload, inspect and equip preparation. */
UCLASS(Abstract)
class MCP_TEST_API UFPSTimedWeaponAbility : public UFPSWeaponAbility
{
    GENERATED_BODY()
public:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
private:
    FTimerHandle Timer;
    void Elapsed();
};
UCLASS()
class MCP_TEST_API UFPSReloadAbility : public UFPSTimedWeaponAbility
{
    GENERATED_BODY()
public: UFPSReloadAbility();
};
UCLASS()
class MCP_TEST_API UFPSModifyAttachmentAbility : public UFPSTimedWeaponAbility
{
    GENERATED_BODY()
public: UFPSModifyAttachmentAbility();
};
UCLASS()
class MCP_TEST_API UFPSModifyMagazineAbility : public UFPSTimedWeaponAbility
{
    GENERATED_BODY()
public: UFPSModifyMagazineAbility();
};
UCLASS()
class MCP_TEST_API UFPSCancelReloadAbility : public UFPSWeaponAbility
{
    GENERATED_BODY()
public:
    UFPSCancelReloadAbility();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
UCLASS()
class MCP_TEST_API UFPSInspectAbility : public UFPSTimedWeaponAbility
{
    GENERATED_BODY()
public: UFPSInspectAbility();
};
UCLASS()
class MCP_TEST_API UFPSPrimaryAbility : public UFPSTimedWeaponAbility
{
    GENERATED_BODY()
public: UFPSPrimaryAbility();
};
UCLASS()
class MCP_TEST_API UFPSSecondaryAbility : public UFPSTimedWeaponAbility
{
    GENERATED_BODY()
public: UFPSSecondaryAbility();
};
/** Used only after an authoritative transfer; clients see its replicated readiness. */
UCLASS()
class MCP_TEST_API UFPSEquipAbility : public UFPSTimedWeaponAbility
{
    GENERATED_BODY()
public: UFPSEquipAbility();
};
UCLASS()
class MCP_TEST_API UFPSAimAbility : public UFPSWeaponAbility
{
    GENERATED_BODY()
public:
    UFPSAimAbility();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
};
UCLASS(Abstract)
class MCP_TEST_API UFPSTransferWeaponAbility : public UFPSWeaponAbility
{
    GENERATED_BODY()
public:
    UFPSTransferWeaponAbility();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
UCLASS()
class MCP_TEST_API UFPSDropAbility : public UFPSTransferWeaponAbility
{
    GENERATED_BODY()
public: UFPSDropAbility() { Command = EFPSWeaponRequest::Drop; }
};
UCLASS()
class MCP_TEST_API UFPSPickupAbility : public UFPSTransferWeaponAbility
{
    GENERATED_BODY()
public: UFPSPickupAbility();
};

UCLASS()
class MCP_TEST_API UFPSCancelWeaponAbility : public UFPSWeaponAbility
{
    GENERATED_BODY()
public:
    UFPSCancelWeaponAbility();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
