#include "FPSWeaponShot.h"
#include "FPSDemoCharacter.h"
#include "FPSWeaponComponent.h"
#include "Sound/SoundBase.h"

FFPSWeaponShotContext FFPSWeaponShotContext::Capture(AFPSDemoCharacter* Source, const FFPSWeaponInstance& Gun, const UFPSWeaponDefinition& Definition)
{
    FFPSWeaponShotContext Shot;
    if (!Source || !Source->HasAuthority() || !Gun.IsValid() || Gun.WeaponId != Definition.WeaponId) return Shot;
    FFPSResolvedWeaponStats Stats; TArray<FString> Errors;
    const auto* Catalog = Source->GetWeapons()->Catalog();
    if (!Catalog || !Catalog->Resolve(Gun.WeaponId, Gun.AttachmentIds, Stats, Errors)) return Shot;
    const auto* Profile = Definition.HitDamageProfile.Get();
    if (!Profile || !Profile->Multipliers.IsValid()) return Shot;
    Shot.HitMultipliers = Profile->Multipliers;
    Shot.Source = Source;
    Shot.WeaponId = Gun.WeaponId;
    Shot.InstanceId = Gun.InstanceId;
    Shot.ShotSerial = Gun.ShotSerial;
    Shot.BaseDamage = Definition.Damage;
    Shot.Range = Stats.Range;
    Shot.FireSound = Stats.FireSound.Get();
    return Shot;
}
