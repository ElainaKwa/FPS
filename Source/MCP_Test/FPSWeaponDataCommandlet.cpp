#include "FPSWeaponDataCommandlet.h"
#include "FPSWeaponData.h"
#include "FPSHitDamage.h"
#include <limits>
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Misc/DataValidation.h"
#include "Sound/SoundBase.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectIterator.h"
#include "Engine/GameInstance.h"
#endif

UFPSWeaponDataCommandlet::UFPSWeaponDataCommandlet()
{
    IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}
#if WITH_EDITOR
namespace
{
bool SaveAsset(UObject* Asset)
{
    UPackage* Package = Asset->GetOutermost();
    Package->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(Package, Asset,
        *FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}

// Historical constants are confined to this explicit one-time import. Never used at runtime.
// Order is ONLY the migration mapping for old FPSWeapon1..6 tags.
struct FLegacy
{
    const TCHAR* Name; const TCHAR* Display; EFPSWeaponCategory Category; EFPSWeaponSlot Slot;
    int32 Magazine; int32 Reserve; float Damage; float Interval; float Reload; float Kick;
    float Range; float FOV; bool Auto; FVector Muzzle; float Sight;
};
const FLegacy Legacy[] = {
    {TEXT("AK"),TEXT("AK"),EFPSWeaponCategory::Rifle,EFPSWeaponSlot::Primary,30,90,34,.11f,2.4f,1.25f,9000,62,true,FVector(57,0,0),6},
    {TEXT("M4"),TEXT("M4"),EFPSWeaponCategory::Rifle,EFPSWeaponSlot::Primary,30,120,25,.085f,2.1f,.8f,9000,62,true,FVector(56,0,0),7},
    {TEXT("MP5"),TEXT("MP5"),EFPSWeaponCategory::SMG,EFPSWeaponSlot::Primary,30,150,20,.065f,1.8f,.5f,5000,72,true,FVector(38,0,0),6},
    {TEXT("UMP45"),TEXT("UMP45"),EFPSWeaponCategory::SMG,EFPSWeaponSlot::Primary,25,125,29,.105f,2.2f,.9f,5000,72,true,FVector(39,0,0),7},
    {TEXT("M9"),TEXT("M9"),EFPSWeaponCategory::Pistol,EFPSWeaponSlot::Secondary,15,60,25,.2f,1.4f,.85f,5000,72,false,FVector(21,0,.6f),5},
    {TEXT("HeavyPistol"),TEXT("Heavy Pistol"),EFPSWeaponCategory::Pistol,EFPSWeaponSlot::Secondary,7,35,55,.38f,1.9f,2.f,5000,72,false,FVector(26,0,.6f),5}
};
const TCHAR* Maps[] = {TEXT("/Game/FPSDemo/Maps/LV_FPSFoundry"), TEXT("/Game/FPSDemo/Maps/LV_FPSTraining")};

bool MigrateDefinitions(TArray<FString>& Errors)
{
    for (const auto& Row : Legacy)
    {
        const FString DefPath = FString::Printf(TEXT("/Game/FPSDemo/Weapons/Definitions/DA_%s"), Row.Name);
        if (FPackageName::DoesPackageExist(DefPath))
        {
            UE_LOG(LogTemp, Display, TEXT("FPS_WEAPON_DATA preserve existing %s"), *DefPath);
            continue;
        }
        const FString CurvePath = FString::Printf(TEXT("/Game/FPSDemo/Weapons/Curves/CR_%s"), Row.Name);
        if (FPackageName::DoesPackageExist(CurvePath)) { Errors.Add(TEXT("Refusing to overwrite existing curve: ") + CurvePath); return false; }
        auto* Def = NewObject<UFPSWeaponDefinition>(CreatePackage(*DefPath), *FPackageName::GetLongPackageAssetName(DefPath), RF_Public | RF_Standalone);
        auto* Curve = NewObject<UCurveVector>(CreatePackage(*CurvePath), *FPackageName::GetLongPackageAssetName(CurvePath), RF_Public | RF_Standalone);
        Def->WeaponId = FName(*(FString(TEXT("Weapon.")) + Row.Name));
        Def->DisplayName = FText::FromString(Row.Display); Def->Category = Row.Category; Def->Slot = Row.Slot;
        Def->MagazineSize = Row.Magazine; Def->InitialMagazine = Row.Magazine; Def->InitialReserve = Row.Reserve;
        Def->Damage = Row.Damage; Def->FireInterval = Row.Interval; Def->ReloadDuration = Row.Reload; Def->FireMode = Row.Auto ? EFPSFireMode::Automatic : EFPSFireMode::Single;
        Def->Range = Row.Range; Def->AimFOV = Row.FOV; Def->MuzzleLocation = Row.Muzzle;
        const bool Pistol = Row.Category == EFPSWeaponCategory::Pistol;
        Def->HipLocation = Pistol ? FVector(32,12,-14) : FVector(42,18,-18);
        Def->AimLocation = FVector(Pistol ? 24 : 38,0,-Row.Sight+.3f);
        const FString MeshPath = FString::Printf(TEXT("/Game/FPSDemo/Weapons/Meshes/SM_WP_%s.SM_WP_%s"), Row.Name, Row.Name);
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
        Def->FirstPerson.Mesh = Def->ThirdPerson.Mesh = Def->Pickup.Mesh = Mesh;
        Def->ThirdPerson.Transform.SetLocation(FVector(Pistol ? 39 : 43,8,17));
        Def->Pickup.Transform.SetScale3D(FVector(.65f));
        Def->FireSound = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/FPSDemo/Audio/S_Fire_%s.S_Fire_%s"),Row.Name,Row.Name));
        Def->ReloadStartSound = LoadObject<USoundBase>(nullptr,TEXT("/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart"));
        Def->ReloadEndSound = LoadObject<USoundBase>(nullptr,TEXT("/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd"));
        Def->EquipSound = LoadObject<USoundBase>(nullptr,TEXT("/Game/FPSDemo/Audio/S_Equip.S_Equip"));
        Def->InspectSound = LoadObject<USoundBase>(nullptr,TEXT("/Game/FPSDemo/Audio/S_Inspect.S_Inspect"));
        Def->EmptySound = LoadObject<USoundBase>(nullptr,TEXT("/Game/FPSDemo/Audio/S_Empty.S_Empty"));
        for (int32 Shot = 0; Shot < FMath::Max(8, Row.Magazine); ++Shot)
        {
            const int32 OriginalShot = FMath::Min(Shot,7);
            const float Values[] = {Row.Kick*(1.f+OriginalShot*.1f), Row.Kick*(OriginalShot%2 == 0 ? .22f : -.3f),1.f+OriginalShot*.04f};
            for (int32 Axis = 0; Axis < 3; ++Axis)
            {
                const auto Key = Curve->FloatCurves[Axis].AddKey(Shot,Values[Axis]);
                Curve->FloatCurves[Axis].SetKeyInterpMode(Key, RCIM_Linear);
                Curve->FloatCurves[Axis].PostInfinityExtrap = RCCE_Constant;
            }
        }
        Def->RecoilCurve = Curve;
        Def->ValidateDefinition(Errors);
        if (!Errors.IsEmpty()) return false;
        FAssetRegistryModule::AssetCreated(Curve);
        FAssetRegistryModule::AssetCreated(Def);
        if (!SaveAsset(Curve) || !SaveAsset(Def)) { Errors.Add(TEXT("Failed saving ") + DefPath); return false; }
    }
    return true;
}

// Explicit editor authoring operation. Saved assets remain the only runtime source.
bool CreateBurstRifle(TArray<FString>& Errors)
{
    const FString Path = TEXT("/Game/FPSDemo/Weapons/Definitions/DA_BR3");
    const FString CurvePath = TEXT("/Game/FPSDemo/Weapons/Curves/CR_BR3");
    if (FPackageName::DoesPackageExist(Path)) return true; // Never overwrite later tuning.
    if (FPackageName::DoesPackageExist(CurvePath)) { Errors.Add(TEXT("BR3 curve already exists without definition")); return false; }
    auto* Source = LoadObject<UFPSWeaponDefinition>(nullptr,TEXT("/Game/FPSDemo/Weapons/Definitions/DA_M4.DA_M4"));
    if (!Source) { Errors.Add(TEXT("Missing M4 presentation template")); return false; }
    auto* Def = DuplicateObject<UFPSWeaponDefinition>(Source,CreatePackage(*Path),TEXT("DA_BR3"));
    Def->SetFlags(RF_Public | RF_Standalone);
    auto* Curve = NewObject<UCurveVector>(CreatePackage(*CurvePath),TEXT("CR_BR3"),RF_Public | RF_Standalone);
    Def->WeaponId = TEXT("Weapon.BR3"); Def->DisplayName = FText::FromString(TEXT("BR-3"));
    Def->FireMode = EFPSFireMode::Burst; Def->BurstCount = 3; Def->FireInterval = .09f; Def->BurstInterval = .35f;
    Def->Damage = 28; Def->MagazineSize = Def->InitialMagazine = 30; Def->InitialReserve = 90;
    Def->ReloadDuration = 2.3f; Def->EquipReadyTime = .25f; Def->EquipBlendTime = .4f;
    Def->RecoilRecoveryTime = .65f; Def->RecoilCurve = Curve;
    for (int32 Shot = 0; Shot < 30; ++Shot)
    {
        const float Values[] = {.72f + .08f * (Shot % 3) + .015f * Shot, (Shot % 3 - 1) * .12f, 1.f + .06f * (Shot % 3)};
        for (int32 Axis = 0; Axis < 3; ++Axis)
        {
            const auto Key = Curve->FloatCurves[Axis].AddKey(Shot,Values[Axis]);
            Curve->FloatCurves[Axis].SetKeyInterpMode(Key,RCIM_Linear);
            Curve->FloatCurves[Axis].PostInfinityExtrap = RCCE_Constant;
        }
    }
    Def->ValidateDefinition(Errors);
    if (!Errors.IsEmpty()) return false;
    FAssetRegistryModule::AssetCreated(Curve); FAssetRegistryModule::AssetCreated(Def);
    if (!SaveAsset(Curve) || !SaveAsset(Def)) { Errors.Add(TEXT("Failed saving BR3")); return false; }
    return true;
}

bool ProcessMaps(bool bMigrate, bool bResave, FString& Review, TArray<FString>& Errors)
{
    for (const TCHAR* MapPath : Maps)
    {
        auto* World = UEditorLoadingAndSavingUtils::LoadMap(MapPath);
        if (!World) { Errors.Add(FString(TEXT("Cannot load map ")) + MapPath); return false; }
        TArray<ATargetPoint*> Old;
        for (TActorIterator<ATargetPoint> It(World); It; ++It)
            if (!It->IsA<AFPSWeaponStand>()) Old.Add(*It);
        bool Changed = false;
        for (auto* Marker : Old)
            for (int32 Index = 0; Index < UE_ARRAY_COUNT(Legacy); ++Index)
            {
                const FName Tag(*FString::Printf(TEXT("FPSWeapon%d"),Index+1));
                if (!Marker->ActorHasTag(Tag)) continue;
                if (!bMigrate) { Errors.Add(Marker->GetPathName() + TEXT(": legacy numeric weapon marker")); continue; }
                const FString Path = FString::Printf(TEXT("/Game/FPSDemo/Weapons/Definitions/DA_%s.DA_%s"),Legacy[Index].Name,Legacy[Index].Name);
                auto* Def = LoadObject<UFPSWeaponDefinition>(nullptr,*Path);
                if (!Def) { Errors.Add(TEXT("Missing migration definition ") + Path); return false; }
                auto* Stand = World->SpawnActor<AFPSWeaponStand>(AFPSWeaponStand::StaticClass(),Marker->GetActorTransform());
                Stand->Definition = Def; Stand->DisplayOrder = Index;
                Stand->SetActorLabel(Marker->GetActorLabel()); Stand->SetFolderPath(Marker->GetFolderPath());
                Stand->Tags = Marker->Tags; Stand->Tags.Remove(Tag);
                World->EditorDestroyActor(Marker,true);
                Changed = true;
                break;
            }
        TArray<AFPSWeaponStand*> Stands;
        for (TActorIterator<AFPSWeaponStand> It(World); It; ++It) Stands.Add(*It);
        if (Stands.IsEmpty()) Errors.Add(FString(MapPath) + TEXT(": no weapon stands"));
        Stands.Sort([](const auto& A, const auto& B)
        {
            const FString KeyA = A.GetActorLabel() + A.GetActorTransform().ToString();
            const FString KeyB = B.GetActorLabel() + B.GetActorTransform().ToString();
            return KeyA < KeyB;
        });
        Review += FString::Printf(TEXT("\n## %s\n"),MapPath);
        for (auto* Stand : Stands)
        {
            FDataValidationContext Context;
            if (Stand->IsDataValid(Context) == EDataValidationResult::Invalid)
                for (const auto& Issue : Context.GetIssues()) Errors.Add(Issue.Message.ToString());
            Review += FString::Printf(TEXT("- %s | %s | order=%d | %s | %s\n"),*Stand->GetActorLabel(),
                Stand->Definition ? *Stand->Definition->GetPrimaryAssetId().ToString() : TEXT("MISSING"),Stand->DisplayOrder,
                *Stand->GetActorTransform().ToString(), Stand->Definition ? *Stand->Definition->GetPathName() : TEXT("MISSING"));
        }
        if ((Changed || bResave) && Errors.IsEmpty() && !UEditorLoadingAndSavingUtils::SaveMap(World,MapPath)) Errors.Add(FString(TEXT("Failed saving ")) + MapPath);
    }
    return Errors.IsEmpty();
}

bool SelfTest(UFPSWeaponDefinition* Def)
{
    bool Pass = true;
    auto Check = [&](bool Result, const TCHAR* What)
    {
        Pass &= Result;
        UE_LOG(LogTemp, Display, TEXT("FPS_WEAPON_DATA_TEST %s %s"),Result ? TEXT("PASS") : TEXT("FAIL"),What);
    };
    auto Invalid = [&]() { TArray<FString> Errors; Def->ValidateDefinition(Errors); return !Errors.IsEmpty(); };
    const int32 Magazine = Def->MagazineSize;
    Def->MagazineSize = 0; Check(Invalid(),TEXT("zero capacity rejected")); Def->MagazineSize = Magazine;
    const float Interval = Def->FireInterval;
    Def->FireInterval = -1; Check(Invalid(),TEXT("negative interval rejected")); Def->FireInterval = Interval;
    Def->FireInterval = -1;
    FDataValidationContext InvalidContext;
    Check(Def->IsDataValid(InvalidContext) == EDataValidationResult::Invalid && InvalidContext.GetNumErrors() > 0, TEXT("editor validator rejects invalid interval"));
    Def->FireInterval = Interval;
    auto* Sound = Def->FireSound.Get();
    Def->FireSound = nullptr; Check(Invalid(),TEXT("missing sound rejected")); Def->FireSound = Sound;
    auto* Curve = Def->RecoilCurve.Get();
    Def->RecoilCurve = nullptr; Check(Invalid(),TEXT("missing recoil curve rejected"));
    Def->bNoRecoil = true; Check(!Invalid(),TEXT("explicit no-recoil accepted"));
    Def->RecoilCurve = TSoftObjectPtr<UCurveVector>(FSoftObjectPath(TEXT("/Game/MissingRecoil.MissingRecoil")));
    Check(Invalid(),TEXT("no-recoil rejects unresolved nonempty curve reference"));
    Def->bNoRecoil = false; Def->RecoilCurve = Curve;
    Def->MagazineSize = 100; Check(Invalid(),TEXT("short curve rejected")); Def->MagazineSize = Magazine;
    Check(Def->RecoilAt(1000).Equals(Def->RecoilAt(31),1.e-5),TEXT("curve end clamps"));
    auto* Copy = DuplicateObject<UFPSWeaponDefinition>(Def, GetTransientPackage());
    FDataValidationContext DuplicateContext;
    Check(Copy->IsDataValid(DuplicateContext) == EDataValidationResult::Invalid, TEXT("editor validator rejects duplicate identity"));
    TArray<FString> Errors;
    UFPSWeaponCatalog::ValidateCatalog({Def,Copy},Errors);
    Check(Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("Duplicate WeaponId")); }),TEXT("duplicate identity rejected"));
    const auto Id = Copy->GetPrimaryAssetId();
    Copy->Rename(TEXT("RenamedWeaponForTest"),CreatePackage(TEXT("/Temp/FPSWeaponMoved")),REN_DontCreateRedirectors | REN_NonTransactional);
    Copy->DisplayName = FText::FromString(TEXT("Different display name"));
    Check(Copy->GetPrimaryAssetId() == Id,TEXT("object rename move display name preserve identity"));
    return Pass;
}
}
#endif

