#include "FPSAttachmentData.h"
#include "FPSContentHash.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "Sound/SoundBase.h"
#include "Serialization/MemoryWriter.h"
#include "Internationalization/Text.h"
#include "Misc/DataValidation.h"
#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
#endif

const TCHAR* FPSAttachments::Label(EFPSAttachmentCategory Category)
{
    const TCHAR* Labels[] = {TEXT("瞄具"), TEXT("枪口"), TEXT("弹匣"), TEXT("后握把"), TEXT("枪托"), TEXT("枪管")};
    return IsCategory(Category) ? Labels[uint8(Category)] : TEXT("未知部位");
}
FName FPSAttachments::Mount(EFPSAttachmentCategory Category)
{
    const TCHAR* Names[] = {TEXT("Mount.Optic"), TEXT("Mount.Muzzle"), TEXT("Mount.Magazine"), TEXT("Mount.RearGrip"), TEXT("Mount.Stock"), TEXT("Mount.Barrel")};
    return IsCategory(Category) ? FName(Names[uint8(Category)]) : NAME_None;
}

namespace
{
bool StableName(FName Name, const TCHAR* Prefix)
{
    const FString Value = Name.ToString();
    return Value.StartsWith(Prefix) && Value.Len() > FCString::Strlen(Prefix) && !Value.Contains(TEXT(" ")) && !Value.Contains(TEXT(":"));
}
bool Saved(const UObject* Object)
{
    return Object && Object->IsAsset() && !Object->HasAnyFlags(RF_Transient) && Object->GetOutermost() != GetTransientPackage();
}
}

void UFPSAttachmentDefinition::ValidateDefinition(TArray<FString>& Errors) const
{
    auto Require = [&](bool Valid, const TCHAR* Reason) { if (!Valid) Errors.Add(GetPathName() + TEXT(": ") + Reason); };
    Require(StableName(AttachmentId, TEXT("Attachment.")), TEXT("AttachmentId must be Attachment.<stable name>"));
    Require(!DisplayName.IsEmpty(), TEXT("DisplayName required"));
    Require(StableName(MountId, TEXT("Mount.")), TEXT("MountId must be Mount.<stable name>"));
    Require(FPSAttachments::IsCategory(Category), TEXT("Invalid attachment category"));
    Require(bUniversal ? Platforms.IsEmpty() : !Platforms.IsEmpty(), TEXT("Universal parts must have no platform whitelist; platform parts require a whitelist"));
    TSet<FName> Seen;
    for (FName Platform : Platforms)
    {
        Require(StableName(Platform, TEXT("Platform.")) && !Seen.Contains(Platform), TEXT("Invalid or duplicate platform"));
        Seen.Add(Platform);
    }
    Require(FMath::IsFinite(ReloadTimeMultiplier) && ReloadTimeMultiplier > 0, TEXT("Reload multiplier must be finite and positive"));
    Require(FMath::IsFinite(AimTimeMultiplier) && AimTimeMultiplier > 0, TEXT("ADS multiplier must be finite and positive"));
    Require(FMath::IsFinite(VerticalRecoilMultiplier) && VerticalRecoilMultiplier >= 0 && FMath::IsFinite(HorizontalRecoilMultiplier) && HorizontalRecoilMultiplier >= 0, TEXT("Recoil multipliers must be finite and nonnegative"));
    Require(FMath::IsFinite(AimFOV) && AimFOV > 0 && AimFOV < 180 && !AimLocation.ContainsNaN(), TEXT("Invalid optic view parameters"));
    Require(Category == EFPSAttachmentCategory::Magazine || (CapacityDelta == 0 && ReloadTimeMultiplier == 1), TEXT("Only magazines can modify capacity/reload time"));
    const bool Handling = Category == EFPSAttachmentCategory::RearGrip || Category == EFPSAttachmentCategory::Stock || Category == EFPSAttachmentCategory::Barrel;
    Require(Category == EFPSAttachmentCategory::Muzzle || Handling || (VerticalRecoilMultiplier == 1 && HorizontalRecoilMultiplier == 1), TEXT("Recoil modifiers require muzzle, rear grip, stock or barrel"));
    Require(Category == EFPSAttachmentCategory::Muzzle || FireSound.IsNull(), TEXT("Only muzzles can override fire sound"));
    Require(Category == EFPSAttachmentCategory::Optic || !bOverrideAim, TEXT("Only optics can override aim view"));
    Require(Category == EFPSAttachmentCategory::Optic || Handling || AimTimeMultiplier == 1, TEXT("ADS modifiers require optic, rear grip, stock or barrel"));
    Require(FMath::IsFinite(RangeMultiplier) && RangeMultiplier > 0 && (Category == EFPSAttachmentCategory::Barrel || RangeMultiplier == 1), TEXT("Positive finite range modifier requires barrel"));
    for (const auto* View : {&FirstPerson, &ThirdPerson, &Pickup})
    {
        Require(bUseWeaponPresentation ? View->Mesh.IsNull() : Saved(View->Mesh.Get()), TEXT("Explicit base presentation requires empty meshes; custom presentation requires three saved meshes"));
        const FVector Scale = View->Transform.GetScale3D();
        Require(View->Transform.IsValid() && Scale.X > 0 && Scale.Y > 0 && Scale.Z > 0, TEXT("Invalid attachment transform"));
    }
    Require(Workbench.Mesh.IsNull() || Saved(Workbench.Mesh.Get()), TEXT("Invalid optional workbench mesh"));
    const FVector WorkbenchScale = Workbench.Transform.GetScale3D();
    Require(Workbench.Transform.IsValid() && WorkbenchScale.X > 0 && WorkbenchScale.Y > 0 && WorkbenchScale.Z > 0, TEXT("Invalid workbench transform"));
    Require(FireSound.IsNull() || Saved(FireSound.Get()), TEXT("Invalid optional fire sound"));
}

