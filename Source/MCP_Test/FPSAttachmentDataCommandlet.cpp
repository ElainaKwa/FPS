#include "FPSAttachmentDataCommandlet.h"
#include "FPSAttachmentData.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include <limits>
#endif

UFPSAttachmentDataCommandlet::UFPSAttachmentDataCommandlet()
{
    IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}

#if WITH_EDITOR
namespace
{
bool Save(UObject* Asset)
{
    Asset->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(Asset->GetOutermost(), Asset,
        *FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}

// One-time authoring only. Existing assets are never retuned from these initial values.
void CreateM4(TArray<FString>& Errors)
{
    auto* Weapon = LoadObject<UFPSWeaponDefinition>(nullptr, TEXT("/Game/FPSDemo/Weapons/Definitions/DA_M4.DA_M4"));
    if (!Weapon || Weapon->WeaponId != TEXT("Weapon.M4")) { Errors.Add(TEXT("M4 definition missing or identity changed")); return; }
    struct FTemplate { const TCHAR* Name; EFPSAttachmentCategory Category; bool Upgrade; };
    const FTemplate Rows[] = {
        {TEXT("M4_OpticStandard"), EFPSAttachmentCategory::Optic, false}, {TEXT("M4_OpticFixed"), EFPSAttachmentCategory::Optic, true},
        {TEXT("M4_MuzzleStandard"), EFPSAttachmentCategory::Muzzle, false}, {TEXT("M4_MuzzleBrake"), EFPSAttachmentCategory::Muzzle, true},
        {TEXT("M4_MagazineStandard"), EFPSAttachmentCategory::Magazine, false}, {TEXT("M4_MagazineExtended"), EFPSAttachmentCategory::Magazine, true}};
    TArray<TSoftObjectPtr<UFPSAttachmentDefinition>> Offers;
    for (const auto& Row : Rows)
    {
        const FString Path = FString(TEXT("/Game/FPSDemo/Weapons/Attachments/DA_")) + Row.Name;
        const FName Id(*(FString(TEXT("Attachment.")) + Row.Name));
        UFPSAttachmentDefinition* Part = nullptr;
        if (FPackageName::DoesPackageExist(Path))
        {
            Part = LoadObject<UFPSAttachmentDefinition>(nullptr, *(Path + TEXT(".") + FPackageName::GetLongPackageAssetName(Path)));
            if (!Part || Part->AttachmentId != Id) { Errors.Add(TEXT("Existing attachment identity conflict: ") + Path); return; }
        }
        else
        {
            Part = NewObject<UFPSAttachmentDefinition>(CreatePackage(*Path), *FPackageName::GetLongPackageAssetName(Path), RF_Public | RF_Standalone);
            Part->AttachmentId = Id; Part->DisplayName = FText::FromString(Row.Name);
            Part->Category = Row.Category; Part->Platforms = {TEXT("Platform.M4")};
            Part->MountId = Row.Category == EFPSAttachmentCategory::Optic ? TEXT("Mount.Optic") : Row.Category == EFPSAttachmentCategory::Muzzle ? TEXT("Mount.Muzzle") : TEXT("Mount.Magazine");
            if (Row.Upgrade)
            {
                if (Row.Category == EFPSAttachmentCategory::Optic)
                {
                    Part->bOverrideAim = true; Part->AimFOV = 48; Part->AimLocation = Weapon->AimLocation; Part->AimTimeMultiplier = 1.15f;
                }
                else if (Row.Category == EFPSAttachmentCategory::Muzzle)
                {
                    Part->VerticalRecoilMultiplier = .8f; Part->HorizontalRecoilMultiplier = .85f;
                }
                else { Part->CapacityDelta = 15; Part->ReloadTimeMultiplier = 1.15f; }
            }
            Part->ValidateDefinition(Errors);
            if (!Errors.IsEmpty()) return;
            FAssetRegistryModule::AssetCreated(Part);
            if (!Save(Part)) { Errors.Add(TEXT("Failed saving: ") + Path); return; }
        }
        Offers.Add(Part);
    }
    // A preexisting offer set is designer-owned. Re-running this command cannot reset it.
    if (Weapon->PlatformId.IsNone() && Weapon->MountIds.IsEmpty() && Weapon->AvailableAttachments.IsEmpty() && Weapon->DefaultAttachments.IsEmpty())
    {
        Weapon->PlatformId = TEXT("Platform.M4");
        Weapon->MountIds = {TEXT("Mount.Optic"), TEXT("Mount.Muzzle"), TEXT("Mount.Magazine")};
        Weapon->AvailableAttachments = Offers;
        if (!Save(Weapon)) Errors.Add(TEXT("Failed saving M4 attachment interfaces"));
    }
}

bool SelfTest()
{
    bool Passed = true;
    auto Check = [&](bool Value, const TCHAR* What) { Passed &= Value; UE_LOG(LogTemp, Display, TEXT("FPS_ATTACHMENT_TEST %s %s"), Value ? TEXT("PASS") : TEXT("FAIL"), What); };
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UFPSWeaponCatalog> Catalog(NewObject<UFPSWeaponCatalog>(GI.Get()));
    Check(Catalog->PrepareDependencies({TEXT("Weapon.M4"), TEXT("Weapon.M9")}), TEXT("explicit M4/M9 session loads"));
    const auto* Weapon = Catalog->Find(TEXT("Weapon.M4"));
    auto* Magazine = const_cast<UFPSAttachmentDefinition*>(Catalog->FindAttachment(TEXT("Attachment.M4_MagazineExtended")));
    auto* Optic = const_cast<UFPSAttachmentDefinition*>(Catalog->FindAttachment(TEXT("Attachment.M4_OpticFixed")));
    const auto* Muzzle = Catalog->FindAttachment(TEXT("Attachment.M4_MuzzleBrake"));
    if (!Weapon || !Magazine || !Optic || !Muzzle) { Check(false, TEXT("M4 fixtures available")); return false; }
    TArray<FString> Errors;
    FFPSResolvedWeaponStats Stats;
    Check(Catalog->Resolve(Weapon->WeaponId, {}, Stats, Errors) && Stats.MagazineSize == Weapon->MagazineSize && Stats.ReloadDuration == Weapon->ReloadDuration && Stats.AimFOV == Weapon->AimFOV, TEXT("empty assembly preserves base values"));
    Check(FFPSAttachmentResolver::Resolve(*Weapon, {Magazine, Optic, Muzzle}, Stats, Errors) && Stats.MagazineSize == Weapon->MagazineSize + Magazine->CapacityDelta &&
        FMath::IsNearlyEqual(Stats.ReloadDuration, Weapon->ReloadDuration * Magazine->ReloadTimeMultiplier) && Stats.AimFOV == Optic->AimFOV && Stats.VerticalRecoilMultiplier == Muzzle->VerticalRecoilMultiplier, TEXT("three category values composed"));
    FFPSResolvedWeaponStats Reverse;
    Check(FFPSAttachmentResolver::Resolve(*Weapon, {Muzzle, Optic, Magazine}, Reverse, Errors) && Reverse.MagazineSize == Stats.MagazineSize && Reverse.ReloadDuration == Stats.ReloadDuration && Reverse.AimFOV == Stats.AimFOV, TEXT("install order independent"));
    auto Invalid = [&](TArray<const UFPSAttachmentDefinition*> Parts)
    {
        TArray<FString> Failure;
        FFPSResolvedWeaponStats Unchanged; Unchanged.MagazineSize = 123;
        return !FFPSAttachmentResolver::Resolve(*Weapon, Parts, Unchanged, Failure) && !Failure.IsEmpty() && Unchanged.MagazineSize == 123;
    };
    Check(Invalid({Magazine, Magazine}), TEXT("duplicate rejected atomically"));
    Check(Invalid({nullptr}), TEXT("missing attachment rejected"));
    const auto Platforms = Magazine->Platforms;
    Magazine->Platforms = {TEXT("Platform.Other")}; Check(Invalid({Magazine}), TEXT("wrong platform rejected"));
    Magazine->bUniversal = true; Magazine->Platforms.Empty();
    Check(!Invalid({Magazine}), TEXT("explicit universal permits compatible mount"));
    const FName Mount = Magazine->MountId;
    Magazine->MountId = TEXT("Mount.Unknown"); Check(Invalid({Magazine}), TEXT("universal still requires interface"));
    Magazine->MountId = Mount; Magazine->bUniversal = false; Magazine->Platforms = Platforms;
    const int32 Delta = Magazine->CapacityDelta;
    Magazine->CapacityDelta = -Weapon->MagazineSize; Check(Invalid({Magazine}), TEXT("zero capacity rejected"));
    Magazine->CapacityDelta = MAX_int32; Check(Invalid({Magazine}), TEXT("capacity overflow rejected")); Magazine->CapacityDelta = Delta;
    const float Reload = Magazine->ReloadTimeMultiplier;
    Magazine->ReloadTimeMultiplier = std::numeric_limits<float>::quiet_NaN(); Check(Invalid({Magazine}), TEXT("NaN rejected"));
    Magazine->ReloadTimeMultiplier = Reload;
    const float FOV = Optic->AimFOV; Optic->AimFOV = 180; Check(Invalid({Optic}), TEXT("invalid FOV rejected")); Optic->AimFOV = FOV;
    const auto Identity = Magazine->GetPrimaryAssetId();
    const FString Digest = Magazine->ContentDigest(Errors);
    ++Magazine->CapacityDelta; Check(Magazine->ContentDigest(Errors) != Digest, TEXT("attachment tuning changes digest")); Magazine->CapacityDelta = Delta;
    auto* Copy = DuplicateObject<UFPSAttachmentDefinition>(Magazine, GetTransientPackage(), TEXT("MovedAttachment"));
    Check(Copy->GetPrimaryAssetId() == Identity && Copy->ContentDigest(Errors) == Digest, TEXT("definition rename preserves identity and digest"));
    TArray<FString> DuplicateErrors;
    UFPSAttachmentDefinition::ValidateCatalog({Magazine, Copy}, DuplicateErrors);
    Check(DuplicateErrors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("Duplicate AttachmentId")); }), TEXT("duplicate identity publication failure"));
    // Mutate only an in-memory fixture; never save fault injection.
    Magazine->ReloadTimeMultiplier = -1;
    Check(Catalog->PrepareDependencies({TEXT("Weapon.M9")}), TEXT("unrelated invalid M4 part does not block M9 session"));
    TArray<FString> Publication;
    UFPSAttachmentDefinition::ValidateCatalog({Magazine}, Publication);
    Check(!Publication.IsEmpty(), TEXT("unrelated invalid part still fails publication"));
    Check(!Catalog->PrepareDependencies({TEXT("Weapon.M4")}) && !Catalog->Find(TEXT("Weapon.M4")) && !Catalog->FindAttachment(Magazine->AttachmentId), TEXT("required invalid part blocks and publishes nothing"));
    Magazine->ReloadTimeMultiplier = Reload;
    Check(Catalog->PrepareDependencies({TEXT("Weapon.M4")}), TEXT("corrected session can load"));
    TWeakObjectPtr<UFPSAttachmentDefinition> WeakPart(Magazine);
    Catalog->ReleaseSession(); CollectGarbage(RF_NoFlags);
    Check(!WeakPart.IsValid() && !Catalog->IsReady(), TEXT("attachment collectible after release and GC"));
    Check(Errors.IsEmpty(), TEXT("normal resolution has no errors"));
    Check(Passed, TEXT("COMPLETE"));
    return Passed;
}
}
#endif

