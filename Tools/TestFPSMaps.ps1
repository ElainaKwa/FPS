param([string]$EngineRoot = 'D:\UE\UE_5.8')
$root = Split-Path $PSScriptRoot -Parent
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
foreach ($mode in @('Solo','Training')) {
    $map = if ($mode -eq 'Solo') { 'LV_FPSFoundry' } else { 'LV_FPSTraining' }
    $log = Join-Path $root "Saved\FPSFlowTests\Map-$mode.log"
    $argsForGame = @("`"$root\MCP_Test.uproject`"", "/Game/FPSDemo/Maps/${map}?game=/Script/MCP_Test.FPSMatchMode?Mode=${mode}?Bots=9", '-game', '-RenderOffscreen', '-windowed', '-ResX=1280', '-ResY=720', '-nosound', '-unattended', '-nosplash', '-FPSMapSmoke', '-ExecCmds="t.MaxFPS 60"', "-abslog=`"$log`"")
    $game = Start-Process -FilePath $exe -ArgumentList $argsForGame -PassThru -WindowStyle Hidden
    try { Start-Sleep -Seconds 35 } finally { if (!$game.HasExited) { Stop-Process -Id $game.Id } }
    $result = Get-Content -LiteralPath $log -Raw
    Select-String -LiteralPath $log -Pattern 'FPS_MAP_SMOKE'
    if ($result -notmatch 'FPS_MAP_SMOKE PASS' -or $result -match 'FPS_MAP_SMOKE FAIL|Fatal error:|Assertion failed:') { throw "Map failed: $mode" }
}
