#pragma once
class AFPSExtractionMode;
struct FFPSExtractionSmoke
{
    static void Tick(AFPSExtractionMode* Mode);
    static void CombatTick(AFPSExtractionMode* Mode);
    static void CaptureTick(AFPSExtractionMode* Mode);
    static bool GridChecks(class UFPSExtractionSubsystem* Loop);
};
