#include "FPSHitDamage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Misc/DataValidation.h"

bool FFPSHitDamageMultipliers::IsValid() const
{
    return FMath::IsFinite(Head) && FMath::IsFinite(Torso) && FMath::IsFinite(Limb) && Head >= 0 && Torso >= 0 && Limb >= 0;
}
float FFPSHitDamageMultipliers::ForRegion(EFPSHitRegion Region) const
{
    switch (Region) { case EFPSHitRegion::Head: return Head; case EFPSHitRegion::Torso: return Torso;
        case EFPSHitRegion::Limb: return Limb; default: return 0; }
}
void UFPSHitDamageProfile::ValidateProfile(TArray<FString>& Errors) const
{
    if (!IsAsset() || HasAnyFlags(RF_Transient) || !ProfileId.ToString().StartsWith(TEXT("HitDamage.")) ||
        ProfileId.ToString().Len() <= 10 || ProfileId.ToString().Contains(TEXT(" ")) || !Multipliers.IsValid())
        Errors.Add(GetPathName() + TEXT(": saved HitDamage.<stable ID> and finite nonnegative multipliers required"));
    auto& Manager = UAssetManager::Get();
    if (Manager.GetPrimaryAssetPath(GetPrimaryAssetId()) != FSoftObjectPath(this))
        Errors.Add(GetPathName() + TEXT(": hit damage Asset Manager identity mismatch"));
    TArray<FAssetData> Assets;
    FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssetsByClass(StaticClass()->GetClassPathName(), Assets, true);
    int32 Count = 0;
    for (const auto& Asset : Assets) { FName Id; Asset.GetTagValue(TEXT("ProfileId"), Id); if (Id == ProfileId) ++Count; }
    if (Count != 1) Errors.Add(GetPathName() + TEXT(": missing or duplicate hit damage identity"));
}
void UFPSHitDamageProfile::ValidateAll(TArray<FString>& Errors)
{
    TArray<FAssetData> Assets;
    auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
    Registry.GetAssetsByClass(StaticClass()->GetClassPathName(), Assets, true);
    for (const auto& Asset : Assets)
        if (const auto* Profile = Cast<UFPSHitDamageProfile>(Asset.GetAsset())) Profile->ValidateProfile(Errors);
        else Errors.Add(Asset.GetObjectPathString() + TEXT(": hit damage profile failed to load"));
}
#if WITH_EDITOR
EDataValidationResult UFPSHitDamageProfile::IsDataValid(FDataValidationContext& Context) const
{
    TArray<FString> Errors; ValidateProfile(Errors);
    for (const auto& Error : Errors) Context.AddError(FText::FromString(Error));
    return Errors.IsEmpty() ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