TArray<FSoftObjectPath> UFPSAttachmentDefinition::Resources() const
{
    TArray<FSoftObjectPath> Paths;
    for (const auto& Path : {FirstPerson.Mesh.ToSoftObjectPath(), ThirdPerson.Mesh.ToSoftObjectPath(), Pickup.Mesh.ToSoftObjectPath(), Workbench.Mesh.ToSoftObjectPath(), FireSound.ToSoftObjectPath()})
        if (!Path.IsNull()) Paths.AddUnique(Path);
    return Paths;
}

FString UFPSAttachmentDefinition::ContentDigest(TArray<FString>& Errors) const
{
    TArray<uint8> Bytes;
    FMemoryWriter Writer(Bytes);
    auto Add = [&](auto Value) { Writer << Value; };
    Add(UFPSWeaponCatalog::ManifestVersion); Add(AttachmentId.ToString().ToLower());
    const FString* Source = FTextInspector::GetSourceString(DisplayName);
    Add(Source ? *Source : FString()); Add(uint8(Category)); Add(MountId.ToString().ToLower()); Add(bUniversal);
    TArray<FName> Sorted = Platforms; Sorted.Sort(FNameLexicalLess()); Add(Sorted.Num());
    for (FName Platform : Sorted) Add(Platform.ToString().ToLower());
    Add(CapacityDelta); Add(ReloadTimeMultiplier); Add(VerticalRecoilMultiplier); Add(HorizontalRecoilMultiplier);
    Add(bOverrideAim); Add(AimFOV); Add(AimLocation); Add(AimTimeMultiplier); Add(RangeMultiplier); Add(bUseWeaponPresentation);
    auto Resource = [&](const FSoftObjectPath& Path)
    {
        Add(!Path.IsNull());
        if (Path.IsNull()) return;
        const FString* Version = SavedResourceVersions.Find(Path);
        if (!Version || Version->IsEmpty()) { Errors.Add(GetPathName() + TEXT(": missing resource revision; save attachment")); return; }
#if WITH_EDITOR
        if (*Version != FPSContent::ResourceRevision(Path, Errors)) Errors.Add(GetPathName() + TEXT(": stale resource revision; resave attachment"));
#endif
        Add(*Version);
    };
    for (const auto* View : {&FirstPerson, &ThirdPerson, &Pickup, &Workbench}) { Add(View->Transform); Resource(View->Mesh.ToSoftObjectPath()); }
    Resource(FireSound.ToSoftObjectPath());
    return FPSContent::Hash(Bytes);
}

