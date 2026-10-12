param([string]$EngineRoot = 'D:\UE\UE_5.8', [string]$RunId = (Get-Date -Format 'yyyyMMdd-HHmmssfff'))
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$report = Join-Path $root "Saved/Automation/WeaponPIEExit-$RunId"
if (Test-Path -LiteralPath $report) { throw "Evidence already exists: $report" }
New-Item -ItemType Directory -Path $report | Out-Null
$log = Join-Path $report 'Editor.log'
$started = Get-Date
$game = Start-Process (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe') -ArgumentList @("`"$root/MCP_Test.uproject`"", '-unattended', '-nullrhi', '-nosound', '-nosplash', '-ExecCmds="Automation RunTests MCP.FPS.Weapons.ExitPIE"', '-TestExit="Automation Test Queue Empty"', "-ReportExportPath=`"$report`"", "-abslog=`"$log`"", '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False') -WindowStyle Hidden -PassThru
try {
    do { Start-Sleep -Seconds 2 } while (!$game.HasExited -and ((Get-Date)-$started).TotalSeconds -lt 380)
    if (!$game.HasExited) { throw 'PIE automation timed out' }
    $text = Get-Content $log -Raw
    Select-String -Path $log -Pattern 'PIE_EXIT|Test Completed'
    if ((Get-Item $log).LastWriteTime -lt $started -or $game.ExitCode -ne 0 -or $text -match 'PIE_EXIT timeout|Fatal error:|Assertion failed:' -or ([regex]::Matches($text,'PIE_EXIT PASS')).Count -ne 6 -or $text -notmatch 'Result=\{Success\}.*MCP.FPS.Weapons.ExitPIE') { throw 'PIE teardown regression failed or incomplete' }
} finally {
    if (!$game.HasExited) { Stop-Process -Id $game.Id }
}
