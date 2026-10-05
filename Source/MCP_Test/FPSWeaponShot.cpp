#include "FPSWeaponShot.h"
#include "FPSDemoCharacter.h"
#include "Sound/SoundBase.h"

FFPSWeaponShotContext FFPSWeaponShotContext::Capture(AFPSDemoCharacter* Source, const FFPSWeaponInstance& Gun, const UFPSWeaponDefinition& Definition)
{
    FFPSWeaponShotContext Shot;
    if (!Source || !Source->HasAuthority() || !Gun.IsValid() || Gun.WeaponId != Definition.WeaponId) return Shot;
    Shot.Source = Source;
    Shot.WeaponId = Gun.WeaponId;
    Shot.InstanceId = Gun.InstanceId;
    Shot.ShotSerial = Gun.ShotSerial;
    Shot.BaseDamage = Definition.Damage;
    Shot.Range = Definition.Range;
    Shot.FireSound = Definition.FireSound.Get();
    return Shot;
}
