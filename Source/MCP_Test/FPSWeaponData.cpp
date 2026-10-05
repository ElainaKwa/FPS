#include "FPSWeaponData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "Misc/DataValidation.h"
#include "Sound/SoundBase.h"
#include "Misc/CommandLine.h"

void UFPSWeaponDefinition::ValidateDefinition(TArray<FString>& Errors) const
{
    auto Require = [&](bool bValid, const TCHAR* Reason) { if (!bValid) Errors.Add(GetPathName() + TEXT(": ") + Reason); };
    auto SavedResource = [](const UObject* Object) { return Object && Object->IsAsset() && !Object->HasAnyFlags(RF_Transient) && Object->GetOutermost() != GetTransientPackage(); };
    const FString Id = WeaponId.ToString();
    Require(Id.StartsWith(TEXT("Weapon.")) && Id.Len() > 7 && !Id.Contains(TEXT(" ")) && !Id.Contains(TEXT(":")), TEXT("WeaponId must be Weapon.<stable name>"));
    Require(!DisplayName.IsEmpty(), TEXT("DisplayName required"));
    Require(Slot == EFPSWeaponSlot::Primary || Slot == EFPSWeaponSlot::Secondary, TEXT("Invalid slot"));
    Require(Category == EFPSWeaponCategory::Rifle || Category == EFPSWeaponCategory::SMG || Category == EFPSWeaponCategory::Pistol, TEXT("Invalid category"));
    Require(MagazineSize > 0 && InitialMagazine >= 0 && InitialMagazine <= MagazineSize && InitialReserve >= 0, TEXT("Invalid ammunition capacity or initial ammunition"));
    auto Numeric = [&](float Value, const TCHAR* Field, bool AllowZero = false)
    {
        if (!FMath::IsFinite(Value) || (AllowZero ? Value < 0 : Value <= 0))
            Errors.Add(FString::Printf(TEXT("%s: %s=%g must be finite and %s"), *GetPathName(), Field, Value, AllowZero ? TEXT("nonnegative") : TEXT("positive")));
    };
    Numeric(Damage, TEXT("Damage"));
    Numeric(FireInterval, TEXT("FireInterval"));
    Numeric(Range, TEXT("Range"));
    Require(FireMode == EFPSFireMode::Single || FireMode == EFPSFireMode::Automatic || FireMode == EFPSFireMode::Burst, TEXT("Invalid fire mode"));
    Require(BurstCount >= 2, TEXT("BurstCount must be at least two"));
    Numeric(BurstInterval, TEXT("BurstInterval"));
    Numeric(RecoilRecoveryTime, TEXT("RecoilRecoveryTime"));
    Numeric(ReloadDuration, TEXT("ReloadDuration"));
    Numeric(InspectDuration, TEXT("InspectDuration"));
    Numeric(EquipReadyTime, TEXT("EquipReadyTime"), true);
    Numeric(EquipBlendTime, TEXT("EquipBlendTime"), true);
    Numeric(AimTransitionTime, TEXT("AimTransitionTime"), true);
    Numeric(AimRecoilMultiplier, TEXT("AimRecoilMultiplier"), true);
    Require(FMath::IsFinite(AimFOV) && AimFOV > 0 && AimFOV < 180, TEXT("AimFOV must be between 0 and 180"));
    Require(!HipLocation.ContainsNaN() && !AimLocation.ContainsNaN() && !MuzzleLocation.ContainsNaN(), TEXT("Invalid presentation location"));
    for (const auto* View : {&FirstPerson, &ThirdPerson, &Pickup})
    {
        Require(SavedResource(View->Mesh.Get()), TEXT("All three presentation meshes must reference saved assets"));
        const FVector Scale = View->Transform.GetScale3D();
        Require(View->Transform.IsValid() && Scale.X > 0 && Scale.Y > 0 && Scale.Z > 0, TEXT("Presentation transform must be finite with positive scale"));
    }
    for (const auto* Sound : {FireSound.Get(), ReloadStartSound.Get(), ReloadEndSound.Get(), EquipSound.Get(), InspectSound.Get(), EmptySound.Get()})
        Require(SavedResource(Sound), TEXT("All firing/operation sounds must reference saved assets"));
    Require(bNoRecoil ? RecoilCurve.IsNull() : RecoilCurve.IsValid(), TEXT("Explicit no-recoil requires no curve; otherwise a curve is mandatory"));
    if (RecoilCurve)
    {
        Require(SavedResource(RecoilCurve.Get()), TEXT("Recoil curve must be a saved asset"));
        for (const FRichCurve& Curve : RecoilCurve->FloatCurves)
        {
            Require(Curve.GetNumKeys() >= 2 && Curve.GetFirstKey().Time <= 0 && Curve.GetLastKey().Time >= MagazineSize - 1, TEXT("Each recoil channel must cover shot 0 through MagazineSize-1"));
            for (const auto& Key : Curve.GetConstRefOfKeys())
                Require(FMath::IsFinite(Key.Time) && FMath::IsFinite(Key.Value) && FMath::IsFinite(Key.ArriveTangent) && FMath::IsFinite(Key.LeaveTangent), TEXT("Nonfinite recoil key"));
        }
    }
}

