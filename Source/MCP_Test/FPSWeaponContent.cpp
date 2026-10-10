#include "FPSWeaponData.h"
#include "FPSModeRules.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/SecureHash.h"
#include "Serialization/MemoryWriter.h"
#include "Internationalization/Text.h"
#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
#endif

namespace
{
FString Hash(const TArray<uint8>& Bytes)
{
    uint8 Digest[FSHA1::DigestSize];
    FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Digest);
    return BytesToHex(Digest, UE_ARRAY_COUNT(Digest));
}
#if WITH_EDITOR
FString ResourceRevision(const FSoftObjectPath& Path, TArray<FString>& Errors)
{
    auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
        TArray<FName> Pending{Path.GetLongPackageFName()};
        TSet<FName> Seen;
        TArray<FString> Revisions;
        while (!Pending.IsEmpty())
        {
            const FName Name = Pending.Pop();
            if (Seen.Contains(Name) || Name.ToString().StartsWith(TEXT("/Script/"))) continue;
            Seen.Add(Name);
            const auto Package = Registry.GetAssetPackageDataCopy(Name);
            if (!Package.IsSet() || Package->GetPackageSavedHash().IsZero())
            {
                Errors.Add(TEXT("Missing saved resource revision: ") + Name.ToString());
                continue;
            }
            Revisions.Add(LexToString(Package->GetPackageSavedHash()));
            TArray<FName> Dependencies;
            Registry.GetDependencies(Name, Dependencies);
            Pending.Append(Dependencies);
        }
        Revisions.Sort();
        return FString::Join(Revisions, TEXT(";"));
}
#endif

}

TArray<FSoftObjectPath> UFPSWeaponDefinition::Resources() const
{
    TArray<FSoftObjectPath> Paths;
    for (const auto& Path : {FirstPerson.Mesh.ToSoftObjectPath(), ThirdPerson.Mesh.ToSoftObjectPath(), Pickup.Mesh.ToSoftObjectPath(),
        RecoilCurve.ToSoftObjectPath(), FireSound.ToSoftObjectPath(), ReloadStartSound.ToSoftObjectPath(), ReloadEndSound.ToSoftObjectPath(),
        EquipSound.ToSoftObjectPath(), InspectSound.ToSoftObjectPath(), EmptySound.ToSoftObjectPath()})
        if (!Path.IsNull()) Paths.AddUnique(Path);
    return Paths;
}

FString UFPSWeaponDefinition::ContentDigest(TArray<FString>& Errors) const
{
    // Format 1: ordered typed binary fields; no definition package path, timestamps or localized display string.
    TArray<uint8> Bytes;
    FMemoryWriter Writer(Bytes);
    auto Add = [&](auto Value) { Writer << Value; };
    Add(UFPSWeaponCatalog::ManifestVersion);
    Add(WeaponId.ToString().ToLower());
    const FString* SourceText = FTextInspector::GetSourceString(DisplayName);
    Add(SourceText ? *SourceText : FString());
    Add(uint8(Slot)); Add(uint8(Category)); Add(MagazineSize); Add(InitialMagazine); Add(InitialReserve);
    Add(Damage); Add(FireInterval); Add(Range); Add(uint8(FireMode)); Add(BurstCount); Add(BurstInterval);
    Add(ReloadDuration); Add(InspectDuration); Add(EquipReadyTime); Add(EquipBlendTime); Add(AimTransitionTime);
    Add(bNoRecoil); Add(AimRecoilMultiplier); Add(RecoilRecoveryTime); Add(HipLocation); Add(AimLocation); Add(MuzzleLocation); Add(AimFOV);
    auto ResourceVersion = [&](const FSoftObjectPath& Path)
    {
        const FString* Saved = SavedResourceVersions.Find(Path);
        if (!Saved || Saved->IsEmpty()) { Errors.Add(TEXT("Missing generated resource revision; save definition: ") + GetPathName()); return; }
#if WITH_EDITOR
        if (*Saved != ResourceRevision(Path, Errors)) Errors.Add(TEXT("Stale resource revision; save definition after editing resources: ") + GetPathName());
#endif
        Add(*Saved);
    };
    for (const auto* View : {&FirstPerson, &ThirdPerson, &Pickup})
    {
        Add(View->Transform); ResourceVersion(View->Mesh.ToSoftObjectPath());
    }
    for (const auto& Sound : {FireSound, ReloadStartSound, ReloadEndSound, EquipSound, InspectSound, EmptySound}) ResourceVersion(Sound.ToSoftObjectPath());
    if (const auto* Curve = RecoilCurve.Get())
        for (const FRichCurve& Channel : Curve->FloatCurves)
        {
            Add(Channel.DefaultValue); Add(uint8(Channel.PreInfinityExtrap)); Add(uint8(Channel.PostInfinityExtrap)); Add(Channel.GetNumKeys());
            for (const auto& Key : Channel.GetConstRefOfKeys())
            {
                Add(Key.Time); Add(Key.Value); Add(uint8(Key.InterpMode)); Add(uint8(Key.TangentMode));
                Add(Key.ArriveTangent); Add(Key.LeaveTangent); Add(uint8(Key.TangentWeightMode)); Add(Key.ArriveTangentWeight); Add(Key.LeaveTangentWeight);
            }
        }
    return Hash(Bytes);
}

