param([string]$EngineRoot='D:\UE\UE_5.8')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$script=Join-Path $root 'ArtSource/FPSDemo/Weapons/IndustrialSet/Reload/Scripts/import_reload_parts.py'
$script=$script.Replace('\','/')
$log=Join-Path $root 'Saved/FPSAnimationTests/ReloadImport.log'
New-Item -ItemType Directory -Path (Split-Path $log -Parent) -Force | Out-Null
$prior=[Environment]::GetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP','Process')
try {
    [Environment]::SetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP','1','Process')
    & $editor (Join-Path $root 'MCP_Test.uproject') '-run=pythonscript' "-script=$script" '-unattended' '-nop4' '-nullrhi' '-nosound' `
        '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0' `
        '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False' "-abslog=$log" *> (Join-Path $root 'Saved/FPSAnimationTests/ReloadImport-console.log')
    if($LASTEXITCODE -ne 0 -or (Get-Content -LiteralPath $log -Raw) -notmatch 'FPS_RELOAD_IMPORT PASS'){throw "Reload import failed: $log"}
    $resave=Join-Path (Split-Path $script -Parent) 'resave_reload_definitions.py'
    $resave=$resave.Replace('\','/')
    & $editor (Join-Path $root 'MCP_Test.uproject') '-run=pythonscript' "-script=$resave" '-unattended' '-nop4' '-nullrhi' '-nosound' `
        '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False' `
        "-abslog=$(Join-Path $root 'Saved/FPSAnimationTests/ReloadRevisions.log')" *> (Join-Path $root 'Saved/FPSAnimationTests/ReloadRevisions-console.log')
    if($LASTEXITCODE -ne 0){throw 'Reload definition revision refresh failed'}
    & (Join-Path $PSScriptRoot 'ValidateFPSWeaponData.ps1') -EngineRoot $EngineRoot -Export *> (Join-Path $root 'Saved/FPSAnimationTests/ReloadContent.log')
    if(!$?){throw 'Weapon content revalidation failed'}
    Write-Output "FPS_RELOAD_IMPORT PASS log=$log"
} finally { [Environment]::SetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP',$prior,'Process') }