FVector UFPSWeaponDefinition::RecoilAt(int32 Shot) const
{
    if (bNoRecoil || !RecoilCurve) return FVector::ZeroVector;
    FVector Result;
    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        const FRichCurve& Curve = RecoilCurve->FloatCurves[Axis];
        Result[Axis] = Curve.Eval(FMath::Clamp(float(Shot), Curve.GetFirstKey().Time, Curve.GetLastKey().Time));
    }
    return Result;
}

void UFPSWeaponCatalog::Discover(TArray<UFPSWeaponDefinition*>& Out, TArray<FString>& Errors)
{
    auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
    TArray<FAssetData> Assets;
    Registry.GetAssetsByClass(UFPSWeaponDefinition::StaticClass()->GetClassPathName(), Assets, true);
    for (const FAssetData& Asset : Assets)
        if (Asset.PackageName.ToString().StartsWith(TEXT("/Game/")))
        {
            auto* Def = Cast<UFPSWeaponDefinition>(Asset.GetAsset());
            if (Def) { for (const auto& Path : Def->Resources()) Path.TryLoad(); Out.Add(Def); }
            else Errors.Add(Asset.GetObjectPathString() + TEXT(": failed to load definition"));
        }
    Out.Sort([](const auto& A, const auto& B) { return A.WeaponId.LexicalLess(B.WeaponId); });
}

void UFPSWeaponCatalog::ValidateCatalog(const TArray<UFPSWeaponDefinition*>& Items, TArray<FString>& Errors)
{
    TMap<FName, const UFPSWeaponDefinition*> Seen;
    for (const auto* Def : Items)
    {
        Def->ValidateDefinition(Errors);
        Def->ContentDigest(Errors);
        if (const auto* Previous = Seen.Find(Def->WeaponId))
            Errors.Add(FString::Printf(TEXT("Duplicate WeaponId %s: %s and %s"), *Def->WeaponId.ToString(), *(*Previous)->GetPathName(), *Def->GetPathName()));
        Seen.Add(Def->WeaponId, Def);
        if (UAssetManager::Get().GetPrimaryAssetPath(Def->GetPrimaryAssetId()) != FSoftObjectPath(Def))
            Errors.Add(Def->GetPathName() + TEXT(": Asset Manager identity/path mismatch; check scan/cook settings"));
    }
    const auto* Settings = GetDefault<UFPSWeaponCatalog>();
    for (const auto Slot : {EFPSWeaponSlot::Primary, EFPSWeaponSlot::Secondary})
    {
        const FName Id = Slot == EFPSWeaponSlot::Primary ? Settings->StartingPrimary : Settings->StartingSecondary;
        const auto* Found = Seen.Find(Id);
        if (!Found || (*Found)->Slot != Slot) Errors.Add(TEXT("Missing or wrong-slot starting weapon: ") + Id.ToString());
    }
}

const UFPSWeaponDefinition* UFPSWeaponCatalog::Find(FName WeaponId) const
{
    const auto* Found = Definitions.Find(WeaponId);
    return Found ? Found->Get() : nullptr;
}

FFPSWeaponInstance UFPSWeaponCatalog::CreateInstance(FName WeaponId) const
{
    FFPSWeaponInstance Instance;
    if (const auto* Def = Find(WeaponId))
    {
        Instance.InstanceId = FGuid::NewGuid(); Instance.WeaponId = WeaponId;
        Instance.Magazine = Def->InitialMagazine; Instance.Reserve = Def->InitialReserve;
    }
    else UE_LOG(LogTemp, Error, TEXT("FPS_WEAPON_DATA cannot create %s"), *WeaponId.ToString());
    return Instance;
}

#if WITH_EDITOR
EDataValidationResult UFPSWeaponDefinition::IsDataValid(FDataValidationContext& Context) const
{
    TArray<FString> Errors;
    for (const auto& Path : Resources()) Path.TryLoad();
    ValidateDefinition(Errors);
    ContentDigest(Errors);
    TArray<UFPSWeaponDefinition*> Items;
    UFPSWeaponCatalog::Discover(Items, Errors);
    for (const auto* Other : Items)
        if (Other != this && Other->WeaponId == WeaponId) Errors.Add(TEXT("Duplicate WeaponId: ") + Other->GetPathName());
    for (const auto& Error : Errors) Context.AddError(FText::FromString(Error));
    return Errors.IsEmpty() ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
EDataValidationResult AFPSWeaponStand::IsDataValid(FDataValidationContext& Context) const
{
    if (auto* Def = Definition.LoadSynchronous()) return Def->IsDataValid(Context);
    Context.AddError(FText::FromString(GetPathName() + TEXT(": weapon stand requires explicit definition")));
    return EDataValidationResult::Invalid;
}
#endif
