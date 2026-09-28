#include "FPSWeaponData.h"

void UFPSWeaponCatalog::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    auto Add = [this](EFPSWeaponModel Model, const TCHAR* Name, EFPSWeaponCategory Category, int32 Magazine, int32 Reserve,
        float Damage, float Interval, float Reload, float Kick, bool bAuto)
    {
        auto* Def = NewObject<UFPSWeaponDefinition>(this);
        Def->Model = Model;
        Def->DisplayName = FText::FromString(Name);
        Def->Category = Category;
        Def->Slot = Category == EFPSWeaponCategory::Pistol ? EFPSWeaponSlot::Secondary : EFPSWeaponSlot::Primary;
        Def->MagazineSize = Magazine; Def->InitialReserve = Reserve; Def->Damage = Damage;
        Def->FireInterval = Interval; Def->ReloadDuration = Reload; Def->bAutomatic = bAuto;
        Def->AimFOV = Category == EFPSWeaponCategory::Rifle ? 62 : 72;
        Def->Range = Category == EFPSWeaponCategory::Rifle ? 9000 : 5000;
        Def->PresentationScale = FVector(1);
        const FString MeshName = TEXT("SM_WP_") + StaticEnum<EFPSWeaponModel>()->GetNameStringByValue(static_cast<int64>(Model));
        Def->Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(FString::Printf(TEXT("/Game/FPSDemo/Weapons/Meshes/%s.%s"), *MeshName, *MeshName)));
        switch (Model)
        {
        case EFPSWeaponModel::AK: Def->MuzzleLocation.X = 57; Def->SightHeight = 6; break;
        case EFPSWeaponModel::MP5: Def->MuzzleLocation.X = 38; Def->SightHeight = 6; break;
        case EFPSWeaponModel::UMP45: Def->MuzzleLocation.X = 39; break;
        case EFPSWeaponModel::M9: Def->MuzzleLocation = FVector(21, 0, .6f); Def->SightHeight = 5; break;
        case EFPSWeaponModel::HeavyPistol: Def->MuzzleLocation = FVector(26, 0, .6f); Def->SightHeight = 5; break;
        default: break;
        }
        Def->RecoilCurve = NewObject<UCurveVector>(Def);
        for (int32 Shot = 0; Shot < 8; ++Shot)
        {
            Def->RecoilCurve->FloatCurves[0].AddKey(Shot, Kick * (1.f + Shot * .1f));
            Def->RecoilCurve->FloatCurves[1].AddKey(Shot, Kick * (Shot % 2 == 0 ? .22f : -.3f));
            Def->RecoilCurve->FloatCurves[2].AddKey(Shot, 1.f + Shot * .04f);
        }
        Definitions.Add(Model, Def);
    };
    Add(EFPSWeaponModel::AK, TEXT("AK"), EFPSWeaponCategory::Rifle, 30, 90, 34, .11f, 2.4f, 1.25f, true);
    Add(EFPSWeaponModel::M4, TEXT("M4"), EFPSWeaponCategory::Rifle, 30, 120, 25, .085f, 2.1f, .8f, true);
    Add(EFPSWeaponModel::MP5, TEXT("MP5"), EFPSWeaponCategory::SMG, 30, 150, 20, .065f, 1.8f, .5f, true);
    Add(EFPSWeaponModel::UMP45, TEXT("UMP45"), EFPSWeaponCategory::SMG, 25, 125, 29, .105f, 2.2f, .9f, true);
    Add(EFPSWeaponModel::M9, TEXT("M9"), EFPSWeaponCategory::Pistol, 15, 60, 25, .2f, 1.4f, .85f, false);
    Add(EFPSWeaponModel::HeavyPistol, TEXT("Heavy Pistol"), EFPSWeaponCategory::Pistol, 7, 35, 55, .38f, 1.9f, 2.f, false);
}

const UFPSWeaponDefinition* UFPSWeaponCatalog::Find(EFPSWeaponModel Model) const
{
    const auto* Found = Definitions.Find(Model);
    return Found ? Found->Get() : nullptr;
}

FFPSWeaponInstance UFPSWeaponCatalog::CreateInstance(EFPSWeaponModel Model) const
{
    FFPSWeaponInstance Instance;
    if (const auto* Def = Find(Model))
    {
        Instance.InstanceId = FGuid::NewGuid(); Instance.Model = Model;
        Instance.Magazine = Def->MagazineSize; Instance.Reserve = Def->InitialReserve;
    }
    return Instance;
}