int32 UFPSAttachmentDataCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
    TArray<FString> Errors;
    auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
    if (FParse::Param(*Params, TEXT("CreateM4"))) CreateM4(Errors);
    UAssetManager::Get().ScanPathsForPrimaryAssets(TEXT("WeaponAttachment"), {TEXT("/Game")}, UFPSAttachmentDefinition::StaticClass(), false, false, true);
    if (Errors.IsEmpty() && FParse::Param(*Params, TEXT("SelfTest"))) return SelfTest() ? 0 : 1;
    TArray<UFPSAttachmentDefinition*> Items;
    UFPSAttachmentDefinition::Discover(Items, Errors);
    UFPSAttachmentDefinition::ValidateCatalog(Items, Errors);
    TArray<UFPSWeaponDefinition*> Weapons;
    UFPSWeaponCatalog::Discover(Weapons, Errors);
    UFPSWeaponCatalog::ValidateCatalog(Weapons, Errors);
    FString Review = TEXT("# 配件资产清单（自动生成）\n\n- 用途：自动生成资料\n- 适用范围：已保存的配件定义\n- 状态：只读导出，禁止手工编辑\n- 最后更新：2026-10-15\n\n来源：Unreal 配件资产。生成入口：`Tools/ValidateFPSAttachmentData.ps1 -Export`。数值修改应在定义资产中完成。\n");
    for (const auto* Part : Items)
    {
        Review += FString::Printf(TEXT("\n## %s\n\n资产：`%s`\n\n```text\n"), *Part->AttachmentId.ToString(), *Part->GetPathName());
        for (TFieldIterator<FProperty> It(UFPSAttachmentDefinition::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
        {
            FString Value; It->ExportText_InContainer(0, Value, Part, Part, const_cast<UFPSAttachmentDefinition*>(Part), PPF_None);
            Review += It->GetName() + TEXT("=") + Value + TEXT("\n");
        }
        Review += TEXT("```\n");
    }
    const FString Path = FPaths::ProjectDir() / TEXT("Docs/FPS/Weapons/AttachmentAssetReview.md");
    if (Errors.IsEmpty())
    {
        if (FParse::Param(*Params, TEXT("Export")))
        {
            if (!FFileHelper::SaveStringToFile(Review, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) Errors.Add(TEXT("Failed exporting attachment review"));
        }
        else
        {
            FString Existing;
            if (!FFileHelper::LoadFileToString(Existing, *Path) || Existing.Replace(TEXT("\r\n"), TEXT("\n")) != Review) Errors.Add(TEXT("Attachment review stale; regenerate with -Export"));
        }
    }
    for (const auto& Error : Errors) UE_LOG(LogTemp, Error, TEXT("FPS_ATTACHMENT_DATA %s"), *Error);
    UE_LOG(LogTemp, Display, TEXT("FPS_ATTACHMENT_DATA %s definitions=%d"), Errors.IsEmpty() ? TEXT("PASS") : TEXT("FAIL"), Items.Num());
    return Errors.IsEmpty() ? 0 : 1;
#else
    return 1;
#endif
}