#if WITH_EDITOR
namespace
{
bool HitDamageSelfTest()
{
    bool Passed = true;
    auto Check = [&](bool Value, const TCHAR* What) { Passed &= Value; UE_LOG(LogTemp, Display, TEXT("FPS_HIT_DATA %s %s"), Value ? TEXT("PASS") : TEXT("FAIL"), What); };
    auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UFPSWeaponCatalog> Catalog(NewObject<UFPSWeaponCatalog>(GI.Get()));
    const TArray<FName> Ids{TEXT("Weapon.M4"),TEXT("Weapon.M9")};
    if (!Catalog->PrepareDependencies(Ids)) { Check(false,TEXT("baseline content ready")); return false; }
    auto* Def=const_cast<UFPSWeaponDefinition*>(Catalog->Find(TEXT("Weapon.M4")));
    auto* Profile=Def->HitDamageProfile.Get();
    if(!Profile){Check(false,TEXT("profile loaded"));return false;}
    const auto Original=Profile->Multipliers;
    Check(Original.Head==2 && Original.Torso==1 && Original.Limb==.75f,TEXT("saved default profile reloads exact multipliers"));
    auto Invalid=[&](){TArray<FString> Errors; Def->ValidateDefinition(Errors);return !Errors.IsEmpty();};
    Profile->Multipliers.Head=0; Check(!Invalid(),TEXT("zero multiplier accepted"));
    Profile->Multipliers.Head=-1;Check(Invalid(),TEXT("negative multiplier rejected"));
    Check(!Catalog->PrepareDependencies(Ids),TEXT("required invalid profile blocks session"));
    Profile->Multipliers=Original;
    Profile->Multipliers.Limb=std::numeric_limits<float>::infinity();Check(Invalid(),TEXT("infinite multiplier rejected"));
    Profile->Multipliers=Original;
    TArray<FString> Errors;const FString Before=Def->ContentDigest(Errors);
    Profile->Multipliers.Head+=.1f;Check(Def->ContentDigest(Errors)!=Before,TEXT("same ID multiplier edit changes digest"));Profile->Multipliers=Original;
    const auto Ref=Def->HitDamageProfile;Def->HitDamageProfile.Reset();
    Check(!Catalog->PrepareDependencies(Ids),TEXT("missing required profile blocks session"));Def->HitDamageProfile=Ref;
    // Registry-only unrelated fixture is never saved and is removed before leaving this commandlet.
    auto* Bad=NewObject<UFPSHitDamageProfile>(CreatePackage(TEXT("/Game/HitDamageTest/Unrelated")),TEXT("Unrelated"),RF_Public|RF_Standalone);
    Bad->ProfileId=TEXT("HitDamage.UnrelatedFixture");Bad->Multipliers.Head=-1;
    FAssetRegistryModule::AssetCreated(Bad);
    Check(Catalog->PrepareDependencies(Ids),TEXT("unrelated invalid profile does not block M4 M9 session"));
    Errors.Reset();UFPSHitDamageProfile::ValidateAll(Errors);Check(!Errors.IsEmpty(),TEXT("unrelated invalid profile fails whole project validation"));
    FAssetRegistryModule::AssetDeleted(Bad);Bad->ClearFlags(RF_Standalone|RF_Public);
    TWeakObjectPtr<UFPSHitDamageProfile> Weak(Profile);Catalog->ReleaseSession();
    CollectGarbage(RF_NoFlags);
    Check(!Weak.IsValid()&&!Catalog->IsReady(),TEXT("profile collectible after session handle release"));
    Check(Passed,TEXT("COMPLETE"));return Passed;
}

bool ContentSelfTest()
{
    bool Passed = true;
    auto Check = [&](bool Value, const TCHAR* What) { Passed &= Value; UE_LOG(LogTemp, Display, TEXT("FPS_CONTENT_TEST %s %s"), Value ? TEXT("PASS") : TEXT("FAIL"), What); };
    auto Resident = []() { int32 Count = 0; for (TObjectIterator<UFPSWeaponDefinition> It; It; ++It) if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->IsAsset()) ++Count; return Count; };
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UFPSWeaponCatalog> Catalog(NewObject<UFPSWeaponCatalog>(GI.Get()));
    Check(Catalog->PrepareDependencies({TEXT("Weapon.M4"), TEXT("Weapon.M9")}), TEXT("unrelated invalid AK does not block M4/M9 session"));
    Check(Catalog->Find(TEXT("Weapon.AK")) == nullptr, TEXT("unrelated definition not published"));
    UE_LOG(LogTemp, Display, TEXT("FPS_CONTENT_TEST RESIDENT loaded=%d"), Resident());
    TWeakObjectPtr<UFPSWeaponDefinition> Weak;
    if (auto* Def = const_cast<UFPSWeaponDefinition*>(Catalog->Find(TEXT("Weapon.M4"))))
    {
        Weak = Def;
        TArray<FString> Errors;
        const FString Original = Def->ContentDigest(Errors);
        auto* Copy = DuplicateObject<UFPSWeaponDefinition>(Def, GetTransientPackage(), TEXT("DefinitionRenamed"));
        Check(Copy->ContentDigest(Errors) == Original, TEXT("definition package rename preserves digest"));
        ++Def->MagazineSize;
        Check(Def->ContentDigest(Errors) != Original, TEXT("same ID capacity edit changes digest")); --Def->MagazineSize;
        const float Interval = Def->FireInterval; Def->FireInterval += .001f;
        Check(Def->ContentDigest(Errors) != Original, TEXT("same ID firing interval edit changes digest")); Def->FireInterval = Interval;
        auto& Channel = Def->RecoilCurve->FloatCurves[0]; const FRichCurve Saved = Channel;
        Channel.SetKeyValue(Channel.GetFirstKeyHandle(), Channel.GetFirstKey().Value + .01f);
        Check(Def->ContentDigest(Errors) != Original, TEXT("curve content edit changes digest")); Channel = Saved;
        const auto Mesh = Def->FirstPerson.Mesh;
        const auto Versions = Def->SavedResourceVersions;
        Def->FirstPerson.Mesh = Catalog->Find(TEXT("Weapon.M9"))->FirstPerson.Mesh;
        Def->SavedResourceVersions.Append(Catalog->Find(TEXT("Weapon.M9"))->SavedResourceVersions);
        Check(Def->ContentDigest(Errors) != Original, TEXT("resource revision changes digest")); Def->FirstPerson.Mesh = Mesh; Def->SavedResourceVersions = Versions;
        Check(Errors.IsEmpty(), TEXT("all content revisions available"));
    }
    else Check(false, TEXT("loaded definition available"));
    Catalog->ReleaseSession();
    CollectGarbage(RF_NoFlags);
    Check(!Weak.IsValid() && !Catalog->IsReady(), TEXT("released definition collectible after handle release"));
    UE_LOG(LogTemp, Display, TEXT("FPS_CONTENT_TEST RESIDENT released=%d"), Resident());
    Check(!Catalog->PrepareDependencies({TEXT("Weapon.AK")}), TEXT("required invalid AK blocks session"));
    Check(!Catalog->Find(TEXT("Weapon.AK")), TEXT("failed session publishes no definition"));
    Catalog->ReleaseSession();
    Check(Passed, TEXT("COMPLETE"));
    return Passed;
}
}
#endif

