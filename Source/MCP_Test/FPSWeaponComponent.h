#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSWeaponData.h"
#include "FPSWeaponComponent.generated.h"

class AFPSDemoCharacter;
class AFPSWeaponPickup;
class UFPSWeaponAbility;
class UFPSFireAbility;
class UFPSTimedWeaponAbility;
class UFPSAimAbility;
class UFPSTransferWeaponAbility;
UENUM()
enum class EFPSWeaponRequest : uint8 { Fire, Reload, Inspect, Primary, Secondary, Drop, Pickup, AimOn, AimOff, Cancel, Equip };

UENUM()
enum class EFPSWeaponResultPhase : uint8 { Started, Committed, Cancelled };

/** One-shot presentation result. Persistent poses remain in the replicated loadout. */
USTRUCT()
struct FFPSWeaponActionResult
{
    GENERATED_BODY()
    UPROPERTY() uint64 EventSerial = 0;
    UPROPERTY() uint64 ActionId = 0;
    UPROPERTY() FGuid Instance;
    UPROPERTY() FName WeaponId;
    UPROPERTY() EFPSWeaponRequest Command = EFPSWeaponRequest::Cancel;
    UPROPERTY() int32 PredictionKey = 0;
    UPROPERTY() EFPSWeaponResultPhase Phase = EFPSWeaponResultPhase::Cancelled;
};

enum class EFPSFireReadiness : uint8 { Invalid, WaitingForState, WaitingForDeadline, Ready };
DECLARE_MULTICAST_DELEGATE(FFPSFireReadinessChanged);

/** Local capability for one activation. Never replicated or accepted from an RPC. */
struct FFPSWeaponActionTicket
{
    uint64 Sequence = 0;
    FGuid Instance;
    EFPSWeaponRequest Command = EFPSWeaponRequest::Cancel;
    int32 PredictionKey = 0;
    bool operator==(const FFPSWeaponActionTicket& Other) const { return Sequence != 0 && Sequence == Other.Sequence && Instance == Other.Instance && Command == Other.Command && PredictionKey == Other.PredictionKey; }
};

