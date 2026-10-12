param(
    [string]$EngineRoot = 'D:\UE\UE_5.8',
    [ValidateRange(0, 60000)][int]$Latency = 0,
    [ValidateRange(0, 100)][int]$PacketLoss = 0,
    [switch]$Training,
    [switch]$Teaching,
    [switch]$MismatchMovement,
    [switch]$Visual,
    [string]$PackageExecutable = '',
    [string]$LogRoot = '',
    [ValidateRange(1, 65535)][int]$Port = 17797
)
$ErrorActionPreference = 'Stop'
if ($Training -and $Teaching) { throw '-Training and -Teaching are mutually exclusive.' }
if ($MismatchMovement -and ($Training -or $Teaching)) { throw '-MismatchMovement requires a LAN server and client.' }
$root = Split-Path $PSScriptRoot -Parent
$project = Join-Path $root 'MCP_Test.uproject'
$exe = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
[string[]]$prefix = @("`"$project`"")
if ($PackageExecutable) { $exe = [System.IO.Path]::GetFullPath($PackageExecutable); $prefix = @() }
if (!(Test-Path -LiteralPath $exe)) { throw "Unreal executable missing: $exe" }
if (!$PackageExecutable -and !(Test-Path -LiteralPath $project)) { throw "Project missing: $project" }
if (!$LogRoot) { $LogRoot = Join-Path $root 'Saved/FPSMovementTests' }
$name = if ($Training) { 'training' } elseif ($Teaching) { 'teaching' } else { "lag$Latency-loss$PacketLoss" }
if ($Visual) { $name += '-visual' }
if ($MismatchMovement) { $name += '-mismatch-movement' }
$logs = Join-Path ([System.IO.Path]::GetFullPath($LogRoot)) ((Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '-' + $name)
if (Test-Path -LiteralPath $logs) { throw "Evidence directory already exists: $logs" }
New-Item -ItemType Directory -Path $logs | Out-Null
$serverLog = Join-Path $logs 'server.log'
$clientLog = Join-Path $logs 'client.log'
$runFile = Join-Path $logs 'run.json'
$started = Get-Date
$timeout = if ($Teaching -or $MismatchMovement) { 120 } else { 300 }
$deadline = $null
$common = @('-game', '-nullrhi', '-nosound', '-unattended', '-nosplash', '-FPSMovementSmoke',
    "-PktLag=$Latency", "-PktLoss=$PacketLoss", '-ExecCmds="t.MaxFPS 60,net.EnableNetStats 1"',
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if ($Teaching) { $common += '-FPSMovementTeaching' }
$map = if ($Teaching) { '/Game/Maps/LV_NetSyncFPS?listen' }
elseif ($Training) { '/Game/FPSDemo/Maps/LV_FPSTraining?game=/Script/MCP_Test.FPSMatchMode?Mode=Training' }
else { '/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=LAN?Bots=0?listen' }
$serverOptions = $common
if ($Training -and $Visual) { $serverOptions = @($common | Where-Object { $_ -ne '-nullrhi' }) + @('-RenderOffscreen', '-ResX=1280', '-ResY=720', '-FPSMovementVisual') }
if ($PackageExecutable) { $serverOptions += "-UserDir=`"$logs/User-server`"" }
[string[]]$serverArguments = @($prefix) + @($map, "-port=$Port", "-abslog=`"$serverLog`"") + $serverOptions
$clientOptions = $common
if ($Visual) { $clientOptions = @($common | Where-Object { $_ -ne '-nullrhi' }) + @('-RenderOffscreen', '-ResX=1280', '-ResY=720', '-FPSMovementVisual') }
if ($PackageExecutable) { $clientOptions += "-UserDir=`"$logs/User-client`"" }
if ($MismatchMovement) { $clientOptions += '-ini:Game:[/Script/MCP_Test.FPSMovementSettings]:SprintDrain=7' }
[string[]]$clientArguments = @($prefix) + @("127.0.0.1:$Port", "-abslog=`"$clientLog`"") + $clientOptions
$owned = @()
$targets = @([pscustomobject]@{ path=$serverLog; role='SERVER' })
if (!$Training) { $targets += [pscustomobject]@{ path=$clientLog; role='CLIENT' } }
function Binary([string]$Path) {
    $item = Get-Item -LiteralPath $Path
    return [ordered]@{ path=$item.FullName; sha256=(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant(); bytes=$item.Length; modified_utc=$item.LastWriteTimeUtc.ToString('o'); version=$item.VersionInfo.FileVersion }
}
$module = Join-Path $root 'Binaries/Win64/UnrealEditor-MCP_Test.dll'
$versionFile = Join-Path $EngineRoot 'Engine/Build/Build.version'
$protocol = [int]([regex]::Match((Get-Content -LiteralPath (Join-Path $root 'Source/MCP_Test/FPSWeaponData.h') -Raw), 'ManifestVersion\s*=\s*(\d+)').Groups[1].Value)
$record = [ordered]@{
    started_utc=$started.ToUniversalTime().ToString('o'); completed_utc=$null; result='RUNNING'; fixture='FPSMovementSmoke'
    mode=$(if ($Training) { 'Training' } elseif ($Teaching) { 'Teaching' } else { 'LAN' }); map=$map; bots=$(if ($Teaching -or $Training) { $null } else { 0 })
    executable=(Binary $exe); editor_module=$(if (!$PackageExecutable -and (Test-Path -LiteralPath $module)) { Binary $module } else { $null })
    engine=$(if (Test-Path -LiteralPath $versionFile) { Get-Content -LiteralPath $versionFile -Raw | ConvertFrom-Json } else { $null })
    packaged=[bool]$PackageExecutable; project=$project; port=$Port; latency_ms=$Latency; packet_loss_percent=$PacketLoss; visual=[bool]$Visual
    timeout_seconds=$timeout; startup_timeout_seconds=180; total_bound_seconds=($timeout+180)
    server_world_ready_utc=$null; runtime_environment=@{ UE_SKIP_UBT_SDK_SETUP='1'; scope='Owned child process inheritance only; launcher environment restored immediately.' }
    protocol=$protocol; mismatch_movement=[bool]$MismatchMovement; admission_timeout_seconds=25
    server_arguments=$serverArguments; client_arguments=$(if ($Training) { @() } else { $clientArguments })
    process_records=@(); logs=@(); sources=@(); captures=@(); error=$null
    injection='Replicated opt-in floor/wall/roof and server pose/resource setup; native input/CharacterMovement and actual weapon GAS; native network correction/replay.'
    unverified=@('Hardware keyboard and controller operation', 'Human movement feel and audiovisual judgement', 'Existing-map obstacle traversal is not established by the isolated collision fixture')
}
foreach ($source in @('Source/MCP_Test/FPSMovementSmoke.h', 'Source/MCP_Test/FPSMovementSmoke.cpp', 'Source/MCP_Test/FPSMovementComponent.h', 'Source/MCP_Test/FPSMovementComponent.cpp', 'Source/MCP_Test/FPSMovementSettings.h', 'Tools/TestFPSMovement.ps1')) {
    $file = Join-Path $root $source
    if (Test-Path -LiteralPath $file) { $record.sources += Binary $file }
}
function Save { $record | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $runFile -Encoding UTF8 }
function Fresh([string]$Path) {
    if (!(Test-Path -LiteralPath $Path) -or (Get-Item -LiteralPath $Path).LastWriteTimeUtc -lt $started.ToUniversalTime()) { return '' }
    return Get-Content -LiteralPath $Path -Raw
}
function Launch([string[]]$Arguments, [string]$Role, [string]$Log) {
    $previousSdkEnvironment = [Environment]::GetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP', [System.EnvironmentVariableTarget]::Process)
    try {
        [Environment]::SetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP', '1', [System.EnvironmentVariableTarget]::Process)
        $process = Start-Process -FilePath $exe -ArgumentList $Arguments -WindowStyle Hidden -PassThru
    } finally {
        [Environment]::SetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP', $previousSdkEnvironment, [System.EnvironmentVariableTarget]::Process)
    }
    $script:owned += $process
    $record.process_records += [ordered]@{ pid=$process.Id; role=$Role; log=$Log; arguments=$Arguments; started_utc=(Get-Date).ToUniversalTime().ToString('o'); owned_cleanup=$false; exit_code=$null }
    Save
}
Save
try {
    Write-Output "FPS_MOVEMENT_TEST START mode=$($record.mode) lag_ms=$Latency loss_percent=$PacketLoss visual=$([bool]$Visual) logs=$logs"
    Launch $serverArguments 'SERVER' $serverLog
    $ready = $false
    $startupDeadline = (Get-Date).AddSeconds(180)
    do {
        Start-Sleep -Seconds 1
        $server = Fresh $serverLog
        if ($server -match 'Fatal error[:!]|Assertion failed:|FPS_MOVEMENT_SMOKE FAIL|TravelFailure:') { throw 'Server failed during actual world startup.' }
        if ($owned[0].HasExited) { throw "Server exited during startup: $($owned[0].ExitCode)" }
        $ready = $server -match 'LogLoad: Took .*LoadMap' -and ($Teaching -or $server -match 'FPS_WEAPON_CONTENT READY')
    } while (!$ready -and (Get-Date) -lt $startupDeadline)
    if (!$ready) { throw 'Fresh server LoadMap/content-session startup was not observed.' }
    $record.server_world_ready_utc = (Get-Date).ToUniversalTime().ToString('o')
    $deadline = (Get-Date).AddSeconds($timeout)
    Save
    if (!$Training) { Launch $clientArguments 'CLIENT' $clientLog }
    if ($MismatchMovement) {
        $admissionDeadline = (Get-Date).AddSeconds(25)
        $rejected = $false
        do {
            Start-Sleep -Seconds 1
            $server = Fresh $serverLog; $client = Fresh $clientLog
            if (($server + $client) -match 'Fatal error[:!]|Assertion failed:|FPS_MOVEMENT_SMOKE FAIL') { throw 'Runtime failure during movement content rejection.' }
            $rejected = $server -match 'FPS_WEAPON_CONTENT SERVER REJECT pawn=0 reason=.*mismatch' -and
                $client -match 'FPS_WEAPON_CONTENT CLIENT MISMATCH' -and $client -match 'FPS_WEAPON_CONTENT RETURN' -and
                $client -match 'FPS_WEAPON_CONTENT RELEASE definitions=7 handles=0'
        } while (!$rejected -and (Get-Date) -lt $admissionDeadline)
        if (!$rejected -or $client -match 'FPS_MOVEMENT_SMOKE READY|FPS_WEAPON_CONTENT CLIENT MATCH definitions=7') { throw 'Movement mismatch did not produce rejection, content release and no gameplay start within 25 seconds.' }
        $record.result = 'PASS_ADMISSION_REJECTION'
        $record.admission_markers = @('CLIENT MISMATCH', 'SERVER REJECT pawn=0', 'RETURN', 'RELEASE definitions=7 handles=0')
        Select-String -LiteralPath $serverLog, $clientLog -Pattern 'FPS_WEAPON_CONTENT'
        Write-Output 'FPS_MOVEMENT_TEST PASS ADMISSION REJECTION: different movement tuning cannot enter gameplay.'
        return
    }
    $complete = $false
    do {
        Start-Sleep -Seconds 1
        $complete = $true
        foreach ($target in $targets) {
            $data = Fresh $target.path
            if ($data -match 'FPS_MOVEMENT_SMOKE FAIL|Fatal error[:!]|Assertion failed:|TravelFailure:') { throw "Runtime failure: $($target.path)" }
            $complete = $complete -and ($data -match "FPS_MOVEMENT_SMOKE PASS $($target.role) COMPLETE(?=\s|$)")
        }
        foreach ($process in $owned) { if ($process.HasExited -and !$complete) { throw "Owned process exited before both endpoints completed: $($process.Id)" } }
    } while (!$complete -and (Get-Date) -lt $deadline)
    if (!$complete) { throw "Movement smoke did not finish within $timeout seconds." }
    foreach ($target in $targets) {
        $data = Fresh $target.path
        if (!$data -or $data -match 'FPS_MOVEMENT_SMOKE FAIL|Fatal error[:!]|Assertion failed:|TravelFailure:' -or $data -notmatch "FPS_MOVEMENT_SMOKE PASS $($target.role) COMPLETE(?=\s|$)") { throw "Incomplete final evidence: $($target.path)" }
        if (!$Teaching) {
            $actualNetMode = if ($target.role -eq 'CLIENT') { 3 } elseif ($Training) { 0 } else { 2 }
            if ($data -notmatch "FPS_MOVEMENT_SMOKE READY role=$($target.role) netmode=$actualNetMode rules=1 stamina=1") { throw "Actual role, net mode or enabled rules marker missing: $($target.path)" }
        }
        if (!$Teaching -and !$Training -and $target.role -eq 'CLIENT' -and $data -notmatch 'FPS_MOVEMENT_SMOKE PASS CLIENT native correction replay observed') { throw 'Actual client native movement replay marker missing.' }
        Select-String -LiteralPath $target.path -Pattern 'FPS_MOVEMENT_SMOKE'
    }
    $record.result = 'PASS'
} catch {
    $record.result = 'FAIL'; $record.error = $_.Exception.Message
    foreach ($target in $targets) { if (Test-Path -LiteralPath $target.path) { Select-String -LiteralPath $target.path -Pattern 'FPS_MOVEMENT_SMOKE|Fatal error[:!]|Assertion failed:|TravelFailure:' } }
    throw
} finally {
    foreach ($process in $owned) {
        $process.Refresh()
        if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue; $process.WaitForExit(5000) | Out-Null; $process.Refresh() }
        foreach ($entry in $record.process_records) { if ($entry.pid -eq $process.Id) { $entry.owned_cleanup=$process.HasExited; if ($process.HasExited) { $entry.exit_code=$process.ExitCode } } }
    }
    foreach ($target in $targets) {
        if (Test-Path -LiteralPath $target.path) { $record.logs += [ordered]@{ path=$target.path; role=$target.role; sha256=(Get-FileHash -LiteralPath $target.path -Algorithm SHA256).Hash.ToLowerInvariant(); complete=((Fresh $target.path) -match "FPS_MOVEMENT_SMOKE PASS $($target.role) COMPLETE(?=\s|$)") } }
    }
    if ($record.editor_module -and (Get-FileHash -LiteralPath $record.editor_module.path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $record.editor_module.sha256) {
        $record.result = 'FAIL'; $record.error = 'Editor module changed during this run; mixed versions cannot qualify.'
    }
    if ($Visual -and !$MismatchMovement -and !$Teaching) {
        $captureNames = @('MovementFull.png', 'MovementWeakJump.png', 'MovementExhausted.png', 'MovementLowRecovering.png', 'MovementStamina.png')
        $missingCaptures = @()
        foreach ($captureName in $captureNames) {
            $image = if ($PackageExecutable) {
                $user = if ($Training) { 'User-server' } else { 'User-client' }
                Get-ChildItem -Path (Join-Path $logs "$user/Saved/Screenshots/*/$captureName") -File -ErrorAction SilentlyContinue |
                    Where-Object { $_.LastWriteTimeUtc -ge $started.ToUniversalTime() } | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
            } else {
                Get-ChildItem -Path (Join-Path $root "Saved/Screenshots/*/$captureName") -File -ErrorAction SilentlyContinue |
                    Where-Object { $_.LastWriteTimeUtc -ge $started.ToUniversalTime() } | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
            }
            if ($image) {
                $captureDir = Join-Path $logs 'Captures'; New-Item -ItemType Directory -Path $captureDir -Force | Out-Null
                $snapshot = Join-Path $captureDir $captureName
                Copy-Item -LiteralPath $image.FullName -Destination $snapshot
                $record.captures += [ordered]@{ source=$image.FullName; snapshot=$snapshot; sha256=(Get-FileHash -LiteralPath $snapshot -Algorithm SHA256).Hash.ToLowerInvariant() }
            } else { $missingCaptures += $captureName }
        }
        if ($missingCaptures.Count -gt 0 -and $record.result -eq 'PASS') {
            $record.result = 'FAIL'; $record.error = "Rendered movement run is missing fresh HUD screenshots: $($missingCaptures -join ', ')"
        }
    }
    $record.completed_utc=(Get-Date).ToUniversalTime().ToString('o'); Save
    Write-Output "FPS_MOVEMENT_TEST RESULT=$($record.result) run=$runFile"
}
if ($record.result -eq 'FAIL') { throw $record.error }