int32 UFPSWeaponDataCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
    if (FParse::Param(*Params,TEXT("HitDamageSelfTest"))) return HitDamageSelfTest() ? 0 : 1;
    if (FParse::Param(*Params,TEXT("ContentSelfTest"))) return ContentSelfTest() ? 0 : 1;
    TArray<FString> Errors;
    const bool bMigrate = FParse::Param(*Params,TEXT("Migrate"));
    auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
    if (bMigrate) MigrateDefinitions(Errors);
    if (FParse::Param(*Params,TEXT("CreateBurstRifle"))) CreateBurstRifle(Errors);
    UAssetManager::Get().ScanPathsForPrimaryAssets(TEXT("WeaponDefinition"),{TEXT("/Game")},UFPSWeaponDefinition::StaticClass(),false,false,true);
    TArray<UFPSWeaponDefinition*> Items;
    UFPSWeaponCatalog::Discover(Items,Errors);
    if (FParse::Param(*Params,TEXT("ResaveSoftReferences")))
        for (auto* Def : Items) if (!SaveAsset(Def)) Errors.Add(TEXT("Failed saving soft references: ") + Def->GetPathName());
#if !UE_BUILD_SHIPPING
    FString InvalidId; FParse::Value(*Params,TEXT("FPSInvalidWeapon="),InvalidId);
    for (auto* Def : Items) if (Def->WeaponId.ToString() == InvalidId) Def->FireInterval = -1;
