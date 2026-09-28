#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Curves/CurveVector.h"
#include "FPSWeaponData.generated.h"

UENUM(BlueprintType)
enum class EFPSWeaponSlot : uint8 { Primary, Secondary };
UENUM(BlueprintType)
enum class EFPSWeaponCategory : uint8 { Rifle, SMG, Pistol };
UENUM(BlueprintType)
enum class EFPSWeaponModel : uint8 { None, AK, M4, MP5, UMP45, M9, HeavyPistol };
UENUM(BlueprintType)
enum class EFPSWeaponAction : uint8 { Idle, Reloading, Switching, Inspecting };

/** Immutable definition. No current ammunition or other per-instance state lives here. */
UCLASS(BlueprintType)
class MCP_TEST_API UFPSWeaponDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) EFPSWeaponModel Model = EFPSWeaponModel::None;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) EFPSWeaponSlot Slot = EFPSWeaponSlot::Primary;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) EFPSWeaponCategory Category = EFPSWeaponCategory::Rifle;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) int32 MagazineSize = 30;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) int32 InitialReserve = 90;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float Damage = 25;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float FireInterval = .1f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float ReloadDuration = 2;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float Range = 6000;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float AimFOV = 65;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) bool bAutomatic = true;
    // X=pitch, Y=yaw (degrees per shot), Z=visual kick. Curve time is shot number.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UCurveVector> RecoilCurve;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TSoftObjectPtr<UStaticMesh> Mesh;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FVector PresentationScale = FVector(1);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FVector MuzzleLocation = FVector(56, 0, 0);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float SightHeight = 7;
};

USTRUCT(BlueprintType)
struct FFPSWeaponInstance
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid InstanceId;
    UPROPERTY(BlueprintReadOnly) EFPSWeaponModel Model = EFPSWeaponModel::None;
    UPROPERTY(BlueprintReadOnly) int32 Magazine = 0;
    UPROPERTY(BlueprintReadOnly) int32 Reserve = 0;
    bool IsValid() const { return Model != EFPSWeaponModel::None && InstanceId.IsValid(); }
};

USTRUCT(BlueprintType)
struct FFPSWeaponLoadout
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FFPSWeaponInstance Primary;
    UPROPERTY(BlueprintReadOnly) FFPSWeaponInstance Secondary;
    UPROPERTY(BlueprintReadOnly) EFPSWeaponSlot ActiveSlot = EFPSWeaponSlot::Primary;
    UPROPERTY(BlueprintReadOnly) EFPSWeaponAction Action = EFPSWeaponAction::Idle;
    UPROPERTY(BlueprintReadOnly) double ActionEndsAt = 0;
    UPROPERTY(BlueprintReadOnly) bool bAiming = false;
};

/** Code-backed definitions for this stage, using the same schema as editable data assets. */
UCLASS()
class MCP_TEST_API UFPSWeaponCatalog : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    const UFPSWeaponDefinition* Find(EFPSWeaponModel Model) const;
    FFPSWeaponInstance CreateInstance(EFPSWeaponModel Model) const;
private:
    UPROPERTY() TMap<EFPSWeaponModel, TObjectPtr<UFPSWeaponDefinition>> Definitions;
};
