param([string]$EngineRoot = 'D:\UE\UE_5.8')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$log = Join-Path $root 'Saved\Logs\FPSWeaponIdentity.log'
& $editorCmd "$root\MCP_Test.uproject" -run=pythonscript "-script=$PSScriptRoot\test_fps_weapon_identity.py" -unattended -nop4 -SCCProvider=None -nullrhi -nosound '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False' "-abslog=$log"
if ($LASTEXITCODE -ne 0) { throw "Saved weapon identity test failed: $log" }
$remaining = Get-ChildItem -LiteralPath "$root\Content\__FPSWeaponIdentityTest" -Recurse -File -ErrorAction SilentlyContinue
if ($remaining) { throw 'Identity fixture files remain' }
Select-String -LiteralPath $log -Pattern 'FPS_WEAPON_IDENTITY PASS'
