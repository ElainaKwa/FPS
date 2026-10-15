#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FPSHitDamage.generated.h"

UENUM(BlueprintType)
enum class EFPSHitRegion : uint8 { None, Head, Torso, Limb };

/** Values copied into the authority's shot context; never accepted from a client. */
USTRUCT(BlueprintType)
struct FFPSHitDamageMultipliers
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float Head = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float Torso = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float Limb = .75f;
    bool IsValid() const;
    float ForRegion(EFPSHitRegion Region) const;
};

/** Shared tuning, independent of character meshes and bone names. */
UCLASS(BlueprintType)
class MCP_TEST_API UFPSHitDamageProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AssetRegistrySearchable) FName ProfileId;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FFPSHitDamageMultipliers Multipliers;
    virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("HitDamageProfile"), ProfileId); }
    void ValidateProfile(TArray<FString>& Errors) const;
    static void ValidateAll(TArray<FString>& Errors);
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
