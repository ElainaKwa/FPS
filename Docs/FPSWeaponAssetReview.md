# Weapon asset review (GENERATED - DO NOT EDIT)

- 用途：自动生成资料
- 适用范围：已保存武器定义与地图武器台
- 状态：只读导出，禁止手工修改
- 最后更新：2026-10-12

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
SavedResourceVersions=(("/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_AK.SM_IND_AK", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;6e16f36697c4fa728f0a658ae68a709000e0ef24;b7fdb68f088695b234844a8e0458c4d9311cfc0f;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31;ffc3ab88f191395215bac41b808e3fe0e864ce78"),("/Game/FPSDemo/Audio/S_Fire_AK.S_Fire_AK", "ae521ce701062a790cee1e35be9f2984295b3328"),("/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart", "bce5feebf15a486663e6668b339db78111ed9198"),("/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd", "4dc92cf128570379d6c8ef4fd0964ecd4dc11056"),("/Game/FPSDemo/Audio/S_Equip.S_Equip", "9c7ef98bc8e56e0b8935942375a6fcf34aa02a8e"),("/Game/FPSDemo/Audio/S_Inspect.S_Inspect", "63d3ea395754e83b054bcdff26476f4604791099"),("/Game/FPSDemo/Audio/S_Empty.S_Empty", "36f6cfd85a175c41a5a0360fb5f2026111f953cb"))
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
SavedResourceVersions=(("/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_BR3.SM_IND_BR3", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;3b9d98757922f960145469d196375df86f3a13d0;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;6e16f36697c4fa728f0a658ae68a709000e0ef24;b7fdb68f088695b234844a8e0458c4d9311cfc0f;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Audio/S_Fire_M4.S_Fire_M4", "c573d556ac962fd89151d8e56a222fda49d20ee3"),("/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart", "bce5feebf15a486663e6668b339db78111ed9198"),("/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd", "4dc92cf128570379d6c8ef4fd0964ecd4dc11056"),("/Game/FPSDemo/Audio/S_Equip.S_Equip", "9c7ef98bc8e56e0b8935942375a6fcf34aa02a8e"),("/Game/FPSDemo/Audio/S_Inspect.S_Inspect", "63d3ea395754e83b054bcdff26476f4604791099"),("/Game/FPSDemo/Audio/S_Empty.S_Empty", "36f6cfd85a175c41a5a0360fb5f2026111f953cb"))
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
SavedResourceVersions=(("/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_HeavyPistol.SM_IND_HeavyPistol", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;ab7c24095358357aa49169d2ba85805878edc12b;b7fdb68f088695b234844a8e0458c4d9311cfc0f;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Audio/S_Fire_HeavyPistol.S_Fire_HeavyPistol", "5708873d4b060116c449b3b460cb14722330012f"),("/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart", "bce5feebf15a486663e6668b339db78111ed9198"),("/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd", "4dc92cf128570379d6c8ef4fd0964ecd4dc11056"),("/Game/FPSDemo/Audio/S_Equip.S_Equip", "9c7ef98bc8e56e0b8935942375a6fcf34aa02a8e"),("/Game/FPSDemo/Audio/S_Inspect.S_Inspect", "63d3ea395754e83b054bcdff26476f4604791099"),("/Game/FPSDemo/Audio/S_Empty.S_Empty", "36f6cfd85a175c41a5a0360fb5f2026111f953cb"))
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
MountIds=("Mount.Optic","Mount.Muzzle","Mount.Magazine")
AvailableAttachments=("/Game/FPSDemo/Weapons/Attachments/DA_M4_OpticStandard.DA_M4_OpticStandard","/Game/FPSDemo/Weapons/Attachments/DA_M4_OpticFixed.DA_M4_OpticFixed","/Game/FPSDemo/Weapons/Attachments/DA_M4_MuzzleStandard.DA_M4_MuzzleStandard","/Game/FPSDemo/Weapons/Attachments/DA_M4_MuzzleBrake.DA_M4_MuzzleBrake","/Game/FPSDemo/Weapons/Attachments/DA_M4_MagazineStandard.DA_M4_MagazineStandard","/Game/FPSDemo/Weapons/Attachments/DA_M4_MagazineExtended.DA_M4_MagazineExtended")
DefaultAttachments=
Slot=Primary
Category=Rifle
MagazineSize=30
InitialMagazine=30
InitialReserve=120
Damage=25.000000
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
SavedResourceVersions=(("/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_M4.SM_IND_M4", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;6e16f36697c4fa728f0a658ae68a709000e0ef24;b7fdb68f088695b234844a8e0458c4d9311cfc0f;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;e8efd383bbef821417bd58e9fa5cac9d3462a1b5;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Audio/S_Fire_M4.S_Fire_M4", "c573d556ac962fd89151d8e56a222fda49d20ee3"),("/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart", "bce5feebf15a486663e6668b339db78111ed9198"),("/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd", "4dc92cf128570379d6c8ef4fd0964ecd4dc11056"),("/Game/FPSDemo/Audio/S_Equip.S_Equip", "9c7ef98bc8e56e0b8935942375a6fcf34aa02a8e"),("/Game/FPSDemo/Audio/S_Inspect.S_Inspect", "63d3ea395754e83b054bcdff26476f4604791099"),("/Game/FPSDemo/Audio/S_Empty.S_Empty", "36f6cfd85a175c41a5a0360fb5f2026111f953cb"))
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
SavedResourceVersions=(("/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_M9.SM_IND_M9", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;b7fdb68f088695b234844a8e0458c4d9311cfc0f;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fd8bd63a1190e22b147ca4ba8d163035d1c569be;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Audio/S_Fire_M9.S_Fire_M9", "71ddc0ec25e4cada30661e06bd0ead653e395139"),("/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart", "bce5feebf15a486663e6668b339db78111ed9198"),("/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd", "4dc92cf128570379d6c8ef4fd0964ecd4dc11056"),("/Game/FPSDemo/Audio/S_Equip.S_Equip", "9c7ef98bc8e56e0b8935942375a6fcf34aa02a8e"),("/Game/FPSDemo/Audio/S_Inspect.S_Inspect", "63d3ea395754e83b054bcdff26476f4604791099"),("/Game/FPSDemo/Audio/S_Empty.S_Empty", "36f6cfd85a175c41a5a0360fb5f2026111f953cb"))
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
SavedResourceVersions=(("/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_MP5.SM_IND_MP5", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;320755844bf4b292b70059f8054e20f24f10f3bd;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;6e16f36697c4fa728f0a658ae68a709000e0ef24;b7fdb68f088695b234844a8e0458c4d9311cfc0f;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Audio/S_Fire_MP5.S_Fire_MP5", "a21bfaafa021d0581c1a4048ee9904c35c5b2577"),("/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart", "bce5feebf15a486663e6668b339db78111ed9198"),("/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd", "4dc92cf128570379d6c8ef4fd0964ecd4dc11056"),("/Game/FPSDemo/Audio/S_Equip.S_Equip", "9c7ef98bc8e56e0b8935942375a6fcf34aa02a8e"),("/Game/FPSDemo/Audio/S_Inspect.S_Inspect", "63d3ea395754e83b054bcdff26476f4604791099"),("/Game/FPSDemo/Audio/S_Empty.S_Empty", "36f6cfd85a175c41a5a0360fb5f2026111f953cb"))
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
SavedResourceVersions=(("/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_UMP45.SM_IND_UMP45", "17d8b3f132e839cd9a6579aa38d8d5364212aae8;3cddfb732e2351cb10188eadb59d17cdaee6fbc7;65630b187e5af9fa9c76f8d8a3013e3715225c01;6e16f36697c4fa728f0a658ae68a709000e0ef24;b7fdb68f088695b234844a8e0458c4d9311cfc0f;d310a4dc2655aa6f4ee4d55876b7a4051b1c8ad9;ed16d2410d0ca59c2d27f3cbb774f23790eec6f3;fe44e162f2c4cd4d0e47460c6ca51b4579ab4b31"),("/Game/FPSDemo/Audio/S_Fire_UMP45.S_Fire_UMP45", "925fa5ca5a8ffe027a278dafff6ad087755cba69"),("/Game/FPSDemo/Audio/S_ReloadStart.S_ReloadStart", "bce5feebf15a486663e6668b339db78111ed9198"),("/Game/FPSDemo/Audio/S_ReloadEnd.S_ReloadEnd", "4dc92cf128570379d6c8ef4fd0964ecd4dc11056"),("/Game/FPSDemo/Audio/S_Equip.S_Equip", "9c7ef98bc8e56e0b8935942375a6fcf34aa02a8e"),("/Game/FPSDemo/Audio/S_Inspect.S_Inspect", "63d3ea395754e83b054bcdff26476f4604791099"),("/Game/FPSDemo/Audio/S_Empty.S_Empty", "36f6cfd85a175c41a5a0360fb5f2026111f953cb"))
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
