param([string]$EngineRoot = 'D:\UE\UE_5.8', [int]$Latency = 0, [int]$PacketLoss = 0, [switch]$Training, [switch]$Visual,
    [string]$PackageExecutable = '', [string]$LogRoot = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$launchPrefix = @("`"$root\MCP_Test.uproject`"")
if ($PackageExecutable) { $exe = [System.IO.Path]::GetFullPath($PackageExecutable); $launchPrefix = @() }
if (!(Test-Path -LiteralPath $exe)) { throw "Unreal executable missing: $exe" }
$name = if ($Training) { 'training' } else { "lag$Latency-loss$PacketLoss" }
if ($Visual) { $name += '-visual' }
if (!$LogRoot) { $LogRoot = Join-Path $root 'Saved\FPSLoadoutTests' }
$logs = Join-Path ([System.IO.Path]::GetFullPath($LogRoot)) $name
New-Item -ItemType Directory -Path $logs -Force | Out-Null
$started = Get-Date
$owned = @()
$common = @('-game','-nullrhi','-nosound','-unattended','-nosplash','-FPSLoadoutSmoke','-ExecCmds="t.MaxFPS 60"',
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if ($Latency -gt 0) { $common += "-PktLag=$Latency" }
if ($PacketLoss -gt 0) { $common += "-PktLoss=$PacketLoss" }
if ($PackageExecutable) { $common += "-UserDir=`"$logs/User`"" }
try {
    $serverLog = Join-Path $logs 'server.log'
    $clientLog = Join-Path $logs 'client.log'
    $map = if ($Training) { '/Game/FPSDemo/Maps/LV_FPSTraining?game=/Script/MCP_Test.FPSMatchMode?Mode=Training' } else { '/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=LAN?Bots=0?listen' }
    $serverOptions = $common
    if ($Training -and $Visual) { $serverOptions = @($common | Where-Object { $_ -ne '-nullrhi' }) + @('-RenderOffscreen','-ResX=1280','-ResY=720','-FPSLoadoutVisual') }
    $owned += Start-Process $exe -ArgumentList ($launchPrefix + @($map,'-port=17789',"-abslog=`"$serverLog`"") + $serverOptions) -WindowStyle Hidden -PassThru
    $targets = @($serverLog)
    if (!$Training) {
        Start-Sleep -Seconds 6
        $clientOptions = $common
        if ($Visual) { $clientOptions = @($common | Where-Object { $_ -ne '-nullrhi' }) + @('-RenderOffscreen','-ResX=1280','-ResY=720','-FPSLoadoutVisual') }
        $owned += Start-Process $exe -ArgumentList ($launchPrefix + @('127.0.0.1:17789',"-abslog=`"$clientLog`"") + $clientOptions) -WindowStyle Hidden -PassThru
        $targets += $clientLog
    }
    $deadline = (Get-Date).AddSeconds(75)
    do {
        Start-Sleep -Seconds 1
        $complete = $true
        foreach ($log in $targets) {
            $data = if (Test-Path $log) { Get-Content $log -Raw } else { '' }
            if ($data -match 'Fatal error:|Assertion failed:') { throw "Runtime failure: $log" }
            $fresh = (Test-Path $log) -and (Get-Item $log).LastWriteTime -ge $started
            $complete = $complete -and $fresh -and ($data -match 'FPS_LOADOUT_SMOKE (PASS|FAIL) (SERVER|CLIENT) COMPLETE\b')
        }
    } while (!$complete -and (Get-Date) -lt $deadline)
    foreach ($log in $targets) {
        $data = Get-Content $log -Raw
        Select-String -LiteralPath $log -Pattern 'FPS_LOADOUT_SMOKE'
        if ((Get-Item $log).LastWriteTime -lt $started -or $data -match 'FPS_LOADOUT_SMOKE FAIL' -or
            $data -notmatch 'FPS_LOADOUT_SMOKE PASS (SERVER|CLIENT) COMPLETE\b') { throw "Failed or incomplete: $log" }
    }
} finally {
    foreach ($game in $owned) { if (!$game.HasExited) { Stop-Process -Id $game.Id -ErrorAction SilentlyContinue } }
}
