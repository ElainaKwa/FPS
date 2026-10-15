#pragma once
#include "CoreMinimal.h"
class UStaticMeshComponent;
class UFPSWeaponCatalog;
class UFPSWeaponDefinition;
enum class EFPSAttachmentView : uint8 { FirstPerson, ThirdPerson, Pickup };
/** Uses only admitted, already loaded assets. Parts are children of the corresponding weapon mesh. */
void RefreshFPSAttachmentMeshes(UStaticMeshComponent* Parent, const UFPSWeaponCatalog* Catalog, const UFPSWeaponDefinition* Definition, const TArray<FName>& Ids,
    EFPSAttachmentView View, TArray<TObjectPtr<UStaticMeshComponent>>& Components);
