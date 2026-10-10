#pragma once
class AFPSDemoCharacter;
class AFPSMatchMode;
/** Opt-in injection test on actual listen-server/client net drivers. Not a natural gameplay soak. */
struct FFPSCharacterSmoke
{
    static void Start(AFPSMatchMode* Mode);
    static void Tick(AFPSDemoCharacter* Pawn);
};
