param([string]$EngineRoot='D:\UE\UE_5.8', [string]$Profile='Player')
$ErrorActionPreference='Stop'
if($Profile -notmatch '^[A-Za-z0-9_-]{1,64}$'){throw 'Profile must use 1-64 letters, numbers, underscores or hyphens.'}
$labRoot=Split-Path $PSScriptRoot -Parent
$labExe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$labURL='/Game/FPSDemo/Maps/Gameplay/FactoryBase/LV_FPSFactoryBase?game=/Script/MCP_Test.FPSFactoryMode?Mode=Solo?Bots=0?LabPhase=Base'
# Explicit interactive play launcher. Automated tests use hidden processes separately.
Start-Process -FilePath $labExe -ArgumentList @("`"$labRoot/MCP_Test.uproject`"",$labURL,'-game','-windowed','-ResX=1600','-ResY=900',"-ExtractionProfile=$Profile",
    '-FactoryLobby','-DDC=InstalledNoZenLocalFallback','-NoZenAutoLaunch','-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
