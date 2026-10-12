#pragma once
#include "CoreMinimal.h"

#if WITH_EDITOR
/** Editor operations shared by the workbench and its editor regression checks. */
namespace FPSWeaponAuthoring
{
    void Register();
    void Unregister();
    void Open();
    void Load(TArray<TObjectPtr<UObject>>& Assets, TArray<FString>& Errors);
    void Validate(UObject* Asset, TArray<FString>& Errors);
    FString Number(const UObject* Asset, FName Field);
    bool SetNumber(const TArray<UObject*>& Assets, FName Field, const FString& Text, FString& Failure);
    bool Save(const TArray<UObject*>& Assets, FString& Failure);
    UObject* CopyTemplate(UObject* Source, FString& Failure);
}
#endif
