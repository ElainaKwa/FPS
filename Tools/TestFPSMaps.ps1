param([string]$EngineRoot = 'D:\UE\UE_5.8', [string]$PackageRoot = '',
    [string]$PackageExecutable = '', [string]$LogRoot = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
if ($PackageRoot) { $exe = Join-Path $PackageRoot 'MCP_Test/Binaries/Win64/MCP_Test.exe' }
if ($PackageExecutable) { $exe = [System.IO.Path]::GetFullPath($PackageExecutable) }
if (!(Test-Path -LiteralPath $exe)) { throw "Unreal executable missing: $exe" }
if (!$LogRoot) { $LogRoot = Join-Path $root 'Saved\FPSFlowTests' }
$LogRoot = [System.IO.Path]::GetFullPath($LogRoot)
New-Item -ItemType Directory -Path $LogRoot -Force | Out-Null
foreach ($mode in @('Solo','Training')) {
    $map = if ($mode -eq 'Solo') { 'LV_FPSFoundry' } else { 'LV_FPSTraining' }
    $log = Join-Path $LogRoot "Map-$mode.log"
    $argsForGame = @("`"$root\MCP_Test.uproject`"", "/Game/FPSDemo/Maps/${map}?game=/Script/MCP_Test.FPSMatchMode?Mode=${mode}?Bots=9", '-game', '-RenderOffscreen', '-windowed', '-ResX=1280', '-ResY=720', '-nosound', '-unattended', '-nosplash', '-FPSMapSmoke', '-ExecCmds="t.MaxFPS 60"', "-abslog=`"$log`"")
    if ($PackageRoot -or $PackageExecutable) {
        $argsForGame = @($argsForGame | Select-Object -Skip 1) + "-UserDir=`"$LogRoot/$mode/User`""
    }
    $started = Get-Date
    $game = Start-Process -FilePath $exe -ArgumentList $argsForGame -PassThru -WindowStyle Hidden
    try {
        $deadline = (Get-Date).AddSeconds(180)
        do {
            Start-Sleep -Seconds 1
            $fresh = (Test-Path -LiteralPath $log) -and (Get-Item -LiteralPath $log).LastWriteTime -ge $started
            $result = if ($fresh) { Get-Content -LiteralPath $log -Raw } else { '' }
            $complete = $result -match "FPS_MAP_SMOKE PASS map=$map\b"
            if ($result -match 'FPS_MAP_SMOKE FAIL|Fatal error:|Assertion failed:') { break }
        } while (!$complete -and !$game.HasExited -and (Get-Date) -lt $deadline)
        # Allow the already-requested frame capture to finish after the scene result.
        if ($complete) { Start-Sleep -Seconds 2 }
    } finally { if (!$game.HasExited) { Stop-Process -Id $game.Id } }
    $result = Get-Content -LiteralPath $log -Raw
    Select-String -LiteralPath $log -Pattern 'FPS_MAP_SMOKE'
    if (!$fresh -or !$complete -or $result -match 'FPS_MAP_SMOKE FAIL|Fatal error:|Assertion failed:') { throw "Map failed or incomplete: $mode" }
}
