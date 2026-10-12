#include "FPSInputSettings.h"
#include "Misc/ConfigCacheIni.h"

UFPSInputSettings* UFPSInputSettings::Get() { return GetMutableDefault<UFPSInputSettings>(); }
void UFPSInputSettings::SetSprintToggle(bool bEnabled)
{
    if (bSprintToggle == bEnabled) return;
    bSprintToggle = bEnabled;
    SaveLocal();
}
void UFPSInputSettings::SetCrouchToggle(bool bEnabled)
{
    if (bCrouchToggle == bEnabled) return;
    bCrouchToggle = bEnabled;
    SaveLocal();
}
void UFPSInputSettings::SaveLocal()
{
    if (!GConfig || GGameUserSettingsIni.IsEmpty()) return;
    SaveConfig(CPF_Config, *GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}