#if WITH_EDITOR
void UFPSWeaponDefinition::PreSave(FObjectPreSaveContext SaveContext)
{
    if (!SaveContext.IsCooking())
    {
        SavedResourceVersions.Empty();
        TArray<FString> Errors;
        for (const auto& Path : {FirstPerson.Mesh.ToSoftObjectPath(), ThirdPerson.Mesh.ToSoftObjectPath(), Pickup.Mesh.ToSoftObjectPath(),
            FireSound.ToSoftObjectPath(), ReloadStartSound.ToSoftObjectPath(), ReloadEndSound.ToSoftObjectPath(), EquipSound.ToSoftObjectPath(), InspectSound.ToSoftObjectPath(), EmptySound.ToSoftObjectPath()})
            SavedResourceVersions.Add(Path, ResourceRevision(Path, Errors));
        for (const auto& Error : Errors) UE_LOG(LogTemp, Error, TEXT("FPS_WEAPON_CONTENT %s"), *Error);
    }
    Super::PreSave(SaveContext);
}
#endif

void UFPSWeaponCatalog::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    WorldInitHandle = FWorldDelegates::OnWorldInitializedActors.AddLambda([this](const UWorld::FActorsInitializedParams& Params)
    {
        if (Params.World->IsGameWorld() && Params.World->GetGameInstance() == GetGameInstance()) PrepareWorld(Params.World);
    });
    WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddLambda([this](UWorld* World, bool, bool)
    {
        if (SessionWorld.Get() == World) ReleaseSession();
    });
}

void UFPSWeaponCatalog::Deinitialize()
{
    FWorldDelegates::OnWorldInitializedActors.Remove(WorldInitHandle);
    FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
    ReleaseSession();
    Super::Deinitialize();
}

void UFPSWeaponCatalog::ReleaseSession()
{
    const int32 Count = Definitions.Num();
    Definitions.Empty();
    if (ResourceHandle) ResourceHandle->ReleaseHandle();
    if (DefinitionHandle) DefinitionHandle->ReleaseHandle();
    ResourceHandle.Reset(); DefinitionHandle.Reset();
    SessionIds.Empty(); SessionDigest.Empty(); ValidationError.Empty(); SessionWorld.Reset(); bReady = false;
    if (Count) UE_LOG(LogTemp, Display, TEXT("FPS_WEAPON_CONTENT RELEASE definitions=%d handles=0"), Count);
}

