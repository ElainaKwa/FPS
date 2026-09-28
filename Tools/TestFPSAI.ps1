param([string]$EngineRoot = 'D:\UE\UE_5.8', [switch]$Crowd)
$root = Split-Path $PSScriptRoot -Parent
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\FPSFlowTests\AI-server.log'
if ($Crowd) { $log = Join-Path $root 'Saved\FPSFlowTests\AI-crowd.log' }
$arguments = @("`"$root\MCP_Test.uproject`"", '/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=Solo?Bots=1', '-game', '-nullrhi', '-nosound', '-unattended', '-nosplash', '-FPSAISmoke', '-ExecCmds="t.MaxFPS 60"', "-abslog=`"$log`"")
if ($Crowd) { $arguments = $arguments | ForEach-Object { $_.Replace('Bots=1','Bots=9').Replace('-FPSAISmoke','-FPSAIStress') } }
$started = Get-Date
$game = Start-Process -FilePath $exe -ArgumentList $arguments -PassThru -WindowStyle Hidden
try {
    do {
        Start-Sleep -Seconds 1
        $finished = (Test-Path -LiteralPath $log) -and (Get-Item -LiteralPath $log).LastWriteTime -ge $started -and ((Get-Content -LiteralPath $log -Raw) -match 'FPS_AI_SMOKE (PASS|FAIL) COMPLETE')
    } while (!$finished -and !$game.HasExited -and ((Get-Date) - $started).TotalSeconds -lt 90)
} finally { if (!$game.HasExited) { Stop-Process -Id $game.Id } }
$result = Get-Content -LiteralPath $log -Raw
Select-String -LiteralPath $log -Pattern 'FPS_AI_SMOKE|FPS_AI_CROWD'
if ($result -notmatch 'FPS_AI_SMOKE PASS COMPLETE' -or $result -match 'FPS_AI_SMOKE FAIL|Fatal error:|Assertion failed:') { throw 'AI checks failed or incomplete' }