/** Inventory, authoritative commits and presentation snapshots. Abilities own action lifetimes. */
UCLASS(ClassGroup=(FPS), meta=(BlueprintSpawnableComponent))
class MCP_TEST_API UFPSWeaponComponent : public UActorComponent
{
    friend struct FFPSCharacterSmoke;
    friend struct FFPSLoadoutSmoke;
    GENERATED_BODY()
public:
    UFPSWeaponComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    void GrantAbilities();
    void InitializeSpawnLoadout();
    bool SelectDefinition(FName WeaponId, FString& Failure);
    void CancelAttack();
    bool IsEnabled() const;
    bool CanAct() const;
    bool CanSprint() const;
    bool CanStart(EFPSWeaponRequest Command) const;
    const FFPSWeaponInstance& Equipped() const;
    const FFPSWeaponLoadout& GetLoadout() const { return Loadout; }
    const UFPSWeaponDefinition* Definition() const;
    const UFPSWeaponCatalog* Catalog() const;
    AFPSWeaponPickup* FindPickup() const;
    bool CanPickup(const AFPSWeaponPickup* Target) const;
    void StartFire();
    void StopFire();
    void Reload();
    void Inspect();
    void Primary();
    void Secondary();
    void Drop();
    void Pickup();
    void AimOn();
    void AimOff();
    void CancelForMenu();
    void StopAllActions();
    void Request(EFPSWeaponRequest Command, AFPSWeaponPickup* Target = nullptr);
    void SetReloadExitsAim(bool bValue);
    bool ReloadExitsAim() const { return bReloadExitsAim; }
    double ServerTime() const;
    FFPSWeaponActionTicket OpenActivation(UFPSWeaponAbility* Ability, EFPSWeaponRequest Command, int32 PredictionKey);
    void RejectPrediction(const FFPSWeaponActionTicket& Ticket);
    bool BeginFire(const FFPSWeaponActionTicket& Ticket, UFPSWeaponAbility* Ability);
    bool RebindAutomaticFire(FFPSWeaponActionTicket& Ticket);
    bool CommitShot(const FFPSWeaponActionTicket& Ticket);
    bool FinishBurst(const FFPSWeaponActionTicket& Ticket);
    bool EndFire(const FFPSWeaponActionTicket& Ticket, bool bCancelled);
    bool BeginAction(FFPSWeaponActionTicket& Ticket, UFPSWeaponAbility* Ability, float& Duration);
    bool FinishAction(const FFPSWeaponActionTicket& Ticket, bool bCancelled);
    bool BeginAim(const FFPSWeaponActionTicket& Ticket, UFPSWeaponAbility* Ability);
    bool EndAim(const FFPSWeaponActionTicket& Ticket);
    bool Transfer(const FFPSWeaponActionTicket& Ticket, AFPSWeaponPickup* Target);
    void InterruptInspect();
    EFPSFireReadiness FireReadiness(const FFPSWeaponActionTicket& Ticket, double& Deadline) const;
    FFPSFireReadinessChanged OnFireReadinessChanged;
private:
    friend struct FFPSWeaponSmoke;
    friend struct FFPSDropSmoke;
    friend struct FFPSWeaponAbilitySmoke;
    friend struct FFPSWeaponDamageSmoke;
    friend struct FFPSWeaponPresentationSmoke;
    UPROPERTY() FFPSWeaponLoadout Loadout;
    UPROPERTY(ReplicatedUsing=OnRep_OwnerState) FFPSWeaponLoadout OwnerState;
    UPROPERTY(ReplicatedUsing=OnRep_VisibleState) FFPSWeaponVisibleState VisibleState;
    UPROPERTY(Replicated) bool bReloadExitsAim = false;
    UFUNCTION() void OnRep_OwnerState();
    UFUNCTION() void OnRep_VisibleState();
    UFUNCTION(Server, Reliable) void ServerSetReloadExitsAim(bool bValue);
    void RefreshPresentation();
    void EmitActionResult(const FFPSWeaponActionTicket& Ticket, EFPSWeaponResultPhase Phase);
    void PresentActionResult(const FFPSWeaponActionResult& Result);
    UFUNCTION(NetMulticast, Reliable) void MulticastActionResult(const FFPSWeaponActionResult& Result);
    uint64 NextResultSerial = 0, LastResultSerial = 0;
    TArray<FFPSWeaponActionResult> PredictedStarts;
    void Changed();
    void CancelOperation(bool bIncludeAim);
    void CancelBurst();
    bool ValidOpening(const FFPSWeaponActionTicket& Ticket, const UFPSWeaponAbility* Ability) const;
    AFPSWeaponPickup* SpawnDrop(const FFPSWeaponInstance& Instance);
    FFPSWeaponInstance& EquippedMutable();
    AFPSDemoCharacter* Character() const;
    UPROPERTY() TObjectPtr<UFPSWeaponAbility> ActionAbility;
    UPROPERTY() TObjectPtr<UFPSWeaponAbility> AimAbility;
    UPROPERTY() TObjectPtr<UFPSWeaponAbility> FireAbility;
    FFPSWeaponActionTicket OpeningTicket, ActionTicket, AimTicket, FireTicket;
    TWeakObjectPtr<UFPSWeaponAbility> OpeningAbility;
    uint64 ActionSequence = 0;
    uint64 LastPredictionSequence = 0;
    EFPSFireMode ActiveFireMode = EFPSFireMode::Single;
    double ActionDeadline = 0;
    bool bTriggerHeld = false;
    bool bAimHeld = false;
    bool bAbilitiesGranted = false;
    int32 LastPredictedFireKey = 0;
    TMap<FGuid, FFPSWeaponInstance> PredictedShots;
#if !UE_BUILD_SHIPPING
    uint32 LocalShotPresentations = 0;
#endif
#if !UE_BUILD_SHIPPING
    uint32 ReloadStartPresentations = 0, ReloadCommitPresentations = 0, ConfirmedStartDuplicates = 0;
#endif
    double LastEmptyClick = -10;
};