void UFPSAttachmentDefinition::Discover(TArray<UFPSAttachmentDefinition*>& Out, TArray<FString>& Errors)
{
    auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
    TArray<FAssetData> Assets;
    Registry.GetAssetsByClass(StaticClass()->GetClassPathName(), Assets, true);
    for (const FAssetData& Asset : Assets)
        if (Asset.PackageName.ToString().StartsWith(TEXT("/Game/")))
        {
            if (auto* Def = Cast<UFPSAttachmentDefinition>(Asset.GetAsset()))
            {
                for (const auto& Path : Def->Resources()) Path.TryLoad();
                Out.Add(Def);
            }
            else Errors.Add(Asset.GetObjectPathString() + TEXT(": failed to load attachment"));
        }
    Out.Sort([](const auto& A, const auto& B) { return A.AttachmentId.LexicalLess(B.AttachmentId); });
}

void UFPSAttachmentDefinition::ValidateCatalog(const TArray<UFPSAttachmentDefinition*>& Items, TArray<FString>& Errors)
{
    TSet<FName> Seen;
    for (const auto* Def : Items)
    {
        Def->ValidateDefinition(Errors); Def->ContentDigest(Errors);
        if (Seen.Contains(Def->AttachmentId)) Errors.Add(TEXT("Duplicate AttachmentId: ") + Def->AttachmentId.ToString());
        Seen.Add(Def->AttachmentId);
        if (UAssetManager::Get().GetPrimaryAssetPath(Def->GetPrimaryAssetId()) != FSoftObjectPath(Def))
            Errors.Add(Def->GetPathName() + TEXT(": Asset Manager identity/path mismatch"));
    }
}

bool FFPSAttachmentResolver::ValidateOffers(const UFPSWeaponDefinition& Weapon, TArray<FString>& Errors)
{
    const int32 Before = Errors.Num();
    FFPSResolvedWeaponStats Stats;
    TArray<const UFPSAttachmentDefinition*> Defaults;
    for (const auto& Ref : Weapon.AvailableAttachments) Resolve(Weapon, {Ref.Get()}, Stats, Errors);
    for (FName Id : Weapon.DefaultAttachments)
    {
        const auto* Ref = Weapon.AvailableAttachments.FindByPredicate([&](const auto& Offer) { return Offer.Get() && Offer->AttachmentId == Id; });
        Defaults.Add(Ref ? Ref->Get() : nullptr);
    }
    Resolve(Weapon, Defaults, Stats, Errors);
    return Errors.Num() == Before;
}

bool FFPSAttachmentResolver::Resolve(const UFPSWeaponDefinition& Weapon, const TArray<const UFPSAttachmentDefinition*>& Attachments,
    FFPSResolvedWeaponStats& Out, TArray<FString>& Errors)
{
    const int32 Before = Errors.Num();
    FFPSResolvedWeaponStats Result;
    Result.MagazineSize = Weapon.MagazineSize; Result.ReloadDuration = Weapon.ReloadDuration;
    Result.AimTransitionTime = Weapon.AimTransitionTime; Result.AimFOV = Weapon.AimFOV; Result.AimLocation = Weapon.AimLocation;
    Result.FireSound = Weapon.FireSound; Result.Range = Weapon.Range;
    TArray<const UFPSAttachmentDefinition*> Sorted;
    for (const auto* Def : Attachments)
        if (Def) Sorted.Add(Def); else Errors.Add(TEXT("Null attachment definition"));
    Sorted.Sort([](const auto& A, const auto& B) { return A.AttachmentId.LexicalLess(B.AttachmentId); });
    TSet<FName> Ids, Mounts;
    TSet<EFPSAttachmentCategory> Categories;
    int64 Capacity = Weapon.MagazineSize;
    for (const auto* Def : Sorted)
    {
        Def->ValidateDefinition(Errors);
        auto Fail = [&](const TCHAR* Message) { Errors.Add(Weapon.WeaponId.ToString() + TEXT(" / ") + Def->AttachmentId.ToString() + TEXT(": ") + Message); };
        if (!Weapon.AvailableAttachments.ContainsByPredicate([&](const auto& Ref) { return Ref.ToSoftObjectPath() == FSoftObjectPath(Def); })) Fail(TEXT("Attachment is not in weapon's explicit offer set"));
        if (Ids.Contains(Def->AttachmentId)) Fail(TEXT("Duplicate attachment"));
        if (Mounts.Contains(Def->MountId) || Categories.Contains(Def->Category)) Fail(TEXT("Conflicting mount/category"));
        if (!Weapon.MountIds.Contains(Def->MountId)) Fail(TEXT("Missing mounting interface"));
        if (!Def->bUniversal && !Def->Platforms.Contains(Weapon.PlatformId)) Fail(TEXT("Incompatible platform"));
        Ids.Add(Def->AttachmentId); Mounts.Add(Def->MountId); Categories.Add(Def->Category);
        Capacity += Def->CapacityDelta;
        Result.ReloadDuration *= Def->ReloadTimeMultiplier; Result.AimTransitionTime *= Def->AimTimeMultiplier;
        Result.VerticalRecoilMultiplier *= Def->VerticalRecoilMultiplier; Result.HorizontalRecoilMultiplier *= Def->HorizontalRecoilMultiplier;
        Result.Range *= Def->RangeMultiplier;
        if (Def->bOverrideAim) { Result.AimFOV = Def->AimFOV; Result.AimLocation = Def->AimLocation; }
        if (!Def->FireSound.IsNull()) Result.FireSound = Def->FireSound;
    }
    if (Capacity <= 0 || Capacity > MAX_int32) Errors.Add(TEXT("Resolved capacity outside int32 positive range"));
    if (!FMath::IsFinite(Result.Range) || Result.Range <= 0 || !FMath::IsFinite(Result.ReloadDuration) || Result.ReloadDuration <= 0 || !FMath::IsFinite(Result.AimTransitionTime) || Result.AimTransitionTime < 0 ||
        !FMath::IsFinite(Result.VerticalRecoilMultiplier) || !FMath::IsFinite(Result.HorizontalRecoilMultiplier) ||
        !FMath::IsFinite(Result.AimFOV) || Result.AimFOV <= 0 || Result.AimFOV >= 180 || Result.AimLocation.ContainsNaN()) Errors.Add(TEXT("Invalid resolved weapon parameters"));
    if (Errors.Num() != Before) return false;
    Result.MagazineSize = int32(Capacity);
    Out = Result;
    return true;
}

