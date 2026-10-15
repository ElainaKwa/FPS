# Weapon asset review (GENERATED - DO NOT EDIT)

- 用途：自动生成资料
- 适用范围：已保存武器定义与地图武器台
- 状态：只读导出，禁止手工修改
- 最后更新：2026-10-15

Source: saved Unreal assets. Regenerate with Tools/ValidateFPSWeaponData.ps1 -Export.

Starting primary=Weapon.M4; secondary=Weapon.M9

## WeaponDefinition:Weapon.AK

Asset: /Game/FPSDemo/Weapons/Definitions/DA_AK.DA_AK

```text
WeaponId=Weapon.AK
DisplayName=NSLOCTEXT("", "BB0478694AE532B61F22CE869323ED28", "AK")
PlatformId=None
MountIds=
AvailableAttachments=
DefaultAttachments=
Slot=Primary
Category=Rifle
MagazineSize=30
InitialMagazine=30
InitialReserve=90
Damage=34.000000
HitDamageProfile=/Game/FPSDemo/Weapons/Damage/DA_HitDamage_Default.DA_HitDamage_Default
FireInterval=0.110000
Range=9000.000000
FireMode=Automatic
BurstCount=3
BurstInterval=0.300000
ReloadDuration=2.400000
InspectDuration=2.000000
EquipReadyTime=0.350000
EquipBlendTime=0.350000
AimTransitionTime=0.200000
bNoRecoil=False
RecoilCurve=/Game/FPSDemo/Weapons/Curves/CR_AK.CR_AK
AimRecoilMultiplier=0.650000
RecoilRecoveryTime=0.500000
FirstPerson=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_AK.SM_IND_AK",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=0.000000,Y=0.000000,Z=0.000000),Scale3D=(X=1.000000,Y=1.000000,Z=1.000000)))
ThirdPerson=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_AK.SM_IND_AK",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=43.000000,Y=8.000000,Z=17.000000),Scale3D=(X=1.000000,Y=1.000000,Z=1.000000)))
Pickup=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_AK.SM_IND_AK",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=0.000000,Y=0.000000,Z=0.000000),Scale3D=(X=0.650000,Y=0.650000,Z=0.650000)))
ModularBody=None
ModularReloadBody=None
WorkbenchBody=None
BuiltinParts=()
WorkbenchParts=()
AttachmentMountLocations=()
HipLocation=(X=42.000000,Y=18.000000,Z=-18.000000)
AimLocation=(X=38.000000,Y=0.000000,Z=-7.200001)
MuzzleLocation=(X=56.349998,Y=0.000000,Z=0.000000)
AimFOV=62.000000
FireSound=/Game/FPSDemo/Audio/S_Fire_AK.S_Fire_AK
ReloadStartSound=/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart
ReloadEndSound=/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd
EquipSound=/Game/FPSDemo/Audio/S_Equip.S_Equip
InspectSound=/Game/FPSDemo/Audio/S_Inspect.S_Inspect
EmptySound=/Game/FPSDemo/Audio/S_Empty.S_Empty
SavedResourceVersions=(("/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_AK.SM_IND_AK", "095c06e29929f96c7cbfe28850f26e5c5634bf04;157d1b4054217c93d6b89ce108495a180418f104;17d8b3f132e839cd9a6579aa38d8d5364212aae8;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;6e16f36697c4fa728f0a658ae68a709000e0ef24;96346e800660364554360fe27c38794282734669;a7540970dde006cb0dc6edc4faa4880e7ddd1e2c;b7fdb68f088695b234844a8e0458c4d9311cfc0f;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Audio/S_Fire_AK.S_Fire_AK", "ae521ce701062a790cee1e35be9f2984295b3328"),("/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart", "bce5feebf15a486663e6668b339db78111ed9198"),("/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd", "4dc92cf128570379d6c8ef4fd0964ecd4dc11056"),("/Game/FPSDemo/Audio/S_Equip.S_Equip", "9c7ef98bc8e56e0b8935942375a6fcf34aa02a8e"),("/Game/FPSDemo/Audio/S_Inspect.S_Inspect", "63d3ea395754e83b054bcdff26476f4604791099"),("/Game/FPSDemo/Audio/S_Empty.S_Empty", "36f6cfd85a175c41a5a0360fb5f2026111f953cb"))
HitDamageProfileId=HitDamage.Default Head=2 Torso=1 Limb=0.75
Curve[0] time=0 value=1.25 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=1 value=1.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=2 value=1.5 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=3 value=1.625 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=4 value=1.75 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=5 value=1.875 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=6 value=2 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=7 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=8 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=9 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=10 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=11 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=12 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=13 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=14 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=15 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=16 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=17 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=18 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=19 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=20 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=21 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=22 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=23 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=24 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=25 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=26 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=27 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=28 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=29 value=2.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=0 value=0.275000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=1 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=2 value=0.275000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=3 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=4 value=0.275000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=5 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=6 value=0.275000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=7 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=8 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=9 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=10 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=11 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=12 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=13 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=14 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=15 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=16 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=17 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=18 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=19 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=20 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=21 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=22 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=23 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=24 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=25 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=26 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=27 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=28 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=29 value=-0.375 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=0 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=1 value=1.03999996 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=2 value=1.08000004 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=3 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=4 value=1.15999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=5 value=1.20000005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=6 value=1.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=7 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=8 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=9 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=10 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=11 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=12 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=13 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=14 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=15 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=16 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=17 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=18 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=19 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=20 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=21 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=22 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=23 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=24 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=25 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=26 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=27 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=28 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=29 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
```

## WeaponDefinition:Weapon.BR3

Asset: /Game/FPSDemo/Weapons/Definitions/DA_BR3.DA_BR3

