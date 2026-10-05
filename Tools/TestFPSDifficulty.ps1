param([string]$EngineRoot = 'D:\UE\UE_5.8')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
foreach ($case in @(@('Easy','Solo','0.5'),@('Normal','Solo','0.65'),@('Hard','Solo','0.85'),@('Hard','Training','1.0'))) {
    $log = Join-Path $root ("Saved\FPSFlowTests\Difficulty-{0}-{1}.log" -f $case[0],$case[1])
    $arguments = @("`"$root\MCP_Test.uproject`"", "/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=$($case[1])?Bots=2?Difficulty=$($case[0])", '-game', '-nullrhi', '-nosound', '-unattended', '-nosplash', '-FPSDifficultySmoke', "-ExpectedMultiplier=$($case[2])", '-ExecCmds="t.MaxFPS 60"', '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False', "-abslog=`"$log`"")
    $started = Get-Date
    $game = Start-Process -FilePath $exe -ArgumentList $arguments -PassThru -WindowStyle Hidden
    try {
        do {
            Start-Sleep -Seconds 1
            $complete = (Test-Path $log) -and (Get-Item $log).LastWriteTime -ge $started -and (Get-Content $log -Raw) -match 'FPS_DIFFICULTY_SMOKE (PASS|FAIL) COMPLETE'
        } while (!$complete -and !$game.HasExited -and ((Get-Date)-$started).TotalSeconds -lt 60)
    } finally { if (!$game.HasExited) { Stop-Process -Id $game.Id } }
    $result = Get-Content $log -Raw
    Select-String -LiteralPath $log -Pattern 'FPS_DIFFICULTY_SMOKE'
    if ($result -notmatch 'FPS_DIFFICULTY_SMOKE PASS COMPLETE' -or $result -match 'FPS_DIFFICULTY_SMOKE FAIL|Fatal error:|Assertion failed:') { throw "Difficulty test failed: $log" }
}