bool UFPSWeaponCatalog::PrepareWorld(UWorld* World, bool bRequireStands)
{
    if (SessionWorld.Get() == World) return bReady;
    TArray<FName> Ids;
    TArray<FString> Errors;
    auto& Manager = UAssetManager::Get();
    const UFPSModeRules* ModeRules = nullptr;
    int32 StandCount = 0;
    for (TActorIterator<AFPSWeaponStand> It(World); It; ++It)
    {
        ++StandCount;
        const auto Id = Manager.GetPrimaryAssetIdForPath(It->Definition.ToSoftObjectPath());
        if (!Id.IsValid() || Id.PrimaryAssetType != FPrimaryAssetType(TEXT("WeaponDefinition"))) Errors.Add(TEXT("Invalid explicit weapon stand: ") + It->GetPathName());
        else Ids.AddUnique(Id.PrimaryAssetName);
    }
    if (bRequireStands || StandCount)
    {
        ModeRules = UFPSModeRules::ForWorld(World, &Errors);
        if (ModeRules) ModeRules->GatherWeaponIds(Ids, Errors);
        if (!StandCount) Errors.Add(TEXT("Map has no explicit weapon stands"));
    }
    else if (World->GetMapName().Contains(TEXT("LV_NetSyncFPS"))) Ids.AddUnique(StartingPrimary);
    PrepareDependencies(Ids);
    if (bReady && ModeRules && Errors.IsEmpty())
    {
        // Admission covers rule identity/preferences as well as the required definition/resource set.
        // Stable weapon IDs participate, never package paths or GUIDs of spawned instances.
        TArray<uint8> Bytes;
        FMemoryWriter Writer(Bytes);
        FString WeaponDigest = SessionDigest;
        Writer << WeaponDigest;
        uint8 Kind = uint8(ModeRules->Kind), Selection = uint8(ModeRules->Selection), Replacement = uint8(ModeRules->Replacement), DeathDrop = uint8(ModeRules->DeathDrop);
        Writer << Kind; Writer << Selection; Writer << Replacement; Writer << DeathDrop;
        float Lifetime = ModeRules->DynamicDropLifetime; int32 Limit = ModeRules->DynamicDropLimit;
        float SelectionInterval = ModeRules->SelectionRequestInterval;
        Writer << SelectionInterval;
        Writer << Lifetime; Writer << Limit;
        auto WriteId = [&](const TSoftObjectPtr<UFPSWeaponDefinition>& Reference)
        {
            FString Id = Manager.GetPrimaryAssetIdForPath(Reference.ToSoftObjectPath()).PrimaryAssetName.ToString().ToLower();
            Writer << Id;
        };
        WriteId(ModeRules->DefaultLoadout->Primary); WriteId(ModeRules->DefaultLoadout->Secondary);
        int32 Count = ModeRules->SelectableWeapons.Num(); Writer << Count;
        for (const auto& Reference : ModeRules->SelectableWeapons) WriteId(Reference);
        SessionDigest = Hash(Bytes);
    }
    if (!Errors.IsEmpty())
    {
        const FString Failure = ValidationError + FString::Join(Errors, TEXT("\n"));
        ReleaseSession(); ValidationError = Failure;
        UE_LOG(LogTemp, Error, TEXT("FPS_WEAPON_CONTENT MAP BLOCKED %s"), *ValidationError);
    }
    SessionWorld = World;
    bReady = ValidationError.IsEmpty();
    return bReady;
}