```text
WeaponId=Weapon.BR3
DisplayName=NSLOCTEXT("", "3A3E0C194C613E5A5206609432350E61", "BR-3")
PlatformId=None
MountIds=
AvailableAttachments=
DefaultAttachments=
Slot=Primary
Category=Rifle
MagazineSize=30
InitialMagazine=30
InitialReserve=90
Damage=28.000000
HitDamageProfile=/Game/FPSDemo/Weapons/Damage/DA_HitDamage_Default.DA_HitDamage_Default
FireInterval=0.090000
Range=9000.000000
FireMode=Burst
BurstCount=3
BurstInterval=0.350000
ReloadDuration=2.300000
InspectDuration=2.000000
EquipReadyTime=0.250000
EquipBlendTime=0.400000
AimTransitionTime=0.200000
bNoRecoil=False
RecoilCurve=/Game/FPSDemo/Weapons/Curves/CR_BR3.CR_BR3
AimRecoilMultiplier=0.650000
RecoilRecoveryTime=0.650000
FirstPerson=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_BR3.SM_IND_BR3",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=0.000000,Y=0.000000,Z=0.000000),Scale3D=(X=1.000000,Y=1.000000,Z=1.000000)))
ThirdPerson=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_BR3.SM_IND_BR3",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=43.000000,Y=8.000000,Z=17.000000),Scale3D=(X=1.000000,Y=1.000000,Z=1.000000)))
Pickup=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_BR3.SM_IND_BR3",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=0.000000,Y=0.000000,Z=0.000000),Scale3D=(X=0.650000,Y=0.650000,Z=0.650000)))
ModularBody=None
ModularReloadBody=None
WorkbenchBody=None
BuiltinParts=()
WorkbenchParts=()
AttachmentMountLocations=()
HipLocation=(X=42.000000,Y=18.000000,Z=-18.000000)
AimLocation=(X=38.000000,Y=0.000000,Z=-7.200001)
MuzzleLocation=(X=58.850002,Y=0.000000,Z=0.000000)
AimFOV=62.000000
FireSound=/Game/FPSDemo/Audio/S_Fire_M4.S_Fire_M4
ReloadStartSound=/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart
ReloadEndSound=/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd
EquipSound=/Game/FPSDemo/Audio/S_Equip.S_Equip
InspectSound=/Game/FPSDemo/Audio/S_Inspect.S_Inspect
EmptySound=/Game/FPSDemo/Audio/S_Empty.S_Empty
SavedResourceVersions=(("/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_BR3.SM_IND_BR3", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;2b30a1e92649807cf385092921cde277b9049d57;32e30fa5b0e7f1bec901d1da20c68f0107978898;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;6e16f36697c4fa728f0a658ae68a709000e0ef24;b7fdb68f088695b234844a8e0458c4d9311cfc0f;cb021642d46be93eed096a80813f35eb9b48ee7a;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;df4470fad9a70d925c9b939813dc4a847d570967;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Audio/S_Fire_M4.S_Fire_M4", "c573d556ac962fd89151d8e56a222fda49d20ee3"),("/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart", "bce5feebf15a486663e6668b339db78111ed9198"),("/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd", "4dc92cf128570379d6c8ef4fd0964ecd4dc11056"),("/Game/FPSDemo/Audio/S_Equip.S_Equip", "9c7ef98bc8e56e0b8935942375a6fcf34aa02a8e"),("/Game/FPSDemo/Audio/S_Inspect.S_Inspect", "63d3ea395754e83b054bcdff26476f4604791099"),("/Game/FPSDemo/Audio/S_Empty.S_Empty", "36f6cfd85a175c41a5a0360fb5f2026111f953cb"))
HitDamageProfileId=HitDamage.Default Head=2 Torso=1 Limb=0.75
Curve[0] time=0 value=0.720000029 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=1 value=0.814999998 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=2 value=0.909999967 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=3 value=0.765000045 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=4 value=0.860000014 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=5 value=0.954999983 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=6 value=0.810000002 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=7 value=0.905000031 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=8 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=9 value=0.855000019 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=10 value=0.949999988 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=11 value=1.04499996 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=12 value=0.900000036 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=13 value=0.995000005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=14 value=1.09000003 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=15 value=0.945000052 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=16 value=1.03999996 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=17 value=1.13499999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=18 value=0.99000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=19 value=1.08500004 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=20 value=1.17999995 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=21 value=1.03500009 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=22 value=1.13 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=23 value=1.22500002 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=24 value=1.08000004 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=25 value=1.17499995 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=26 value=1.26999998 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=27 value=1.125 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=28 value=1.22000003 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=29 value=1.31500006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=0 value=-0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=1 value=0 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=2 value=0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=3 value=-0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=4 value=0 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=5 value=0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=6 value=-0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=7 value=0 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=8 value=0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=9 value=-0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=10 value=0 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=11 value=0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=12 value=-0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=13 value=0 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=14 value=0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=15 value=-0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=16 value=0 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=17 value=0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=18 value=-0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=19 value=0 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=20 value=0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=21 value=-0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=22 value=0 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=23 value=0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=24 value=-0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=25 value=0 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=26 value=0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=27 value=-0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=28 value=0 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=29 value=0.119999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=0 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=1 value=1.05999994 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=2 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=3 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=4 value=1.05999994 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=5 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=6 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=7 value=1.05999994 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=8 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=9 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=10 value=1.05999994 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=11 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=12 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=13 value=1.05999994 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=14 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=15 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=16 value=1.05999994 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=17 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=18 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=19 value=1.05999994 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=20 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=21 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=22 value=1.05999994 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=23 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=24 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=25 value=1.05999994 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=26 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=27 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=28 value=1.05999994 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=29 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
```

## WeaponDefinition:Weapon.HeavyPistol

Asset: /Game/FPSDemo/Weapons/Definitions/DA_HeavyPistol.DA_HeavyPistol

