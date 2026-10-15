param([string]$EngineRoot='D:\UE\UE_5.8',[string]$Profile='Player')
$ErrorActionPreference='Stop'
if($Profile -notmatch '^[A-Za-z0-9_-]{1,64}$'){throw 'Invalid profile name'}
$factoryRoot=Split-Path $PSScriptRoot -Parent
$factoryExe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$factoryMap='/Game/FPSDemo/Maps/Gameplay/FactoryBase/LV_FPSFactoryBase?game=/Script/MCP_Test.FPSFactoryMode?Mode=Solo?Bots=0'
# User-invoked interactive launcher; the isolated test launcher stays hidden.
Start-Process -FilePath $factoryExe -ArgumentList @("`"$factoryRoot/MCP_Test.uproject`"",$factoryMap,'-game','-windowed','-ResX=1600','-ResY=900',"-ExtractionProfile=$Profile",'-DDC=InstalledNoZenLocalFallback','-NoZenAutoLaunch','-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
