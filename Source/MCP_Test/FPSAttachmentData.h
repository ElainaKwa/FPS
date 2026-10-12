#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FPSWeaponData.h"
#include "FPSAttachmentData.generated.h"

UENUM(BlueprintType)
enum class EFPSAttachmentCategory : uint8 { Optic, Muzzle, Magazine };

/** A model's authored values, never a character buff or a second copy of weapon tuning. */
UCLASS(BlueprintType)
class MCP_TEST_API UFPSAttachmentDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AssetRegistrySearchable, Category="Identity") FName AttachmentId;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") FText DisplayName;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Compatibility") EFPSAttachmentCategory Category = EFPSAttachmentCategory::Optic;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Compatibility") FName MountId;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Compatibility") bool bUniversal = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Compatibility", meta=(EditCondition="!bUniversal")) TArray<FName> Platforms;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Magazine", meta=(ToolTip="Integer addition to base weapon capacity; only magazine attachments.")) int32 CapacityDelta = 0;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Magazine", meta=(ClampMin="0.001")) float ReloadTimeMultiplier = 1.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Muzzle", meta=(ClampMin="0")) float VerticalRecoilMultiplier = 1.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Muzzle", meta=(ClampMin="0")) float HorizontalRecoilMultiplier = 1.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Optic") bool bOverrideAim = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Optic", meta=(ClampMin="1", ClampMax="179", Units="deg")) float AimFOV = 65.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Optic", meta=(Units="cm")) FVector AimLocation = FVector::ZeroVector;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Optic", meta=(ClampMin="0.001")) float AimTimeMultiplier = 1.f;
    // Explicitly retain base visuals until an attachment has its own approved meshes.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") bool bUseWeaponPresentation = true;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") FFPSWeaponPresentation FirstPerson;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") FFPSWeaponPresentation ThirdPerson;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") FFPSWeaponPresentation Pickup;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio") TSoftObjectPtr<USoundBase> FireSound;
    UPROPERTY(VisibleAnywhere, Category="Content") TMap<FSoftObjectPath, FString> SavedResourceVersions;
    virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("WeaponAttachment"), AttachmentId); }
    void ValidateDefinition(TArray<FString>& Errors) const;
    TArray<FSoftObjectPath> Resources() const;
    FString ContentDigest(TArray<FString>& Errors) const;
    static void Discover(TArray<UFPSAttachmentDefinition*>& Out, TArray<FString>& Errors);
    static void ValidateCatalog(const TArray<UFPSAttachmentDefinition*>& Items, TArray<FString>& Errors);
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
    virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#endif
};

/** Derived value only. Not serialized as another editable source. */
USTRUCT(BlueprintType)
struct FFPSResolvedWeaponStats
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 MagazineSize = 0;
    UPROPERTY(BlueprintReadOnly) float ReloadDuration = 0;
    UPROPERTY(BlueprintReadOnly) float AimTransitionTime = 0;
    UPROPERTY(BlueprintReadOnly) float AimFOV = 0;
    UPROPERTY(BlueprintReadOnly) FVector AimLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) float VerticalRecoilMultiplier = 1.f;
    UPROPERTY(BlueprintReadOnly) float HorizontalRecoilMultiplier = 1.f;
    UPROPERTY(BlueprintReadOnly) TSoftObjectPtr<USoundBase> FireSound;
};

/** Does not load assets or mutate weapon/character state. Failure leaves Out unchanged. */
class MCP_TEST_API FFPSAttachmentResolver
{
public:
    static bool ValidateOffers(const UFPSWeaponDefinition& Weapon, TArray<FString>& Errors);
    static bool Resolve(const UFPSWeaponDefinition& Weapon, const TArray<const UFPSAttachmentDefinition*>& Attachments,
        FFPSResolvedWeaponStats& Out, TArray<FString>& Errors);
};