```text
WeaponId=Weapon.HeavyPistol
DisplayName=NSLOCTEXT("", "4E29F1524F581E1BE11369BD4968F6A4", "Heavy Pistol")
PlatformId=None
MountIds=
AvailableAttachments=
DefaultAttachments=
Slot=Secondary
Category=Pistol
MagazineSize=7
InitialMagazine=7
InitialReserve=35
Damage=55.000000
HitDamageProfile=/Game/FPSDemo/Weapons/Damage/DA_HitDamage_Default.DA_HitDamage_Default
FireInterval=0.380000
Range=5000.000000
FireMode=Single
BurstCount=3
BurstInterval=0.300000
ReloadDuration=1.900000
InspectDuration=2.000000
EquipReadyTime=0.350000
EquipBlendTime=0.350000
AimTransitionTime=0.200000
bNoRecoil=False
RecoilCurve=/Game/FPSDemo/Weapons/Curves/CR_HeavyPistol.CR_HeavyPistol
AimRecoilMultiplier=0.650000
RecoilRecoveryTime=0.500000
FirstPerson=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_HeavyPistol.SM_IND_HeavyPistol",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=0.000000,Y=0.000000,Z=0.000000),Scale3D=(X=1.000000,Y=1.000000,Z=1.000000)))
ThirdPerson=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_HeavyPistol.SM_IND_HeavyPistol",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=39.000000,Y=8.000000,Z=17.000000),Scale3D=(X=1.000000,Y=1.000000,Z=1.000000)))
Pickup=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_HeavyPistol.SM_IND_HeavyPistol",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=0.000000,Y=0.000000,Z=0.000000),Scale3D=(X=0.650000,Y=0.650000,Z=0.650000)))
ModularBody=None
ModularReloadBody=None
WorkbenchBody=None
BuiltinParts=()
WorkbenchParts=()
AttachmentMountLocations=()
HipLocation=(X=32.000000,Y=12.000000,Z=-14.000000)
AimLocation=(X=24.000000,Y=0.000000,Z=-5.100000)
MuzzleLocation=(X=25.350000,Y=0.000000,Z=0.000000)
AimFOV=72.000000
FireSound=/Game/FPSDemo/Audio/S_Fire_HeavyPistol.S_Fire_HeavyPistol
ReloadStartSound=/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart
ReloadEndSound=/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd
EquipSound=/Game/FPSDemo/Audio/S_Equip.S_Equip
InspectSound=/Game/FPSDemo/Audio/S_Inspect.S_Inspect
EmptySound=/Game/FPSDemo/Audio/S_Empty.S_Empty
SavedResourceVersions=(("/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_HeavyPistol.SM_IND_HeavyPistol", "072ba8fe93a1ed949b21b60e855566221346e40f;07c529d29f02014fca361381429a9acd6724582b;17d8b3f132e839cd9a6579aa38d8d5364212aae8;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;abee33bce9cb180c62b2c6d9fece9f5e5df9976c;b7fdb68f088695b234844a8e0458c4d9311cfc0f;bbde49af579a1982ea65f529cf648469c6dfcd2c;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Audio/S_Fire_HeavyPistol.S_Fire_HeavyPistol", "5708873d4b060116c449b3b460cb14722330012f"),("/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart", "bce5feebf15a486663e6668b339db78111ed9198"),("/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd", "4dc92cf128570379d6c8ef4fd0964ecd4dc11056"),("/Game/FPSDemo/Audio/S_Equip.S_Equip", "9c7ef98bc8e56e0b8935942375a6fcf34aa02a8e"),("/Game/FPSDemo/Audio/S_Inspect.S_Inspect", "63d3ea395754e83b054bcdff26476f4604791099"),("/Game/FPSDemo/Audio/S_Empty.S_Empty", "36f6cfd85a175c41a5a0360fb5f2026111f953cb"))
HitDamageProfileId=HitDamage.Default Head=2 Torso=1 Limb=0.75
Curve[0] time=0 value=2 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=1 value=2.20000005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=2 value=2.4000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=3 value=2.5999999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=4 value=2.79999995 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=5 value=3 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=6 value=3.20000005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=7 value=3.4000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=0 value=0.439999998 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=1 value=-0.600000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=2 value=0.439999998 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=3 value=-0.600000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=4 value=0.439999998 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=5 value=-0.600000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=6 value=0.439999998 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=7 value=-0.600000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=0 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=1 value=1.03999996 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=2 value=1.08000004 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=3 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=4 value=1.15999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=5 value=1.20000005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=6 value=1.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=7 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
```

## WeaponDefinition:Weapon.M4

Asset: /Game/FPSDemo/Weapons/Definitions/DA_M4.DA_M4

