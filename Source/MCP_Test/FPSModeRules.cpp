#include "FPSModeRules.h"
#include "Engine/AssetManager.h"
#include "EngineUtils.h"

UFPSModeRules* UFPSModeRules::ForWorld(UWorld* World, TArray<FString>* Errors)
{
    UFPSModeRules* Result = nullptr;
    int32 Count = 0;
    for (TActorIterator<AFPSModeSetup> It(World); It; ++It) { ++Count; Result = It->Rules.LoadSynchronous(); }
    if (Count != 1 || !Result)
    {
        if (Errors) Errors->Add(TEXT("Structured map requires exactly one valid FPSModeSetup"));
        return nullptr;
    }
    return Result;
}
void UFPSModeRules::GatherWeaponIds(TArray<FName>& Ids, TArray<FString>& Errors) const
{
    if (DynamicDropLimit < 1 || !FMath::IsFinite(DynamicDropLifetime) || DynamicDropLifetime <= 0 ||
        !FMath::IsFinite(SelectionRequestInterval) || SelectionRequestInterval < 0)
        Errors.Add(TEXT("Mode requires a positive dynamic-drop cap/lifetime and valid selection request interval"));
    auto Add = [&](const TSoftObjectPtr<UFPSWeaponDefinition>& Reference, const EFPSWeaponSlot* Slot)
    {
        const auto Id = UAssetManager::Get().GetPrimaryAssetIdForPath(Reference.ToSoftObjectPath());
        if (!Id.IsValid() || Id.PrimaryAssetType != FPrimaryAssetType(TEXT("WeaponDefinition")))
        { Errors.Add(TEXT("Invalid mode weapon reference: ") + Reference.ToString()); return; }
        Ids.AddUnique(Id.PrimaryAssetName);
        if (Slot)
        {
            const auto* Def = Reference.LoadSynchronous();
            if (!Def || Def->Slot != *Slot) Errors.Add(TEXT("Wrong-slot spawn definition: ") + Reference.ToString());
        }
    };
    if (!DefaultLoadout) Errors.Add(TEXT("Mode has no default spawn loadout"));
    else
    {
        const EFPSWeaponSlot Primary = EFPSWeaponSlot::Primary, Secondary = EFPSWeaponSlot::Secondary;
        Add(DefaultLoadout->Primary, &Primary); Add(DefaultLoadout->Secondary, &Secondary);
    }
    if (SelectableWeapons.IsEmpty()) Errors.Add(TEXT("Mode has no selectable weapons"));
    for (const auto& Reference : SelectableWeapons) Add(Reference, nullptr);
}
