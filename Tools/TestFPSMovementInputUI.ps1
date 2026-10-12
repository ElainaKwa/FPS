param(
    [string]$EngineRoot = 'D:\UE\UE_5.8',
    [string]$PackageExecutable = '',
    [string]$LogRoot = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$project = Join-Path $root 'MCP_Test.uproject'
$exe = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
[string[]]$prefix = @("`"$project`"")
if ($PackageExecutable) { $exe = [System.IO.Path]::GetFullPath($PackageExecutable); $prefix = @() }
if (!(Test-Path -LiteralPath $exe)) { throw "Executable missing: $exe" }
if (!$LogRoot) { $LogRoot = Join-Path $root 'Saved/FPSMovementTests/InputUI' }
$run = Join-Path ([System.IO.Path]::GetFullPath($LogRoot)) (Get-Date -Format 'yyyyMMdd-HHmmss-fff')
New-Item -ItemType Directory -Path $run | Out-Null
$config = Join-Path $run 'GameUserSettings.ini'
$baseline = Join-Path $run 'BaselineGameUserSettings.ini'
$capture = Join-Path $run 'MovementInputSettings.png'
$report = Join-Path $run 'run.json'
$originals = @()
$configRoot = Join-Path $root 'Saved/Config'
if (Test-Path -LiteralPath $configRoot) {
    $originals = @(Get-ChildItem -LiteralPath $configRoot -Filter GameUserSettings.ini -Recurse -File)
}
function FileRecord([string]$Path) {
    $item = Get-Item -LiteralPath $Path
    return [ordered]@{ path=$item.FullName; sha256=(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant(); bytes=$item.Length; modified_utc=$item.LastWriteTimeUtc.ToString('o') }
}
$protected = @($originals | ForEach-Object { FileRecord $_.FullName })
$source = Join-Path $root 'Saved/Config/WindowsEditor/GameUserSettings.ini'
if (Test-Path -LiteralPath $source) { Copy-Item -LiteralPath $source -Destination $baseline }
else { [System.IO.File]::WriteAllText($baseline, "[/Script/MCP_Test.FPSInputSettings]`r`nbSprintToggle=False`r`nbCrouchToggle=False`r`n", [System.Text.UTF8Encoding]::new($false)) }
Copy-Item -LiteralPath $baseline -Destination $config
$record = [ordered]@{
    result='RUNNING'; started_utc=(Get-Date).ToUniversalTime().ToString('o'); completed_utc=$null
    fixture='FPSMovementInputUI'; packaged=[bool]$PackageExecutable; executable=(FileRecord $exe)
    startup_timeout_seconds=180; child_launch_environment=@{ UE_SKIP_UBT_SDK_SETUP='1' }
    editor_module=$null; initial_config=(FileRecord $config); protected_original_configs=$protected
    process_records=@(); logs=@(); captures=@(); persisted_config=$null; restored_config=$null; sources=@(); error=$null
    injection='Actual saved menu widgets and UFPSActionButton OnClicked multicast injection; no hardware mouse event. Independent processes use one isolated GameUserSettingsINI override.'
    unverified=@('Hardware mouse interaction', 'Manual visual and movement feel assessment')
}
$versionFile = Join-Path $EngineRoot 'Engine/Build/Build.version'
$record.engine = if (Test-Path -LiteralPath $versionFile) { Get-Content -LiteralPath $versionFile -Raw | ConvertFrom-Json } else { $null }
$record.project = FileRecord $project
$module = Join-Path $root 'Binaries/Win64/UnrealEditor-MCP_Test.dll'
if (!$PackageExecutable) { $record.editor_module = FileRecord $module }
foreach ($relative in @('Tools/TestFPSMovementInputUI.ps1','Source/MCP_Test/FPSInputSettings.h','Source/MCP_Test/FPSInputSettings.cpp','Source/MCP_Test/FPSInterface.cpp','Source/MCP_Test/FPSMovementSmoke.cpp','Content/FPSDemo/Art/Industrial/UI/Widgets/WBP_FPSPause.uasset')) {
    $record.sources += FileRecord (Join-Path $root $relative)
}
function Save { $record | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $report -Encoding UTF8 }
function Fresh([string]$Path, [datetime]$Started) {
    if (!(Test-Path -LiteralPath $Path) -or (Get-Item -LiteralPath $Path).LastWriteTimeUtc -lt $Started.ToUniversalTime()) { return '' }
    return Get-Content -LiteralPath $Path -Raw
}
function VerifyFrozen {
    if ((FileRecord $exe).sha256 -ne $record.executable.sha256) { throw 'Executable changed during input UI acceptance.' }
    if ($record.editor_module -and (FileRecord $module).sha256 -ne $record.editor_module.sha256) { throw 'Editor module changed during input UI acceptance.' }
}
function RunEndpoint([string]$Name, [string[]]$Extra, [bool]$Visual) {
    VerifyFrozen
    $log = Join-Path $run ($Name + '.log')
    [string[]]$options = @('-game','-unattended','-nosound','-nosplash','-FPSMovementSmoke','-FPSMovementInputUI',"-GameUserSettingsINI=`"$config`"", '-ExecCmds="t.MaxFPS 60"',
        '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
    if ($Visual) { $options += @('-RenderOffscreen','-ResX=1280','-ResY=720',"-FPSMovementInputUIScreenShot=`"$capture`"") }
    else { $options += '-nullrhi' }
    if ($PackageExecutable) { $options += "-UserDir=`"$run/User`"" }
    [string[]]$arguments = @($prefix) + @('/Game/FPSDemo/Maps/LV_FPSTraining?game=/Script/MCP_Test.FPSMatchMode?Mode=Training',"-abslog=`"$log`"") + $options + $Extra
    $started = Get-Date
    $priorSdkSetup = [System.Environment]::GetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP', 'Process')
    try {
        [System.Environment]::SetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP', '1', 'Process')
        $process = Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
    } finally {
        [System.Environment]::SetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP', $priorSdkSetup, 'Process')
    }
    $entry = [ordered]@{ name=$Name; pid=$process.Id; arguments=$arguments; log=$log; started_utc=$started.ToUniversalTime().ToString('o'); exit_code=$null; owned_cleanup=$false }
    $record.process_records += $entry
    Save
    try {
        Write-Host "FPS_MOVEMENT_INPUT_UI_TEST START phase=$Name pid=$($process.Id) log=$log"
        $complete = $false
        $deadline = $started.AddSeconds(180)
        do {
            Start-Sleep -Seconds 1
            $data = Fresh $log $started
            if ($data -match 'FPS_MOVEMENT_UI_INPUT FAIL|FPS_MOVEMENT_SMOKE FAIL|Fatal error[:!]|Assertion failed:|TravelFailure:') { throw "Actual runtime failure in $Name." }
            $complete = $data -match 'FPS_MOVEMENT_UI_INPUT PASS COMPLETE(?=\s|$)'
            if ($complete -and $Visual) { $complete = (Test-Path -LiteralPath $capture) -and (Get-Item -LiteralPath $capture).LastWriteTimeUtc -ge $started.ToUniversalTime() }
            if ($process.HasExited -and !$complete) { throw "$Name exited before complete evidence: $($process.ExitCode)" }
        } while (!$complete -and (Get-Date) -lt $deadline)
        if (!$complete) { throw "Input UI phase $Name did not complete within 180 seconds." }
        VerifyFrozen
        Select-String -LiteralPath $log -Pattern 'FPS_MOVEMENT_UI_INPUT' | ForEach-Object { Write-Host $_.Line }
        return $data
    } finally {
        $process.Refresh()
        if (!$process.HasExited) { Stop-Process -Id $process.Id -Force; $entry.owned_cleanup = $true }
        if (!$process.WaitForExit(15000)) { throw "Owned input UI process failed to stop: $($process.Id)" }
        $process.Refresh(); $entry.exit_code = $process.ExitCode
        $entry.completed_utc = (Get-Date).ToUniversalTime().ToString('o')
        # Hash only closed logs; no process outside this invocation is stopped.
        if (Test-Path -LiteralPath $log) { $record.logs += FileRecord $log }
        Save
    }
}
Save
try {
    $writer = RunEndpoint 'writer' @() $true
    $match = [regex]::Match(($writer -join "`n"), 'FPS_MOVEMENT_UI_INPUT PASS SAVED sprint=([01]) crouch=([01])')
    if (!$match.Success) { throw 'UI writer did not report both saved input choices.' }
    $sprint = $match.Groups[1].Value; $crouch = $match.Groups[2].Value
    $persisted = Join-Path $run 'WrittenGameUserSettings.ini'
    Copy-Item -LiteralPath $config -Destination $persisted
    $record.persisted_config = FileRecord $persisted
    $record.captures += FileRecord $capture
    Save
    $reader = RunEndpoint 'reader' @('-FPSMovementInputUIReadback',"-FPSMovementInputUIExpectedSprint=$sprint","-FPSMovementInputUIExpectedCrouch=$crouch") $false
    if (($reader -join "`n") -notmatch "FPS_MOVEMENT_UI_INPUT PASS NEW_PROCESS sprint=$sprint crouch=$crouch(?=\s|$)") { throw 'Independent process did not load both saved UI input choices.' }
    $record.result = 'PASS'
} catch {
    $record.result = 'FAIL'; $record.error = $_.Exception.Message
    throw
} finally {
    Copy-Item -LiteralPath $baseline -Destination $config -Force
    $record.restored_config = FileRecord $config
    if ($record.restored_config.sha256 -ne $record.initial_config.sha256) { $record.result='FAIL'; $record.error='Isolated original configuration was not restored byte-for-byte.' }
    foreach ($item in $protected) {
        if (!(Test-Path -LiteralPath $item.path) -or (FileRecord $item.path).sha256 -ne $item.sha256) { $record.result='FAIL'; $record.error="Original user configuration changed: $($item.path)" }
    }
    $record.completed_utc = (Get-Date).ToUniversalTime().ToString('o')
    Save
}
if ($record.result -ne 'PASS') { throw $record.error }
Write-Output "FPS_MOVEMENT_INPUT_UI_TEST PASS COMPLETE report=$report"