```text
WeaponId=Weapon.M4
DisplayName=NSLOCTEXT("", "929FAB41457E4F5514173A9C8BA78987", "M4")
PlatformId=Platform.M4
MountIds=("Mount.Optic","Mount.Muzzle","Mount.Magazine","Mount.RearGrip","Mount.Stock","Mount.Barrel")
AvailableAttachments=("/Game/FPSDemo/Weapons/Attachments/DA_M4_OpticStandard.DA_M4_OpticStandard","/Game/FPSDemo/Weapons/Attachments/DA_M4_OpticFixed.DA_M4_OpticFixed","/Game/FPSDemo/Weapons/Attachments/DA_M4_MuzzleStandard.DA_M4_MuzzleStandard","/Game/FPSDemo/Weapons/Attachments/DA_M4_MuzzleBrake.DA_M4_MuzzleBrake","/Game/FPSDemo/Weapons/Attachments/DA_M4_MagazineStandard.DA_M4_MagazineStandard","/Game/FPSDemo/Weapons/Attachments/DA_M4_MagazineExtended.DA_M4_MagazineExtended","/Game/FPSDemo/Weapons/Attachments/DA_M4_RearGripErgo.DA_M4_RearGripErgo","/Game/FPSDemo/Weapons/Attachments/DA_M4_RearGripControl.DA_M4_RearGripControl","/Game/FPSDemo/Weapons/Attachments/DA_M4_StockLight.DA_M4_StockLight","/Game/FPSDemo/Weapons/Attachments/DA_M4_StockStable.DA_M4_StockStable","/Game/FPSDemo/Weapons/Attachments/DA_M4_BarrelLight.DA_M4_BarrelLight","/Game/FPSDemo/Weapons/Attachments/DA_M4_BarrelHeavy.DA_M4_BarrelHeavy")
DefaultAttachments=
Slot=Primary
Category=Rifle
MagazineSize=30
InitialMagazine=30
InitialReserve=120
Damage=25.000000
HitDamageProfile=/Game/FPSDemo/Weapons/Damage/DA_HitDamage_Default.DA_HitDamage_Default
FireInterval=0.085000
Range=9000.000000
FireMode=Automatic
BurstCount=3
BurstInterval=0.300000
ReloadDuration=2.100000
InspectDuration=2.000000
EquipReadyTime=0.350000
EquipBlendTime=0.350000
AimTransitionTime=0.200000
bNoRecoil=False
RecoilCurve=/Game/FPSDemo/Weapons/Curves/CR_M4.CR_M4
AimRecoilMultiplier=0.650000
RecoilRecoveryTime=0.500000
FirstPerson=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_M4.SM_IND_M4",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=0.000000,Y=0.000000,Z=0.000000),Scale3D=(X=1.000000,Y=1.000000,Z=1.000000)))
ThirdPerson=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_M4.SM_IND_M4",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=43.000000,Y=8.000000,Z=17.000000),Scale3D=(X=1.000000,Y=1.000000,Z=1.000000)))
Pickup=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_M4.SM_IND_M4",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=0.000000,Y=0.000000,Z=0.000000),Scale3D=(X=0.650000,Y=0.650000,Z=0.650000)))
ModularBody=/Game/FPSDemo/Art/Weapons/Rifles/M4/Modular/Meshes/SM_M4_ModularBody.SM_M4_ModularBody
ModularReloadBody=/Game/FPSDemo/Art/Weapons/Rifles/M4/Modular/Meshes/SM_M4_ModularReloadBody.SM_M4_ModularReloadBody
WorkbenchBody=/Game/FPSDemo/Art/Weapons/Rifles/M4/Modular/Meshes/SM_M4_ModularWorkbenchBody.SM_M4_ModularWorkbenchBody
BuiltinParts=(("Mount.RearGrip", (Mesh="/Game/FPSDemo/Art/Weapons/Attachments/RearGrips/M4/Meshes/SM_M4_ModularRearGripStandard.SM_M4_ModularRearGripStandard")),("Mount.Stock", (Mesh="/Game/FPSDemo/Art/Weapons/Attachments/Stocks/M4/Meshes/SM_M4_ModularStockStandard.SM_M4_ModularStockStandard")),("Mount.Barrel", (Mesh="/Game/FPSDemo/Art/Weapons/Attachments/Barrels/M4/Meshes/SM_M4_ModularBarrelStandard.SM_M4_ModularBarrelStandard")))
WorkbenchParts=(("Mount.Muzzle", (Mesh="/Game/FPSDemo/Art/Weapons/M4/ExtractionPreview/SM_EXP_M4_MuzzleStandard.SM_EXP_M4_MuzzleStandard")),("Mount.Magazine", (Mesh="/Game/FPSDemo/Art/Weapons/M4/ExtractionPreview/SM_EXP_M4_MagazineStandard.SM_EXP_M4_MagazineStandard")))
AttachmentMountLocations=(("Mount.Optic", (X=10.000000,Y=0.000000,Z=10.000000)),("Mount.Muzzle", (X=56.000000,Y=0.000000,Z=0.000000)),("Mount.Magazine", (X=15.000000,Y=0.000000,Z=-17.000000)),("Mount.RearGrip", (X=-2.000000,Y=0.000000,Z=-8.000000)),("Mount.Stock", (X=-28.000000,Y=0.000000,Z=-2.000000)),("Mount.Barrel", (X=48.000000,Y=0.000000,Z=0.000000)))
HipLocation=(X=42.000000,Y=18.000000,Z=-18.000000)
AimLocation=(X=38.000000,Y=0.000000,Z=-7.200001)
MuzzleLocation=(X=56.349998,Y=0.000000,Z=0.000000)
AimFOV=62.000000
FireSound=/Game/FPSDemo/Audio/S_Fire_M4.S_Fire_M4
ReloadStartSound=/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart
ReloadEndSound=/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd
EquipSound=/Game/FPSDemo/Audio/S_Equip.S_Equip
InspectSound=/Game/FPSDemo/Audio/S_Inspect.S_Inspect
EmptySound=/Game/FPSDemo/Audio/S_Empty.S_Empty
SavedResourceVersions=(("/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_M4.SM_IND_M4", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;4f61ba6383ac4389b7418328702f5a1fc7c0e7c4;55e0d77a3d14970ec08deaaf476affb39e439030;6d997235bbe3e48c340cbf149642a8bd6768d53d;6e16f36697c4fa728f0a658ae68a709000e0ef24;8bb420526e950e8eea75758a72247dec8a4d72f3;b7fdb68f088695b234844a8e0458c4d9311cfc0f;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Audio/S_Fire_M4.S_Fire_M4", "c573d556ac962fd89151d8e56a222fda49d20ee3"),("/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart", "bce5feebf15a486663e6668b339db78111ed9198"),("/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd", "4dc92cf128570379d6c8ef4fd0964ecd4dc11056"),("/Game/FPSDemo/Audio/S_Equip.S_Equip", "9c7ef98bc8e56e0b8935942375a6fcf34aa02a8e"),("/Game/FPSDemo/Audio/S_Inspect.S_Inspect", "63d3ea395754e83b054bcdff26476f4604791099"),("/Game/FPSDemo/Audio/S_Empty.S_Empty", "36f6cfd85a175c41a5a0360fb5f2026111f953cb"),("/Game/FPSDemo/Art/Weapons/Rifles/M4/Modular/Meshes/SM_M4_ModularBody.SM_M4_ModularBody", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;6e16f36697c4fa728f0a658ae68a709000e0ef24;bd9fda120b1802f0ac663487d6e41b3329eb832f;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Art/Weapons/Rifles/M4/Modular/Meshes/SM_M4_ModularReloadBody.SM_M4_ModularReloadBody", "078e1af82d4c5ca0ee8084e7125733a087129892;17d8b3f132e839cd9a6579aa38d8d5364212aae8;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;6e16f36697c4fa728f0a658ae68a709000e0ef24;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Art/Weapons/Rifles/M4/Modular/Meshes/SM_M4_ModularWorkbenchBody.SM_M4_ModularWorkbenchBody", "02db35d48cdb3c1f658a6e8831e3eb1f7f062c74;17d8b3f132e839cd9a6579aa38d8d5364212aae8;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;6e16f36697c4fa728f0a658ae68a709000e0ef24;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Art/Weapons/Attachments/RearGrips/M4/Meshes/SM_M4_ModularRearGripStandard.SM_M4_ModularRearGripStandard", "127015f1d0855be2b18837416728c1b91b2deed1;b7fdb68f088695b234844a8e0458c4d9311cfc0f;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Art/Weapons/Attachments/Stocks/M4/Meshes/SM_M4_ModularStockStandard.SM_M4_ModularStockStandard", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;a92cf534023c974e400433b22b78f6373a0436c4;b7fdb68f088695b234844a8e0458c4d9311cfc0f;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Art/Weapons/Attachments/Barrels/M4/Meshes/SM_M4_ModularBarrelStandard.SM_M4_ModularBarrelStandard", "6f5b98b26255ce6a1dff33b29e51e98ba1fbd930;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3"),("/Game/FPSDemo/Art/Weapons/M4/ExtractionPreview/SM_EXP_M4_MuzzleStandard.SM_EXP_M4_MuzzleStandard", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;d3c29c117fe670d5e15f4885ae5a921c00e43dc4;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Art/Weapons/M4/ExtractionPreview/SM_EXP_M4_MagazineStandard.SM_EXP_M4_MagazineStandard", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;e7bd4795718f103898a10810d277912bf585955f;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3"))
HitDamageProfileId=HitDamage.Default Head=2 Torso=1 Limb=0.75
Curve[0] time=0 value=0.800000012 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=1 value=0.880000055 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=2 value=0.960000038 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=3 value=1.03999996 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=4 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=5 value=1.20000005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=6 value=1.28000009 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=7 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=8 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=9 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=10 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=11 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=12 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=13 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=14 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=15 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=16 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=17 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=18 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=19 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=20 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=21 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=22 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=23 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=24 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=25 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=26 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=27 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=28 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=29 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=0 value=0.175999999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=1 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=2 value=0.175999999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=3 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=4 value=0.175999999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=5 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=6 value=0.175999999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=7 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=8 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=9 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=10 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=11 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=12 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=13 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=14 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=15 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=16 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=17 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=18 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=19 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=20 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=21 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=22 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=23 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=24 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=25 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=26 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=27 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=28 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=29 value=-0.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=0 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=1 value=1.03999996 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=2 value=1.08000004 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=3 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=4 value=1.15999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=5 value=1.20000005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=6 value=1.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=7 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=8 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=9 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=10 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=11 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=12 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=13 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=14 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=15 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=16 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=17 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=18 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=19 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=20 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=21 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=22 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=23 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=24 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=25 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=26 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=27 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=28 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=29 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
```

