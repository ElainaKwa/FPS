#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSWeaponData.h"
#include "FPSWeaponComponent.generated.h"

class AFPSDemoCharacter;
class AFPSWeaponPickup;
UENUM()
enum class EFPSWeaponRequest : uint8 { Reload, Inspect, Primary, Secondary, Drop, Pickup, AimOn, AimOff };

UCLASS(ClassGroup=(FPS), meta=(BlueprintSpawnableComponent))
class MCP_TEST_API UFPSWeaponComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UFPSWeaponComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    bool IsEnabled() const;
    bool CanAct() const;
    bool CanSprint() const;
    const FFPSWeaponInstance& Equipped() const;
    const FFPSWeaponLoadout& GetLoadout() const { return Loadout; }
    const UFPSWeaponDefinition* Definition() const;
    const UFPSWeaponCatalog* Catalog() const;
    AFPSWeaponPickup* FindPickup() const;
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
    bool FireAuthority();
    void Request(EFPSWeaponRequest Command, AFPSWeaponPickup* Target = nullptr);
private:
    friend struct FFPSWeaponSmoke;
    UPROPERTY(ReplicatedUsing=OnRep_Loadout) FFPSWeaponLoadout Loadout;
    UFUNCTION() void OnRep_Loadout();
    UFUNCTION(Server, Reliable) void ServerShoot();
    UFUNCTION(Server, Reliable) void ServerRequest(EFPSWeaponRequest Command, AFPSWeaponPickup* Target);
    void ShootLocal();
    void Changed();
    void BeginAction(EFPSWeaponAction Action, float Duration);
    AFPSWeaponPickup* SpawnDrop(const FFPSWeaponInstance& Instance);
    FFPSWeaponInstance& EquippedMutable();
    AFPSDemoCharacter* Character() const;
    double ServerTime() const;
    bool bTriggerHeld = false;
    double NextLocalShot = 0;
    double LastServerShot = -100;
    int32 LocalShotIndex = 0;
    FGuid ActionWeaponId;
    EFPSWeaponAction LastAudioAction = EFPSWeaponAction::Idle;
    FGuid LastAudioInstance;
    int32 LastAudioMagazine = 0;
    double LastEmptyClick = -10;
};
