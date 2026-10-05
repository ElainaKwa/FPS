param(
    [string]$EngineRoot = 'D:\UE\UE_5.8',
    [switch]$Export,
    [switch]$SelfTest,
    [switch]$Migrate
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$projectFile = Join-Path $projectRoot 'MCP_Test.uproject'
$logFile = Join-Path $projectRoot 'Saved\Logs\FPSWeaponDataValidation.log'
$arguments = @($projectFile, '-run=FPSWeaponData', '-unattended', '-nop4', '-nullrhi', '-nosound', '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False', "-abslog=$logFile")
if ($Export) { $arguments += '-Export' }
if ($SelfTest) { $arguments += '-SelfTest' }
if ($Migrate) { $arguments += '-Migrate' }
& $editorCmd @arguments
if ($LASTEXITCODE -ne 0) { throw "Weapon data validation failed ($LASTEXITCODE): $logFile" }
Select-String -LiteralPath $logFile -Pattern 'FPS_WEAPON_DATA'