## WeaponDefinition:Weapon.M9

Asset: /Game/FPSDemo/Weapons/Definitions/DA_M9.DA_M9

```text
WeaponId=Weapon.M9
DisplayName=NSLOCTEXT("", "93103CB54CED7079684A0884870B0617", "M9")
PlatformId=None
MountIds=
AvailableAttachments=
DefaultAttachments=
Slot=Secondary
Category=Pistol
MagazineSize=15
InitialMagazine=15
InitialReserve=60
Damage=25.000000
HitDamageProfile=/Game/FPSDemo/Weapons/Damage/DA_HitDamage_Default.DA_HitDamage_Default
FireInterval=0.200000
Range=5000.000000
FireMode=Single
BurstCount=3
BurstInterval=0.300000
ReloadDuration=1.400000
InspectDuration=2.000000
EquipReadyTime=0.350000
EquipBlendTime=0.350000
AimTransitionTime=0.200000
bNoRecoil=False
RecoilCurve=/Game/FPSDemo/Weapons/Curves/CR_M9.CR_M9
AimRecoilMultiplier=0.650000
RecoilRecoveryTime=0.500000
FirstPerson=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_M9.SM_IND_M9",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=0.000000,Y=0.000000,Z=0.000000),Scale3D=(X=1.000000,Y=1.000000,Z=1.000000)))
ThirdPerson=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_M9.SM_IND_M9",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=39.000000,Y=8.000000,Z=17.000000),Scale3D=(X=1.000000,Y=1.000000,Z=1.000000)))
Pickup=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_M9.SM_IND_M9",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=0.000000,Y=0.000000,Z=0.000000),Scale3D=(X=0.650000,Y=0.650000,Z=0.650000)))
ModularBody=None
ModularReloadBody=None
WorkbenchBody=None
BuiltinParts=()
WorkbenchParts=()
AttachmentMountLocations=()
HipLocation=(X=32.000000,Y=12.000000,Z=-14.000000)
AimLocation=(X=24.000000,Y=0.000000,Z=-5.100000)
MuzzleLocation=(X=20.350000,Y=0.000000,Z=0.000000)
AimFOV=72.000000
FireSound=/Game/FPSDemo/Audio/S_Fire_M9.S_Fire_M9
ReloadStartSound=/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart
ReloadEndSound=/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd
EquipSound=/Game/FPSDemo/Audio/S_Equip.S_Equip
InspectSound=/Game/FPSDemo/Audio/S_Inspect.S_Inspect
EmptySound=/Game/FPSDemo/Audio/S_Empty.S_Empty
SavedResourceVersions=(("/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_M9.SM_IND_M9", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;39a1d8cbe8c20d87c443f97a75b603d6ef6880fe;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;6388190158b6769093be7df2b02bb7f8a71f868a;ae8a1201239db0265f2023e3301b444f03e2930f;b7fdb68f088695b234844a8e0458c4d9311cfc0f;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;e6d4b7733e23f90d9b1b3e57cc0fbfe1661b7c7a;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Audio/S_Fire_M9.S_Fire_M9", "71ddc0ec25e4cada30661e06bd0ead653e395139"),("/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart", "bce5feebf15a486663e6668b339db78111ed9198"),("/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd", "4dc92cf128570379d6c8ef4fd0964ecd4dc11056"),("/Game/FPSDemo/Audio/S_Equip.S_Equip", "9c7ef98bc8e56e0b8935942375a6fcf34aa02a8e"),("/Game/FPSDemo/Audio/S_Inspect.S_Inspect", "63d3ea395754e83b054bcdff26476f4604791099"),("/Game/FPSDemo/Audio/S_Empty.S_Empty", "36f6cfd85a175c41a5a0360fb5f2026111f953cb"))
HitDamageProfileId=HitDamage.Default Head=2 Torso=1 Limb=0.75
Curve[0] time=0 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=1 value=0.935000062 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=2 value=1.0200001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=3 value=1.10500002 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=4 value=1.19000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=5 value=1.2750001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=6 value=1.36000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=7 value=1.44500005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=8 value=1.44500005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=9 value=1.44500005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=10 value=1.44500005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=11 value=1.44500005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=12 value=1.44500005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=13 value=1.44500005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=14 value=1.44500005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=0 value=0.187000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=1 value=-0.255000025 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=2 value=0.187000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=3 value=-0.255000025 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=4 value=0.187000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=5 value=-0.255000025 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=6 value=0.187000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=7 value=-0.255000025 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=8 value=-0.255000025 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=9 value=-0.255000025 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=10 value=-0.255000025 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=11 value=-0.255000025 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=12 value=-0.255000025 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=13 value=-0.255000025 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=14 value=-0.255000025 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=0 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=1 value=1.03999996 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=2 value=1.08000004 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=3 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=4 value=1.15999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=5 value=1.20000005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=6 value=1.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=7 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=8 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=9 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=10 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=11 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=12 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=13 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=14 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
```

