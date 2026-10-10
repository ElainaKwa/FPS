param(
    [string]$EngineRoot = 'D:\UE\UE_5.8',
    [string]$PackageRoot = '',
    [string]$ServerExecutable = '',
    [ValidateRange(2,10)][int]$Clients = 10,
    [ValidateRange(0,60000)][int]$Latency = 150,
    [ValidateRange(0,100)][int]$PacketLoss = 3,
    [switch]$Reconnect,
    [ValidateRange(0,300)][int]$SampleSeconds = 0,
    [string]$RunName = 'DedicatedLong',
    [string]$OutputRoot = '',
    [ValidateRange(1,65535)][int]$Port = 17785
)
$ErrorActionPreference = 'Stop'
if ($SampleSeconds -gt 0 -and ($SampleSeconds -lt 35 -or $Reconnect)) { throw 'Sampling requires at least 35 seconds and cannot claim reconnect or complete-round acceptance.' }
if ($RunName -notmatch '^[A-Za-z0-9_-]+$') { throw 'RunName must be a directory leaf containing letters, digits, hyphens or underscores.' }
$root = Split-Path $PSScriptRoot -Parent
$allowedOutput = [System.IO.Path]::GetFullPath((Join-Path $root 'Saved/FPSCharacterAcceptance'))
if (!$OutputRoot) { $OutputRoot = $allowedOutput }
$OutputRoot = [System.IO.Path]::GetFullPath($OutputRoot).TrimEnd('\','/')
if ($OutputRoot -ne $allowedOutput -and !$OutputRoot.StartsWith($allowedOutput + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'OutputRoot must remain within Saved/FPSCharacterAcceptance.' }
$logs = Join-Path $OutputRoot ($RunName + '-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
if (Test-Path -LiteralPath $logs) { throw "Evidence directory already exists: $logs" }
New-Item -ItemType Directory -Path $logs | Out-Null
$project = Join-Path $root 'MCP_Test.uproject'
if (!$PackageRoot) { $PackageRoot = Join-Path $root 'Saved/Packages/CharacterAcceptance/Windows' }
$PackageRoot = [System.IO.Path]::GetFullPath($PackageRoot)
$clientExe = Join-Path $PackageRoot 'MCP_Test/Binaries/Win64/MCP_Test.exe'
$serverExe = if ($ServerExecutable) { [System.IO.Path]::GetFullPath($ServerExecutable) } else { Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe' }
foreach ($path in @($clientExe, $serverExe, $project)) { if (!(Test-Path -LiteralPath $path)) { throw "Required launch input missing: $path" } }
$editorServer = [System.IO.Path]::GetFileNameWithoutExtension($serverExe) -like 'UnrealEditor*'
$serverLog = Join-Path $logs 'server.log'
$runFile = Join-Path $logs 'run.json'
$common = @('-nullrhi','-nosound','-unattended','-nosplash','-FPSAcceptance',"-FPSExpectedPlayers=$Clients","-PktLag=$Latency","-PktLoss=$PacketLoss",'-ExecCmds="t.MaxFPS 30,net.EnableNetStats 1"')
$map = '/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=LAN?Bots=0'
[string[]]$prefix = @()
if ($editorServer) { $prefix = @("`"$project`"") }
[string[]]$serverArguments = @($prefix) + @($map,'-server',"-port=$Port","-abslog=`"$serverLog`"","-UserDir=`"$logs/User-server`"",'-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False') + $common
if ($editorServer) { $serverArguments = @($serverArguments | Where-Object { $_ -notlike '-UserDir=*' }) }
$started = Get-Date
$timeout = if ($SampleSeconds -gt 0) { $SampleSeconds + 100 } else { 470 }
$owned = @(); $targets = @(); $clientProcesses = @(); $clientLogs = @(); $allClientLogs = @()
$killedAt = $null; $rejoined = $false; $complete = $false
function Get-BinaryRecord([string]$Path) {
    $file = Get-Item -LiteralPath $Path
    return [ordered]@{ path=$file.FullName; sha256=(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant(); bytes=$file.Length; modified_utc=$file.LastWriteTimeUtc.ToString('o'); file_version=$file.VersionInfo.FileVersion }
}
$engineVersion = Join-Path $EngineRoot 'Engine/Build/Build.version'
$module = Join-Path $root 'Binaries/Win64/UnrealEditor-MCP_Test.dll'
$record = [ordered]@{
    started_utc=$started.ToUniversalTime().ToString('o'); completed_utc=$null; result='RUNNING'; logs=$logs
    package_root=$PackageRoot; project=$project; server_binary=(Get-BinaryRecord $serverExe); client_binary=(Get-BinaryRecord $clientExe)
    editor_module=$(if ($editorServer -and (Test-Path -LiteralPath $module)) { Get-BinaryRecord $module } else { $null })
    engine_build=$(if (Test-Path -LiteralPath $engineVersion) { Get-Content -LiteralPath $engineVersion -Raw | ConvertFrom-Json } else { $null })
    protocol=6; fixture='FPSAcceptance'; mode='LAN'; map=$map; bots=0; expected_players=$Clients; round_seconds=300
    expected_server_netmode=1; expected_client_netmode=3; latency_ms=$Latency; packet_loss_percent=$PacketLoss
    reconnect=[bool]$Reconnect; blackout_seconds=20; sample_seconds=$SampleSeconds; timeout_seconds=$timeout; nullrhi=$true
    process_records=@(); client_arguments=@(); server_arguments=$serverArguments; reconnect_events=@(); verified_logs=@(); error=$null
}
function Save-Record { $record | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath $runFile -Encoding UTF8 }
function Add-OwnedProcess([System.Diagnostics.Process]$Process, [string]$Role, [string]$Log, [string[]]$Arguments, [string]$UserDir) {
    $script:owned += $Process
    $record.process_records += [ordered]@{ pid=$Process.Id; role=$Role; log=$Log; user_dir=$UserDir; arguments=$Arguments; started_utc=$Process.StartTime.ToUniversalTime().ToString('o'); exit_code=$null; owned_cleanup=$false; intentionally_disconnected=$false }
    Save-Record
}
function Read-FreshLog([string]$Path) {
    if (!(Test-Path -LiteralPath $Path) -or (Get-Item -LiteralPath $Path).LastWriteTime -lt $started) { return '' }
    return [string](Get-Content -LiteralPath $Path -Raw)
}
function New-Client([int]$Index, [switch]$Rejoin) {
    $suffix = if ($Rejoin) { '-rejoined' } else { '' }
    $log = Join-Path $logs "client-$Index$suffix.log"
    $userDir = Join-Path $logs "User-client-$Index$suffix"
    $extra = if ($Index -eq 1 -and !$Rejoin) { @('-FPSAcceptanceBlackout') } else { @() }
    $arguments = @("127.0.0.1:$Port","-abslog=`"$log`"","-UserDir=`"$userDir`"","-FPSAcceptanceClient=$Index") + $common + $extra
    $process = Start-Process -FilePath $clientExe -ArgumentList $arguments -WindowStyle Hidden -PassThru
    Add-OwnedProcess $process "CLIENT-$Index$suffix" $log $arguments $userDir
    $record.client_arguments += ,$arguments; Save-Record
    return [pscustomobject]@{ Process=$process; Log=$log }
}
Save-Record
try {
    Write-Output "FPS_CHARACTER_ACCEPTANCE START clients=$Clients duration=300 lag_ms=$Latency loss_percent=$PacketLoss reconnect=$([bool]$Reconnect) sample_seconds=$SampleSeconds server=$serverExe logs=$logs"
    $process = Start-Process -FilePath $serverExe -ArgumentList $serverArguments -WindowStyle Hidden -PassThru
    Add-OwnedProcess $process 'SERVER' $serverLog $serverArguments $(if ($editorServer) { 'ENGINE_DEFAULT_SHARED_CACHE' } else { Join-Path $logs 'User-server' })
    $serverReadyDeadline = (Get-Date).AddSeconds(90)
    $canTick = $false
    do {
        Start-Sleep -Seconds 1
        $startupText = Read-FreshLog $serverLog
        if ($startupText -match 'Fatal error[:!]|Assertion failed:') { throw 'Dedicated server failed before its world finished loading.' }
        if ($process.HasExited) { throw 'Dedicated server exited during world initialization.' }
        $canTick = $startupText -match 'FPS_WEAPON_CONTENT READY' -and $startupText -match 'Load map complete /Game/FPSDemo/Maps/LV_FPSFoundry'
    } while (!$canTick -and (Get-Date) -lt $serverReadyDeadline)
    if (!$canTick) { throw 'Dedicated world readiness timeout; socket listening alone is insufficient.' }
    $record.server_world_ready_utc = (Get-Date).ToUniversalTime().ToString('o'); Save-Record
    for ($i=1; $i -le $Clients; $i++) {
        $client = New-Client $i
        $clientProcesses += $client.Process; $clientLogs += $client.Log; $allClientLogs += $client.Log
        Start-Sleep -Milliseconds 700
    }
    $deadline = $started.AddSeconds($timeout)
    do {
        Start-Sleep -Seconds 2
        $serverText = Read-FreshLog $serverLog
        if ($serverText -match 'FPS_ACCEPT FAIL|Fatal error[:!]|Assertion failed:') { throw 'Server runtime acceptance failure.' }
        if ($Reconnect -and !$killedAt -and $serverText -match 'FPS_ACCEPT SAMPLE role=SERVER netmode=1 age=1[2-9][0-9]\.') {
            $lastClient = $clientProcesses[-1]
            if ($lastClient.HasExited) { throw 'Intended reconnect client had already exited.' }
            Stop-Process -Id $lastClient.Id -ErrorAction Stop
            $killedAt = Get-Date
            foreach ($entry in $record.process_records) { if ($entry.pid -eq $lastClient.Id) { $entry.intentionally_disconnected = $true } }
            $record.reconnect_events += [ordered]@{ event='owned-client-stop'; client=$Clients; pid=$lastClient.Id; utc=$killedAt.ToUniversalTime().ToString('o') }
            Save-Record
            Write-Output "FPS_CHARACTER_ACCEPTANCE RECONNECT stop client=$Clients pid=$($lastClient.Id)"
        }
        if ($killedAt -and !$rejoined -and ((Get-Date)-$killedAt).TotalSeconds -ge 70) {
            if ($serverText -notmatch "FPS_ACCEPT SAMPLE role=SERVER netmode=1 age=.* players=$($Clients-1) ") { throw 'Server did not observe the disconnected real player slot before rejoin.' }
            $client = New-Client $Clients -Rejoin
            $clientProcesses[-1] = $client.Process; $clientLogs[-1] = $client.Log; $allClientLogs += $client.Log; $rejoined = $true
            $record.reconnect_events += [ordered]@{ event='real-client-rejoin'; client=$Clients; pid=$client.Process.Id; utc=(Get-Date).ToUniversalTime().ToString('o') }
            Save-Record
            Write-Output "FPS_CHARACTER_ACCEPTANCE RECONNECT rejoin client=$Clients pid=$($client.Process.Id)"
        }
        $complete = $serverText -match 'FPS_ACCEPT PASS SERVER COMPLETE(?=\s|$)'
        foreach ($log in $clientLogs) {
            $data = Read-FreshLog $log
            if ($data -match 'FPS_ACCEPT FAIL|TravelFailure:|Fatal error[:!]|Assertion failed:') { throw "Client runtime acceptance failure: $log" }
            $complete = $complete -and ($data -match 'FPS_ACCEPT PASS CLIENT COMPLETE(?=\s|$)')
        }
        if ($owned[0].HasExited) { throw 'Dedicated server exited before completion.' }
        foreach ($clientProcess in $clientProcesses) {
            if ($clientProcess.HasExited -and !($killedAt -and !$rejoined -and $clientProcess.Id -eq $record.reconnect_events[0].pid)) { throw "Active client exited before completion: $($clientProcess.Id)" }
        }
        if ($SampleSeconds -gt 0 -and $serverText -match 'FPS_ACCEPT SAMPLE role=SERVER netmode=1 age=([0-9.]+)') {
            $ages = [regex]::Matches($serverText, 'FPS_ACCEPT SAMPLE role=SERVER netmode=1 age=([0-9.]+)')
            if ([double]$ages[-1].Groups[1].Value -ge $SampleSeconds) { break }
        }
    } while (!$complete -and (Get-Date) -lt $deadline)
    if ($SampleSeconds -eq 0 -and !$complete) { throw 'Bounded wait ended without complete fresh server and all active client markers.' }
    $serverText = Read-FreshLog $serverLog
    if ($serverText -notmatch "FPS_ACCEPT PASS SERVER READY realControllers=$Clients admitted=$Clients duration=300 protocol=6 netmode=1") { throw 'Actual dedicated mode, full player readiness or protocol marker missing.' }
    foreach ($log in @($serverLog) + $allClientLogs) {
        $data = Read-FreshLog $log
        Select-String -LiteralPath $log -Pattern 'FPS_ACCEPT|FPS_WEAPON_CONTENT'
        if (!$data -or $data -match 'FPS_ACCEPT FAIL|Fatal error[:!]|Assertion failed:|TravelFailure:') { throw "Failed or stale evidence: $log" }
        $role = if ($log -eq $serverLog) { 'SERVER' } else { 'CLIENT' }
        $active = $log -eq $serverLog -or $clientLogs -contains $log
        if ($active -and $SampleSeconds -eq 0 -and $data -notmatch "FPS_ACCEPT PASS $role COMPLETE(?=\s|$)") { throw "Missing exact role COMPLETE: $log" }
        $netmode = if ($role -eq 'SERVER') { 1 } else { 3 }
        if ($data -notmatch "FPS_ACCEPT SAMPLE role=$role netmode=$netmode .*inBps=[1-9][0-9]* outBps=[1-9][0-9]*") { throw "No active bytes-per-second sample for expected actual netmode: $log" }
        # Unreal keeps the live log open. Hash only after owned processes have stopped in finally.
        $record.verified_logs += [ordered]@{ path=$log; role=$role; active=$active; fresh=$true; complete=($data -match "FPS_ACCEPT PASS $role COMPLETE(?=\s|$)"); sha256=$null; modified_utc=(Get-Item -LiteralPath $log).LastWriteTimeUtc.ToString('o') }
    }
    if ($SampleSeconds -gt 0) {
        $record.result = 'PASS_SAMPLE_ONLY'
        Write-Output 'FPS_CHARACTER_ACCEPTANCE PASS SAMPLE ONLY: process, input prerequisites and network observations; complete round and later lifecycle remain unverified.'
    } else {
        if ((Read-FreshLog $clientLogs[0]) -notmatch 'FPS_ACCEPT PASS BLACKOUT reliable round trip recovered') { throw 'Actual blackout recovery RPC round trip missing.' }
        $observers = @($clientLogs | Where-Object { (Read-FreshLog $_) -match 'RELEVANCE observed missing1=1 restored1=1 missing2=1 restored2=1' }).Count
        if ($observers -lt ($Clients - 2)) { throw "Repeated relevant actor restoration missing: $observers observers" }
        if ($Reconnect -and (!$rejoined -or (Read-FreshLog $clientLogs[-1]) -notmatch 'rejoined real player fresh default inventory and base permission')) { throw 'Real disconnect/new default inventory rejoin evidence missing.' }
        if ($serverText -notmatch 'FPS_ACCEPT PASS SERVER COMPLETE age=3[0-9][0-9]\.[0-9]+ observedWall=([0-9.]+)') { throw 'Full actual round server elapsed marker missing.' }
        if ([double]$Matches[1] -lt 295) { throw 'Server wall observation was shorter than the unscaled full round.' }
        if ($serverText -notmatch 'FPS_ACCEPT PASS SERVER DROPS.*natural 180 second lifetime cleanup' -or $serverText -notmatch 'FPS_ACCEPT PASS SERVER COMPLETE.*dynamic=0') { throw 'Natural dynamic lifetime or real round cleanup evidence missing.' }
        $record.relevance_observers = $observers; $record.result = 'PASS_FULL_ROUND'
        Write-Output 'FPS_CHARACTER_ACCEPTANCE PASS FULL ROUND: admitted real players, unscaled 300 seconds, packaged owner clients, dynamic lifetime and round cleanup.'
    }
} catch {
    $record.result = 'FAIL'; $record.error = $_.Exception.Message
    foreach ($log in @($serverLog) + $allClientLogs) { if (Test-Path -LiteralPath $log) { Select-String -LiteralPath $log -Pattern 'FPS_ACCEPT|Fatal error:|Assertion failed:|TravelFailure:' } }
    throw
} finally {
    foreach ($game in $owned) {
        $game.Refresh()
        $exitCode = $null
        if ($game.HasExited) { try { $exitCode = $game.ExitCode } catch {} }
        else {
            Stop-Process -Id $game.Id -ErrorAction SilentlyContinue
            $game.WaitForExit(5000) | Out-Null
            $game.Refresh()
            if ($game.HasExited) { try { $exitCode = $game.ExitCode } catch {} }
        }
        foreach ($entry in $record.process_records) { if ($entry.pid -eq $game.Id) { $entry.exit_code = $exitCode; $entry.owned_cleanup = $true } }
    }
    foreach ($entry in $record.verified_logs) {
        if (Test-Path -LiteralPath $entry.path) {
            $entry.sha256 = (Get-FileHash -LiteralPath $entry.path -Algorithm SHA256).Hash.ToLowerInvariant()
            $entry.modified_utc = (Get-Item -LiteralPath $entry.path).LastWriteTimeUtc.ToString('o')
        }
    }
    $record.completed_utc = (Get-Date).ToUniversalTime().ToString('o')
    Save-Record
    Write-Output "FPS_CHARACTER_ACCEPTANCE RESULT=$($record.result) run=$runFile"
}
