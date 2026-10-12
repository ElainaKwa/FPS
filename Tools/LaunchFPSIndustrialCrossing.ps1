param([string]$EngineRoot='D:\UE\UE_5.8', [ValidateRange(0,9)][int]$Bots=9, [switch]$Listen)
$ErrorActionPreference='Stop'
$crossingRoot=Split-Path $PSScriptRoot -Parent
$crossingExe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$crossingMode=if($Listen){'LAN'}else{'Solo'}
$crossingURL="/Game/FPSDemo/Maps/Gameplay/IndustrialCrossing/LV_FPSIndustrialCrossing?game=/Script/MCP_Test.FPSMatchMode?Mode=${crossingMode}?Bots=${Bots}"
if($Listen){$crossingURL+='?listen'}
# This launcher explicitly opens an interactive play window.
Start-Process -FilePath $crossingExe -ArgumentList @("`"$crossingRoot/MCP_Test.uproject`"",$crossingURL,'-game','-windowed','-ResX=1600','-ResY=900',
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