## WeaponDefinition:Weapon.MP5

Asset: /Game/FPSDemo/Weapons/Definitions/DA_MP5.DA_MP5

```text
WeaponId=Weapon.MP5
DisplayName=NSLOCTEXT("", "10CA92914646903818BD84B6CFB72D4E", "MP5")
PlatformId=None
MountIds=
AvailableAttachments=
DefaultAttachments=
Slot=Primary
Category=SMG
MagazineSize=30
InitialMagazine=30
InitialReserve=150
Damage=20.000000
HitDamageProfile=/Game/FPSDemo/Weapons/Damage/DA_HitDamage_Default.DA_HitDamage_Default
FireInterval=0.065000
Range=5000.000000
FireMode=Automatic
BurstCount=3
BurstInterval=0.300000
ReloadDuration=1.800000
InspectDuration=2.000000
EquipReadyTime=0.350000
EquipBlendTime=0.350000
AimTransitionTime=0.200000
bNoRecoil=False
RecoilCurve=/Game/FPSDemo/Weapons/Curves/CR_MP5.CR_MP5
AimRecoilMultiplier=0.650000
RecoilRecoveryTime=0.500000
FirstPerson=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_MP5.SM_IND_MP5",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=0.000000,Y=0.000000,Z=0.000000),Scale3D=(X=1.000000,Y=1.000000,Z=1.000000)))
ThirdPerson=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_MP5.SM_IND_MP5",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=43.000000,Y=8.000000,Z=17.000000),Scale3D=(X=1.000000,Y=1.000000,Z=1.000000)))
Pickup=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_MP5.SM_IND_MP5",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=0.000000,Y=0.000000,Z=0.000000),Scale3D=(X=0.650000,Y=0.650000,Z=0.650000)))
ModularBody=None
ModularReloadBody=None
WorkbenchBody=None
BuiltinParts=()
WorkbenchParts=()
AttachmentMountLocations=()
HipLocation=(X=42.000000,Y=18.000000,Z=-18.000000)
AimLocation=(X=38.000000,Y=0.000000,Z=-6.000000)
MuzzleLocation=(X=38.349998,Y=0.000000,Z=0.000000)
AimFOV=72.000000
FireSound=/Game/FPSDemo/Audio/S_Fire_MP5.S_Fire_MP5
ReloadStartSound=/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart
ReloadEndSound=/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd
EquipSound=/Game/FPSDemo/Audio/S_Equip.S_Equip
InspectSound=/Game/FPSDemo/Audio/S_Inspect.S_Inspect
EmptySound=/Game/FPSDemo/Audio/S_Empty.S_Empty
SavedResourceVersions=(("/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_MP5.SM_IND_MP5", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;20e2868ecc74966d0035bfaef91e6e6a06079e26;273c861a0f4796047bb8cba8b469ecab4a68a4d5;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;6158f2f7f7a7566f82fc5755879ddcf322ce02f6;6e16f36697c4fa728f0a658ae68a709000e0ef24;b7fdb68f088695b234844a8e0458c4d9311cfc0f;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;dbf4b90a76aec94aaeb514dc879b9af6f377b926;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Audio/S_Fire_MP5.S_Fire_MP5", "a21bfaafa021d0581c1a4048ee9904c35c5b2577"),("/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart", "bce5feebf15a486663e6668b339db78111ed9198"),("/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd", "4dc92cf128570379d6c8ef4fd0964ecd4dc11056"),("/Game/FPSDemo/Audio/S_Equip.S_Equip", "9c7ef98bc8e56e0b8935942375a6fcf34aa02a8e"),("/Game/FPSDemo/Audio/S_Inspect.S_Inspect", "63d3ea395754e83b054bcdff26476f4604791099"),("/Game/FPSDemo/Audio/S_Empty.S_Empty", "36f6cfd85a175c41a5a0360fb5f2026111f953cb"))
HitDamageProfileId=HitDamage.Default Head=2 Torso=1 Limb=0.75
Curve[0] time=0 value=0.5 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=1 value=0.550000012 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=2 value=0.600000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=3 value=0.649999976 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=4 value=0.699999988 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=5 value=0.75 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=6 value=0.800000012 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=7 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=8 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=9 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=10 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=11 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=12 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=13 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=14 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=15 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=16 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=17 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=18 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=19 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=20 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=21 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=22 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=23 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=24 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=25 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=26 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=27 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=28 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=29 value=0.850000024 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=0 value=0.109999999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=1 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=2 value=0.109999999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=3 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=4 value=0.109999999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=5 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=6 value=0.109999999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=7 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=8 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=9 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=10 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=11 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=12 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=13 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=14 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=15 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=16 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=17 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=18 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=19 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=20 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=21 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=22 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=23 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=24 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=25 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=26 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=27 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=28 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=29 value=-0.150000006 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=0 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=1 value=1.03999996 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=2 value=1.08000004 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=3 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=4 value=1.15999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=5 value=1.20000005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=6 value=1.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=7 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=8 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=9 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=10 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=11 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=12 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=13 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=14 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=15 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=16 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=17 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=18 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=19 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=20 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=21 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=22 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=23 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=24 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=25 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=26 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=27 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=28 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=29 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
```