bool UFPSWeaponCatalog::PrepareDependencies(const TArray<FName>& Ids)
{
    ReleaseSession();
    const double Started = FPlatformTime::Seconds();
    SessionIds = Ids;
    SessionIds.Sort(FNameLexicalLess());
    TArray<FString> Errors;
    TArray<FSoftObjectPath> Paths;
    auto& Manager = UAssetManager::Get();
    auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    TArray<FAssetData> Metadata;
    Registry.GetAssetsByClass(UFPSWeaponDefinition::StaticClass()->GetClassPathName(), Metadata, true);
    for (FName Id : SessionIds)
    {
        int32 Matches = 0;
        for (const auto& Asset : Metadata)
        {
            FName TaggedId; Asset.GetTagValue(TEXT("WeaponId"), TaggedId);
            if (TaggedId == Id) ++Matches;
        }
        if (Matches != 1) Errors.Add(FString::Printf(TEXT("Required WeaponId %s has %d definitions"), *Id.ToString(), Matches));
        const auto Path = Manager.GetPrimaryAssetPath(FPrimaryAssetId(TEXT("WeaponDefinition"), Id));
        if (Path.IsNull()) Errors.Add(TEXT("Missing required definition: ") + Id.ToString()); else Paths.AddUnique(Path);
    }
    if (Errors.IsEmpty() && !Paths.IsEmpty()) DefinitionHandle = Manager.GetStreamableManager().RequestSyncLoad(Paths, false, TEXT("FPS match definitions"));
    TArray<FSoftObjectPath> Resources;
    if (Errors.IsEmpty())
        for (FName Id : SessionIds)
        {
            const auto Path = Manager.GetPrimaryAssetPath(FPrimaryAssetId(TEXT("WeaponDefinition"), Id));
            auto* Def = Cast<UFPSWeaponDefinition>(Path.ResolveObject());
            if (!Def || Def->WeaponId != Id) { Errors.Add(TEXT("Required definition failed to load: ") + Id.ToString()); continue; }
            Definitions.Add(Id, Def);
            for (const auto& Resource : Def->Resources()) Resources.AddUnique(Resource);
        }
    if (Errors.IsEmpty() && !Resources.IsEmpty()) ResourceHandle = Manager.GetStreamableManager().RequestSyncLoad(Resources, false, TEXT("FPS match resources"));
    TArray<uint8> Bytes;
    FMemoryWriter Writer(Bytes);
    int32 Version = ManifestVersion; Writer << Version;
    for (FName Id : SessionIds)
        if (auto* Def = Definitions.FindRef(Id).Get())
        {
#if !UE_BUILD_SHIPPING
            // Process-local fault injection; assets are never saved by runtime checks.
            const float OriginalInterval = Def->FireInterval;
            FString InvalidId; FParse::Value(FCommandLine::Get(), TEXT("FPSInvalidWeapon="), InvalidId);
            if (InvalidId == Id.ToString() || (FParse::Param(FCommandLine::Get(), TEXT("FPSWeaponInvalidDataTest")) && Id == SessionIds[0])) Def->FireInterval = -1;
#endif
            Def->ValidateDefinition(Errors);
            if (Id == StartingPrimary && Def->Slot != EFPSWeaponSlot::Primary) Errors.Add(TEXT("Wrong-slot starting primary"));
            if (Id == StartingSecondary && Def->Slot != EFPSWeaponSlot::Secondary) Errors.Add(TEXT("Wrong-slot starting secondary"));
#if !UE_BUILD_SHIPPING
            Def->FireInterval = OriginalInterval;
            FString Change; FParse::Value(FCommandLine::Get(), TEXT("FPSContentChange="), Change);
            if (Id == StartingPrimary && Change == TEXT("capacity")) ++Def->MagazineSize;
            if (Id == StartingPrimary && Change == TEXT("interval")) Def->FireInterval += .01f;
#endif
            FString Name = Id.ToString().ToLower(), Digest = Def->ContentDigest(Errors);
            Writer << Name; Writer << Digest;
#if !UE_BUILD_SHIPPING
            if (Id == StartingPrimary && Change == TEXT("capacity")) --Def->MagazineSize;
            Def->FireInterval = OriginalInterval;
#endif
        }
    ValidationError = FString::Join(Errors, TEXT("\n"));
    bReady = Errors.IsEmpty();
    if (bReady) SessionDigest = Hash(Bytes);
    else { Definitions.Empty(); if (ResourceHandle) ResourceHandle->ReleaseHandle(); if (DefinitionHandle) DefinitionHandle->ReleaseHandle(); ResourceHandle.Reset(); DefinitionHandle.Reset(); }
    UE_LOG(LogTemp, Display, TEXT("FPS_WEAPON_CONTENT %s definitions=%d resources=%d ms=%.2f digest=%s error=%s"),
        bReady ? TEXT("READY") : TEXT("BLOCKED"), Definitions.Num(), Resources.Num(), (FPlatformTime::Seconds()-Started)*1000, *SessionDigest, *ValidationError);
    return bReady;
}
