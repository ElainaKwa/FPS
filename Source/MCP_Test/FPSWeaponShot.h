#pragma once

#include "CoreMinimal.h"
#include "FPSWeaponData.h"
#include "FPSHitDamage.h"

class AFPSDemoCharacter;

/** Server-local value snapshot. Never accepted from clients or used as permission to fire.
 * The active Ability/component validates and consumes ammo before resolving this shot.
 * Damage is definition base damage; WeaponDamage on the ASC is only an equipment display cache.
 */
struct FFPSWeaponShotContext
{
    TWeakObjectPtr<AFPSDemoCharacter> Source;
    FName WeaponId;
    FGuid InstanceId;
    uint32 ShotSerial = 0;
    float BaseDamage = 0;
    FFPSHitDamageMultipliers HitMultipliers;
    float Range = 0;
    TWeakObjectPtr<USoundBase> FireSound;

    static FFPSWeaponShotContext Capture(AFPSDemoCharacter* Source, const FFPSWeaponInstance& Gun, const UFPSWeaponDefinition& Definition);
};
