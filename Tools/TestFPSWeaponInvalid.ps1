param([string]$EngineRoot = 'D:\UE\UE_5.8')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\FPSFlowTests\WeaponInvalid.log'
$runStarted = Get-Date
$game = Start-Process -FilePath $exe -ArgumentList @("`"$root\MCP_Test.uproject`"", '/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=Solo?Bots=3', '-game', '-RenderOffscreen', '-ResX=1280', '-ResY=720', '-nosound', '-unattended', '-nosplash', '-FPSWeaponInvalidDataTest', '-ExecCmds="t.MaxFPS 30"', "-abslog=`"$log`"") -PassThru -WindowStyle Hidden
try {
    $deadline = (Get-Date).AddSeconds(90)
    do {
        Start-Sleep -Seconds 1
        $ready = (Test-Path $log) -and (Get-Item $log).LastWriteTime -ge $runStarted -and ((Get-Content $log -Raw) -match 'FPS_WEAPON_INVALID (PASS|FAIL)')
    } while (!$ready -and !$game.HasExited -and (Get-Date) -lt $deadline)
    if ($ready) { Start-Sleep -Seconds 2 }
} finally { if (!$game.HasExited) { Stop-Process -Id $game.Id } }
$result = Get-Content -LiteralPath $log -Raw
if ($result -notmatch 'FPS_WEAPON_INVALID PASS pawns=0 pickups=0 combat=0' -or $result -notmatch 'MATCH BLOCKED' -or $result -match 'FPS_WEAPON_INVALID FAIL|Fatal error:|Assertion failed:') { throw 'Invalid data runtime gate failed' }
Select-String -LiteralPath $log -Pattern 'FPS_WEAPON_INVALID|MATCH BLOCKED'