## WeaponDefinition:Weapon.UMP45

Asset: /Game/FPSDemo/Weapons/Definitions/DA_UMP45.DA_UMP45

```text
WeaponId=Weapon.UMP45
DisplayName=NSLOCTEXT("", "49C944B24AA2EAB2ADEFFCB31F71C34D", "UMP45")
PlatformId=None
MountIds=
AvailableAttachments=
DefaultAttachments=
Slot=Primary
Category=SMG
MagazineSize=25
InitialMagazine=25
InitialReserve=125
Damage=29.000000
HitDamageProfile=/Game/FPSDemo/Weapons/Damage/DA_HitDamage_Default.DA_HitDamage_Default
FireInterval=0.105000
Range=5000.000000
FireMode=Automatic
BurstCount=3
BurstInterval=0.300000
ReloadDuration=2.200000
InspectDuration=2.000000
EquipReadyTime=0.350000
EquipBlendTime=0.350000
AimTransitionTime=0.200000
bNoRecoil=False
RecoilCurve=/Game/FPSDemo/Weapons/Curves/CR_UMP45.CR_UMP45
AimRecoilMultiplier=0.650000
RecoilRecoveryTime=0.500000
FirstPerson=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_UMP45.SM_IND_UMP45",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=0.000000,Y=0.000000,Z=0.000000),Scale3D=(X=1.000000,Y=1.000000,Z=1.000000)))
ThirdPerson=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_UMP45.SM_IND_UMP45",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=43.000000,Y=8.000000,Z=17.000000),Scale3D=(X=1.000000,Y=1.000000,Z=1.000000)))
Pickup=(Mesh="/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_UMP45.SM_IND_UMP45",Transform=(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=1.000000),Translation=(X=0.000000,Y=0.000000,Z=0.000000),Scale3D=(X=0.650000,Y=0.650000,Z=0.650000)))
ModularBody=None
ModularReloadBody=None
WorkbenchBody=None
BuiltinParts=()
WorkbenchParts=()
AttachmentMountLocations=()
HipLocation=(X=42.000000,Y=18.000000,Z=-18.000000)
AimLocation=(X=38.000000,Y=0.000000,Z=-6.000000)
MuzzleLocation=(X=38.349998,Y=0.000000,Z=0.000000)
AimFOV=72.000000
FireSound=/Game/FPSDemo/Audio/S_Fire_UMP45.S_Fire_UMP45
ReloadStartSound=/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart
ReloadEndSound=/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd
EquipSound=/Game/FPSDemo/Audio/S_Equip.S_Equip
InspectSound=/Game/FPSDemo/Audio/S_Inspect.S_Inspect
EmptySound=/Game/FPSDemo/Audio/S_Empty.S_Empty
SavedResourceVersions=(("/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_UMP45.SM_IND_UMP45", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;6d5e4113a86006c62e987037f25397015be93f5d;6e16f36697c4fa728f0a658ae68a709000e0ef24;760e75fac41d011389288a5ab8b84e7785f7040e;ae5fe29f1408fb6b80b89e6bd2c1ab71b9cbf129;b7fdb68f088695b234844a8e0458c4d9311cfc0f;bc59340cc9c052cca44a3b2269da19faa8d21ef2;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Audio/S_Fire_UMP45.S_Fire_UMP45", "925fa5ca5a8ffe027a278dafff6ad087755cba69"),("/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart", "bce5feebf15a486663e6668b339db78111ed9198"),("/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd", "4dc92cf128570379d6c8ef4fd0964ecd4dc11056"),("/Game/FPSDemo/Audio/S_Equip.S_Equip", "9c7ef98bc8e56e0b8935942375a6fcf34aa02a8e"),("/Game/FPSDemo/Audio/S_Inspect.S_Inspect", "63d3ea395754e83b054bcdff26476f4604791099"),("/Game/FPSDemo/Audio/S_Empty.S_Empty", "36f6cfd85a175c41a5a0360fb5f2026111f953cb"))
HitDamageProfileId=HitDamage.Default Head=2 Torso=1 Limb=0.75
Curve[0] time=0 value=0.899999976 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=1 value=0.99000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=2 value=1.08000004 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=3 value=1.16999996 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=4 value=1.25999999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=5 value=1.3499999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=6 value=1.43999994 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=7 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=8 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=9 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=10 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=11 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=12 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=13 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=14 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=15 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=16 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=17 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=18 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=19 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=20 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=21 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=22 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=23 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[0] time=24 value=1.52999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=0 value=0.197999999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=1 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=2 value=0.197999999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=3 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=4 value=0.197999999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=5 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=6 value=0.197999999 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=7 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=8 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=9 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=10 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=11 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=12 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=13 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=14 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=15 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=16 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=17 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=18 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=19 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=20 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=21 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=22 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=23 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[1] time=24 value=-0.270000011 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=0 value=1 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=1 value=1.03999996 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=2 value=1.08000004 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=3 value=1.12 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=4 value=1.15999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=5 value=1.20000005 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=6 value=1.24000001 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=7 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=8 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=9 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=10 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=11 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=12 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=13 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=14 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=15 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=16 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=17 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=18 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=19 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=20 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=21 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=22 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=23 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
Curve[2] time=24 value=1.27999997 interp=0 tangentMode=0 arrive=0 leave=0 weightMode=0 arriveWeight=0 leaveWeight=0
```

