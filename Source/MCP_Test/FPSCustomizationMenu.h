#pragma once
#include "CoreMinimal.h"
class AFPSDemoCharacter;
class SWidget;
TSharedRef<SWidget> MakeFPSCustomizationMenu(AFPSDemoCharacter* Character, TFunction<void()> Close);
#if !UE_BUILD_SHIPPING
bool FPSCustomizationSmokePointer(class UFPSWeaponComponent* Weapon, FName Part, int32 Phase);
#endif
