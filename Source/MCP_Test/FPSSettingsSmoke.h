#pragma once
class AFPSDemoPlayerController;
struct FFPSSettingsSmoke
{
    static void Tick(AFPSDemoPlayerController* PC);
    static void NetworkTick(AFPSDemoPlayerController* PC);
    static void ServerProbe(AFPSDemoPlayerController* PC,int32 Stage);
};
