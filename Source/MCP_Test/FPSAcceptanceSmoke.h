#pragma once
class AFPSFrontController;
class UWorld;
struct FFPSAcceptanceSmoke
{
    static bool ReadyToStart(UWorld* World);
    static void Tick(AFPSFrontController* PC, float DeltaSeconds);
};