## /Game/FPSDemo/Maps/LV_FPSFoundry
- FPSWeapon1 | WeaponDefinition:Weapon.AK | order=0 | -2280.000000,-1050.000000,75.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_AK.DA_AK
- FPSWeapon1 | WeaponDefinition:Weapon.AK | order=0 | 2280.000000,-1050.000000,75.000000|0.000000,180.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_AK.DA_AK
- FPSWeapon2 | WeaponDefinition:Weapon.M4 | order=1 | -2280.000000,-630.000000,75.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_M4.DA_M4
- FPSWeapon2 | WeaponDefinition:Weapon.M4 | order=1 | 2280.000000,-630.000000,75.000000|0.000000,180.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_M4.DA_M4
- FPSWeapon3 | WeaponDefinition:Weapon.MP5 | order=2 | -2280.000000,-210.000000,75.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_MP5.DA_MP5
- FPSWeapon3 | WeaponDefinition:Weapon.MP5 | order=2 | 2280.000000,-210.000000,75.000000|0.000000,180.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_MP5.DA_MP5
- FPSWeapon4 | WeaponDefinition:Weapon.UMP45 | order=3 | -2280.000000,210.000000,75.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_UMP45.DA_UMP45
- FPSWeapon4 | WeaponDefinition:Weapon.UMP45 | order=3 | 2280.000000,210.000000,75.000000|0.000000,180.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_UMP45.DA_UMP45
- FPSWeapon5 | WeaponDefinition:Weapon.M9 | order=4 | -2280.000000,630.000000,75.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_M9.DA_M9
- FPSWeapon5 | WeaponDefinition:Weapon.M9 | order=4 | 2280.000000,630.000000,75.000000|0.000000,180.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_M9.DA_M9
- FPSWeapon6 | WeaponDefinition:Weapon.HeavyPistol | order=5 | -2280.000000,1050.000000,75.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_HeavyPistol.DA_HeavyPistol
- FPSWeapon6 | WeaponDefinition:Weapon.HeavyPistol | order=5 | 2280.000000,1050.000000,75.000000|0.000000,180.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_HeavyPistol.DA_HeavyPistol
- WeaponStand_BR3_0 | WeaponDefinition:Weapon.BR3 | order=6 | -2280.000000,-840.000000,75.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_BR3.DA_BR3
- WeaponStand_BR3_1 | WeaponDefinition:Weapon.BR3 | order=6 | 2280.000000,-840.000000,75.000000|0.000000,180.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_BR3.DA_BR3

## /Game/FPSDemo/Maps/LV_FPSTraining
- FPSWeapon1 | WeaponDefinition:Weapon.AK | order=0 | -2650.000000,-875.000000,75.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_AK.DA_AK
- FPSWeapon2 | WeaponDefinition:Weapon.M4 | order=1 | -2650.000000,-525.000000,75.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_M4.DA_M4
- FPSWeapon3 | WeaponDefinition:Weapon.MP5 | order=2 | -2650.000000,-175.000000,75.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_MP5.DA_MP5
- FPSWeapon4 | WeaponDefinition:Weapon.UMP45 | order=3 | -2650.000000,175.000000,75.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_UMP45.DA_UMP45
- FPSWeapon5 | WeaponDefinition:Weapon.M9 | order=4 | -2650.000000,525.000000,75.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_M9.DA_M9
- FPSWeapon6 | WeaponDefinition:Weapon.HeavyPistol | order=5 | -2650.000000,875.000000,75.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_HeavyPistol.DA_HeavyPistol
- WeaponStand_BR3_0 | WeaponDefinition:Weapon.BR3 | order=6 | -2650.000000,-700.000000,75.000000|0.000000,0.000000,-0.000000|1.000000,1.000000,1.000000 | /Game/FPSDemo/Weapons/Definitions/DA_BR3.DA_BR3
