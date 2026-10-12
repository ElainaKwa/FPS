#include "FPSWeaponAuthoring.h"
#if WITH_EDITOR
#include "FPSAttachmentData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Engine/AssetManager.h"
#include "ScopedTransaction.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Internationalization/Regex.h"

namespace FPSWeaponAuthoring
{
void Load(TArray<TObjectPtr<UObject>>& Assets, TArray<FString>& Errors)
{
    Assets.Empty();
    TArray<UFPSWeaponDefinition*> Weapons; UFPSWeaponCatalog::Discover(Weapons, Errors);
    TArray<UFPSAttachmentDefinition*> Parts; UFPSAttachmentDefinition::Discover(Parts, Errors);
    for (auto* Weapon : Weapons) Assets.Add(Weapon);
    for (auto* Part : Parts) Assets.Add(Part);
}

void Validate(UObject* Asset, TArray<FString>& Errors)
{
    if (const auto* Weapon = Cast<UFPSWeaponDefinition>(Asset))
    {
        for (const auto& Path : Weapon->Resources()) Path.TryLoad();
        for (const auto& Ref : Weapon->AvailableAttachments)
            if (const auto* Part = Ref.LoadSynchronous())
                for (const auto& Path : Part->Resources()) Path.TryLoad();
        Weapon->ValidateDefinition(Errors);
        FFPSAttachmentResolver::ValidateOffers(*Weapon, Errors);
    }
    else if (const auto* Part = Cast<UFPSAttachmentDefinition>(Asset))
    {
        for (const auto& Path : Part->Resources()) Path.TryLoad();
        Part->ValidateDefinition(Errors);
    }
    else Errors.Add(TEXT("Unsupported definition type"));
}

FString Number(const UObject* Asset, FName Field)
{
    if (!Asset) return FString();
    const auto* Property = FindFProperty<FNumericProperty>(Asset->GetClass(), Field);
    if (!Property) return FString();
    const void* Address = Property->ContainerPtrToValuePtr<void>(Asset);
    return Property->IsInteger() ? FString::Printf(TEXT("%lld"), Property->GetSignedIntPropertyValue(Address)) : FString::Printf(TEXT("%.7g"), Property->GetFloatingPointPropertyValue(Address));
}

bool SetNumber(const TArray<UObject*>& Assets, FName Field, const FString& Text, FString& Failure)
{
    if (Assets.IsEmpty()) { Failure = TEXT("请选择要修改的资产。"); return false; }
    const FString Input = Text.TrimStartAndEnd();
    TSet<UObject*> Unique;
    // Test the entire batch before changing any original. Invalid input never leaves partial edits.
    for (auto* Asset : Assets)
    {
        if (!Asset || Unique.Contains(Asset)) { Failure = TEXT("无效或重复资产。"); return false; }
        Unique.Add(Asset);
        const auto* Property = FindFProperty<FNumericProperty>(Asset->GetClass(), Field);
        if (!Property || !Property->HasAnyPropertyFlags(CPF_Edit) || Property->IsEnum()) { Failure = TEXT("此列不是可编辑数值。"); return false; }
        const FRegexPattern Pattern(Property->IsInteger() ? TEXT("^[+-]?[0-9]+$") : TEXT("^[+-]?([0-9]+(\\.[0-9]*)?|\\.[0-9]+)([eE][+-]?[0-9]+)?$"));
        FRegexMatcher Matcher(Pattern, Input);
        if (!Matcher.FindNext()) { Failure = TEXT("数值格式无效，整数列不能输入小数。"); return false; }
        TStrongObjectPtr<UObject> Candidate(DuplicateObject(Asset, GetTransientPackage()));
        void* Address = Property->ContainerPtrToValuePtr<void>(Candidate.Get());
        double FloatValue = 0; int64 IntValue = 0;
        if (Property->IsInteger())
        {
            if (!LexTryParseString(IntValue, *Input) || IntValue < MIN_int32 || IntValue > MAX_int32) { Failure = TEXT("请输入有效的 int32 整数。"); return false; }
            Property->SetIntPropertyValue(Address, IntValue);
        }
        else
        {
            if (!LexTryParseString(FloatValue, *Input) || !FMath::IsFinite(FloatValue)) { Failure = TEXT("请输入有限数值。"); return false; }
            Property->SetFloatingPointPropertyValue(Address, FloatValue);
        }
        TArray<FString> Errors;
        Validate(Candidate.Get(), Errors);
        if (!Errors.IsEmpty()) { Failure = Asset->GetPathName() + TEXT("\n") + FString::Join(Errors, TEXT("\n")); return false; }
    }
    const FScopedTransaction Transaction(FText::FromString(TEXT("修改枪械／配件数值")));
    for (auto* Asset : Assets)
    {
        auto* Property = FindFProperty<FNumericProperty>(Asset->GetClass(), Field);
        Asset->SetFlags(RF_Transactional); Asset->Modify(); Asset->PreEditChange(Property);
        void* Address = Property->ContainerPtrToValuePtr<void>(Asset);
        if (Property->IsInteger()) { int64 Value = 0; LexTryParseString(Value, *Input); Property->SetIntPropertyValue(Address, Value); }
        else { double Value = 0; LexTryParseString(Value, *Input); Property->SetFloatingPointPropertyValue(Address, Value); }
        FPropertyChangedEvent Event(Property, EPropertyChangeType::ValueSet); Asset->PostEditChangeProperty(Event);
        Asset->MarkPackageDirty();
    }
    Failure.Empty();
    return true;
}

bool Save(const TArray<UObject*>& Assets, FString& Failure)
{
    if (Assets.IsEmpty()) { Failure = TEXT("请选择要保存的资产。"); return false; }
    TArray<FString> Errors;
    TSet<FPrimaryAssetId> Ids;
    auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    for (auto* Asset : Assets)
    {
        Validate(Asset, Errors);
        const auto* Definition = Cast<UPrimaryDataAsset>(Asset);
        if (!Definition) continue;
        const auto Identity = Definition->GetPrimaryAssetId();
        if (Ids.Contains(Identity)) Errors.Add(TEXT("Duplicate identity: ") + Identity.ToString());
        Ids.Add(Identity);
        TArray<FAssetData> Metadata; Registry.GetAssetsByClass(Asset->GetClass()->GetClassPathName(), Metadata, true);
        const FName Tag = Cast<UFPSWeaponDefinition>(Asset) ? FName(TEXT("WeaponId")) : FName(TEXT("AttachmentId"));
        for (const auto& Other : Metadata)
        {
            FName OtherId; Other.GetTagValue(Tag, OtherId);
            if (OtherId == Identity.PrimaryAssetName && Other.GetSoftObjectPath() != FSoftObjectPath(Asset)) Errors.Add(TEXT("Duplicate identity: ") + Other.GetObjectPathString());
        }
    }
    if (!Errors.IsEmpty()) { Failure = FString::Join(Errors, TEXT("\n")); return false; }
    TArray<FString> Saved;
    for (auto* Asset : Assets)
    {
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
        const FString Filename = FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
        if (!UPackage::SavePackage(Asset->GetOutermost(), Asset, *Filename, Args))
        {
            Failure = TEXT("保存失败：") + Asset->GetPathName() + TEXT("\n此前已保存：\n") + FString::Join(Saved, TEXT("\n"));
            return false;
        }
        Saved.Add(Asset->GetPathName());
    }
    auto& Manager = UAssetManager::Get();
    Manager.ScanPathsForPrimaryAssets(TEXT("WeaponDefinition"), {TEXT("/Game")}, UFPSWeaponDefinition::StaticClass(), false, false, true);
    Manager.ScanPathsForPrimaryAssets(TEXT("WeaponAttachment"), {TEXT("/Game")}, UFPSAttachmentDefinition::StaticClass(), false, false, true);
    Failure.Empty();
    return true;
}

UObject* CopyTemplate(UObject* Source, FString& Failure)
{
    if (!Cast<UFPSWeaponDefinition>(Source) && !Cast<UFPSAttachmentDefinition>(Source)) { Failure = TEXT("请选择一个定义作为模板。"); return nullptr; }
    FString PackageName, AssetName;
    FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().CreateUniqueAssetName(Source->GetOutermost()->GetName(), TEXT("_Copy"), PackageName, AssetName);
    UObject* Copy = DuplicateObject(Source, CreatePackage(*PackageName), *AssetName);
    Copy->SetFlags(RF_Public | RF_Standalone | RF_Transactional);
    const FString Suffix = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    if (auto* Weapon = Cast<UFPSWeaponDefinition>(Copy)) Weapon->WeaponId = FName(*(TEXT("Weapon.Copy.") + Suffix));
    if (auto* Part = Cast<UFPSAttachmentDefinition>(Copy)) Part->AttachmentId = FName(*(TEXT("Attachment.Copy.") + Suffix));
    FAssetRegistryModule::AssetCreated(Copy);
    Copy->MarkPackageDirty(); Failure.Empty();
    return Copy;
}
}
#endif
