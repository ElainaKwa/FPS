param([string]$EngineRoot = 'D:\UE\UE_5.8')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
& (Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat') MCP_TestEditor Win64 Development "-Project=$projectRoot\MCP_Test.uproject" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw "C++ build failed: $LASTEXITCODE" }
# UBT can skip post-build actions when C++ is already current. Always check asset-only edits too.
& "$PSScriptRoot\ValidateFPSWeaponData.ps1" -EngineRoot $EngineRoot
