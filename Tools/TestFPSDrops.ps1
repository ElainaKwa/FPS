param(
    [string]$EngineRoot = 'D:\UE\UE_5.8',
    [ValidateRange(0, 60000)][int]$Latency = 0,
    [ValidateRange(0, 100)][int]$PacketLoss = 0,
    [switch]$Training,
    [switch]$Visual,
    [switch]$Lifetime,
    [switch]$Observer,
    [switch]$ContinueAfterFailure,
    [string]$PackageExecutable = '',
    [string]$LogRoot = ''
)
$ErrorActionPreference = 'Stop'
if ($Observer -and ($Training -or $Lifetime)) { throw '-Observer requires the normal LAN test.' }
$root = Split-Path $PSScriptRoot -Parent
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$project = Join-Path $root 'MCP_Test.uproject'
$launchPrefix = @("`"$project`"")
if ($PackageExecutable) { $exe = [System.IO.Path]::GetFullPath($PackageExecutable); $launchPrefix = @() }
if (!(Test-Path -LiteralPath $exe)) { throw "Unreal executable missing: $exe" }
if (!$PackageExecutable -and !(Test-Path -LiteralPath $project)) { throw "Unreal project missing: $project" }
$name = if ($Training) { 'training' } else { "lag$Latency-loss$PacketLoss" }
if ($Visual) { $name += '-visual' }
if ($Lifetime) { $name += '-lifetime' }
if ($Observer) { $name += '-observer' }
if (!$LogRoot) { $LogRoot = Join-Path $root 'Saved\FPSDropTests' }
$logs = Join-Path ([System.IO.Path]::GetFullPath($LogRoot)) $name
New-Item -ItemType Directory -Path $logs -Force | Out-Null
$serverLog = Join-Path $logs 'server.log'
$clientLog = Join-Path $logs 'client.log'
$observerLog = Join-Path $logs 'observer.log'
$runFile = Join-Path $logs 'run.json'
$timeoutSeconds = if ($Lifetime) { 300 } else { 180 }
$started = Get-Date
$owned = @()
$targets = @([pscustomobject]@{ Path = $serverLog; Role = 'SERVER'; Marker = 'FPS_DROP_SMOKE' })
if (!$Training) { $targets += [pscustomobject]@{ Path = $clientLog; Role = 'CLIENT'; Marker = 'FPS_DROP_SMOKE' } }
$observerStarted = $false

# Clear only this run's fixed logs before launch so a previous COMPLETE cannot qualify.
foreach ($target in $targets) { [System.IO.File]::WriteAllText($target.Path, '') }
if ($Observer) { [System.IO.File]::WriteAllText($observerLog, '') }
$common = @('-game', '-nullrhi', '-nosound', '-unattended', '-nosplash', '-FPSDropSmoke',
    '-ExecCmds="t.MaxFPS 60"', "-PktLag=$Latency", "-PktLoss=$PacketLoss",
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if ($Lifetime) { $common += '-FPSDropLifetimeSmoke' }
if ($PackageExecutable) { $common += "-UserDir=`"$logs/User`"" }
$map = if ($Training) {
    '/Game/FPSDemo/Maps/LV_FPSTraining?game=/Script/MCP_Test.FPSMatchMode?Mode=Training'
} else {
    '/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=LAN?Bots=0?listen'
}
$serverOptions = $common
if ($Training -and $Visual) {
    $serverOptions = @($common | Where-Object { $_ -ne '-nullrhi' }) +
        @('-RenderOffscreen', '-ResX=1280', '-ResY=720', '-FPSDropVisual')
}
$serverArguments = $launchPrefix + @($map, '-port=17793', "-abslog=`"$serverLog`"") + $serverOptions
$clientArguments = @()
if (!$Training) {
    $clientOptions = $common
    if ($Visual) {
        $clientOptions = @($common | Where-Object { $_ -ne '-nullrhi' }) +
            @('-RenderOffscreen', '-ResX=1280', '-ResY=720', '-FPSDropVisual')
    }
    $clientArguments = $launchPrefix + @('127.0.0.1:17793', "-abslog=`"$clientLog`"") + $clientOptions
}
$record = [ordered]@{
    started = $started.ToString('o')
    engine_executable = $exe
    executable_sha256 = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant()
    packaged = [bool]$PackageExecutable
    project = $project
    mode = $(if ($Training) { 'Training' } else { 'LAN' })
    map = $map
    port = 17793
    bots = $(if ($Training) { $null } else { 0 })
    latency_ms = $Latency
    packet_loss_percent = $PacketLoss
    visual = [bool]$Visual
    lifetime = [bool]$Lifetime
    observer = [bool]$Observer
    continue_after_failure = [bool]$ContinueAfterFailure
    timeout_seconds = $timeoutSeconds
    server_arguments = $serverArguments
    client_arguments = $clientArguments
    observer_arguments = @()
    process_ids = @()
    result = 'RUNNING'
}
$record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $runFile -Encoding UTF8
try {
    Write-Output "FPS_DROP_TEST START mode=$($record.mode) lag_ms=$Latency loss_percent=$PacketLoss lifetime=$([bool]$Lifetime) observer=$([bool]$Observer) visual=$([bool]$Visual) timeout_seconds=$timeoutSeconds logs=$logs"
    $deadline = $started.AddSeconds($timeoutSeconds)
    $owned += Start-Process -FilePath $exe -ArgumentList $serverArguments -WindowStyle Hidden -PassThru
    if (!$Training) {
        Start-Sleep -Seconds 6
        $owned += Start-Process -FilePath $exe -ArgumentList $clientArguments -WindowStyle Hidden -PassThru
    }
    $record.process_ids = @($owned | ForEach-Object { $_.Id })
    $record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $runFile -Encoding UTF8
    do {
        Start-Sleep -Seconds 1
        if ($Observer -and !$observerStarted -and (Test-Path -LiteralPath $serverLog)) {
            $serverData = Get-Content -LiteralPath $serverLog -Raw
            if ((Get-Item -LiteralPath $serverLog).LastWriteTime -ge $started -and
                $serverData -match 'FPS_DROP_SMOKE PASS SERVER late-observer resting fixture ready') {
                $observerArguments = $launchPrefix + @('127.0.0.1:17793', "-abslog=`"$observerLog`"") + $common + '-FPSDropObserver'
                $owned += Start-Process -FilePath $exe -ArgumentList $observerArguments -WindowStyle Hidden -PassThru
                $targets += [pscustomobject]@{ Path = $observerLog; Role = 'CLIENT'; Marker = 'FPS_DROP_OBSERVER' }
                $record.observer_arguments = $observerArguments
                $record.process_ids = @($owned | ForEach-Object { $_.Id })
                $record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $runFile -Encoding UTF8
                $observerStarted = $true
            }
        }
        $complete = $true
        foreach ($target in $targets) {
            $fresh = (Test-Path -LiteralPath $target.Path) -and (Get-Item -LiteralPath $target.Path).LastWriteTime -ge $started
            $data = if ($fresh) { Get-Content -LiteralPath $target.Path -Raw } else { '' }
            if ($data -match 'Fatal error:|Assertion failed:' -or (!$ContinueAfterFailure -and $data -match 'FPS_DROP_(SMOKE|OBSERVER) FAIL')) {
                Select-String -LiteralPath $target.Path -Pattern 'FPS_DROP_(SMOKE|OBSERVER)|Fatal error:|Assertion failed:'
                throw "Runtime failure: $($target.Path)"
            }
            $resultPattern = if ($ContinueAfterFailure) { '(PASS|FAIL)' } else { 'PASS' }
            $complete = $complete -and $fresh -and ($data -match "$($target.Marker) $resultPattern $($target.Role) COMPLETE\b")
        }
        if ($Observer -and !$observerStarted) { $complete = $false }
    } while (!$complete -and (Get-Date) -lt $deadline)
    if ($Observer -and !$observerStarted) { throw 'Late observer readiness marker missing.' }
    foreach ($target in $targets) {
        $fresh = (Test-Path -LiteralPath $target.Path) -and (Get-Item -LiteralPath $target.Path).LastWriteTime -ge $started
        $data = if ($fresh) { Get-Content -LiteralPath $target.Path -Raw } else { '' }
        if ($fresh) { Select-String -LiteralPath $target.Path -Pattern 'FPS_DROP_(SMOKE|OBSERVER)' }
        if (!$fresh -or $data -match 'FPS_DROP_(SMOKE|OBSERVER) FAIL|Fatal error:|Assertion failed:' -or
            $data -notmatch "$($target.Marker) PASS $($target.Role) COMPLETE\b") {
            throw "Failed or incomplete: $($target.Path)"
        }
    }
    $record.result = 'PASS'
    Write-Output "FPS_DROP_TEST PASS mode=$($record.mode) lifetime=$([bool]$Lifetime)"
} catch {
    $record.result = 'FAIL'
    $record.error = $_.Exception.Message
    throw
} finally {
    foreach ($game in $owned) {
        if (!$game.HasExited) { Stop-Process -Id $game.Id -ErrorAction SilentlyContinue }
    }
    $record.finished = (Get-Date).ToString('o')
    $record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $runFile -Encoding UTF8
}