const UFPSAttachmentDefinition* UFPSWeaponCatalog::FindAttachment(FName AttachmentId) const
{
    const auto* Found = Attachments.Find(AttachmentId);
    return Found ? Found->Get() : nullptr;
}

bool UFPSWeaponCatalog::Resolve(FName WeaponId, const TArray<FName>& Ids, FFPSResolvedWeaponStats& Out, TArray<FString>& Errors) const
{
    const auto* Weapon = Find(WeaponId);
    if (!Weapon) { Errors.Add(TEXT("Weapon not admitted: ") + WeaponId.ToString()); return false; }
    TArray<const UFPSAttachmentDefinition*> Parts;
    for (FName Id : Ids)
    {
        const auto* Part = FindAttachment(Id);
        if (!Part) { Errors.Add(TEXT("Attachment not admitted: ") + Id.ToString()); return false; }
        Parts.Add(Part);
    }
    return FFPSAttachmentResolver::Resolve(*Weapon, Parts, Out, Errors);
}

#if WITH_EDITOR
void UFPSAttachmentDefinition::PreSave(FObjectPreSaveContext SaveContext)
{
    if (!SaveContext.IsCooking())
    {
        SavedResourceVersions.Empty();
        TArray<FString> Errors;
        for (const auto& Path : Resources()) SavedResourceVersions.Add(Path, FPSContent::ResourceRevision(Path, Errors));
        for (const auto& Error : Errors) UE_LOG(LogTemp, Error, TEXT("FPS_ATTACHMENT %s"), *Error);
    }
    Super::PreSave(SaveContext);
}
EDataValidationResult UFPSAttachmentDefinition::IsDataValid(FDataValidationContext& Context) const
{
    TArray<FString> Errors;
    for (const auto& Path : Resources()) Path.TryLoad();
    ValidateDefinition(Errors); ContentDigest(Errors);
    auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    TArray<FAssetData> Metadata;
    Registry.GetAssetsByClass(StaticClass()->GetClassPathName(), Metadata, true);
    for (const auto& Asset : Metadata)
    {
        FName Id; Asset.GetTagValue(TEXT("AttachmentId"), Id);
        if (Id == AttachmentId && Asset.GetSoftObjectPath() != FSoftObjectPath(this)) Errors.Add(TEXT("Duplicate AttachmentId: ") + Asset.GetObjectPathString());
    }
    for (const auto& Error : Errors) Context.AddError(FText::FromString(Error));
    return Errors.IsEmpty() ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
