#pragma once
class AFPSMatchMode;
class UFPSWeaponComponent;
struct FFPSWeaponSmoke
{
    static void Start(AFPSMatchMode* Mode);
    static void Tick(UFPSWeaponComponent* Weapons);
};