#endif
    UFPSWeaponCatalog::ValidateCatalog(Items,Errors);
    FString Review = TEXT("# Weapon asset review (GENERATED - DO NOT EDIT)\n\n- 用途：自动生成资料\n- 适用范围：已保存武器定义与地图武器台\n- 状态：只读导出，禁止手工修改\n- 最后更新：2026-10-15\n\nSource: saved Unreal assets. Regenerate with Tools/ValidateFPSWeaponData.ps1 -Export.\n");
    const auto* Defaults = GetDefault<UFPSWeaponCatalog>();
    Review += FString::Printf(TEXT("\nStarting primary=%s; secondary=%s\n"),*Defaults->StartingPrimary.ToString(),*Defaults->StartingSecondary.ToString());
    for (const auto* Def : Items)
    {
        Review += FString::Printf(TEXT("\n## %s\n\nAsset: %s\n\n```text\n"),*Def->GetPrimaryAssetId().ToString(),*Def->GetPathName());
        for (TFieldIterator<FProperty> It(UFPSWeaponDefinition::StaticClass(),EFieldIteratorFlags::ExcludeSuper); It; ++It)
        {
            FString Value;
            It->ExportText_InContainer(0,Value,Def,Def,const_cast<UFPSWeaponDefinition*>(Def),PPF_None);
            Review += It->GetName() + TEXT("=") + Value + TEXT("\n");
        }
        if (const auto* Profile=Def->HitDamageProfile.Get())
            Review += FString::Printf(TEXT("HitDamageProfileId=%s Head=%.9g Torso=%.9g Limb=%.9g\n"),*Profile->ProfileId.ToString(),Profile->Multipliers.Head,Profile->Multipliers.Torso,Profile->Multipliers.Limb);
        if (Def->RecoilCurve)
            for (int32 Axis = 0; Axis < 3; ++Axis)
                for (const auto& K : Def->RecoilCurve->FloatCurves[Axis].GetConstRefOfKeys())
                    Review += FString::Printf(TEXT("Curve[%d] time=%.9g value=%.9g interp=%d tangentMode=%d arrive=%.9g leave=%.9g weightMode=%d arriveWeight=%.9g leaveWeight=%.9g\n"),
                        Axis,K.Time,K.Value,int32(K.InterpMode),int32(K.TangentMode),K.ArriveTangent,K.LeaveTangent,int32(K.TangentWeightMode),K.ArriveTangentWeight,K.LeaveTangentWeight);
        Review += TEXT("```\n");
    }
    if (Errors.IsEmpty()) ProcessMaps(bMigrate,FParse::Param(*Params,TEXT("ResaveSoftReferences")),Review,Errors);
    if (Errors.IsEmpty() && FParse::Param(*Params,TEXT("SelfTest")) && !SelfTest(Items[0])) Errors.Add(TEXT("Self test failed"));
    const FString ReviewPath = FPaths::ProjectDir() / TEXT("Docs/FPSWeaponAssetReview.md");
    if (Errors.IsEmpty())
    {
        if (FParse::Param(*Params,TEXT("Export")))
        {
            if (!FFileHelper::SaveStringToFile(Review,*ReviewPath,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) Errors.Add(TEXT("Failed writing review"));
        }
        else
        {
            FString Previous;
            if (!FFileHelper::LoadFileToString(Previous,*ReviewPath) || Previous.Replace(TEXT("\r\n"), TEXT("\n")) != Review) Errors.Add(TEXT("Asset review stale. Regenerate with -Export and review the diff."));
        }
    }
    for (const auto& Error : Errors) UE_LOG(LogTemp, Error, TEXT("FPS_WEAPON_DATA %s"),*Error);
    UE_LOG(LogTemp, Display, TEXT("FPS_WEAPON_DATA %s definitions=%d"),Errors.IsEmpty() ? TEXT("PASS") : TEXT("FAIL"),Items.Num());
    return Errors.IsEmpty() ? 0 : 1;
#else
    return 1;
#endif
}
