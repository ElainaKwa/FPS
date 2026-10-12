param([string]$EngineRoot = 'D:\UE\UE_5.8', [switch]$Audio, [switch]$Visual, [switch]$Abilities, [switch]$Teaching, [switch]$Damage, [switch]$Presentation, [switch]$Recoil, [switch]$Uncompensated, [int]$Latency = 0, [int]$PacketLoss = 0, [string]$InvalidWeapon = "", [string]$PackageExecutable = '', [string]$LogRoot = '')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$editorExe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$projectFile = Join-Path $projectRoot 'MCP_Test.uproject'
if ($PackageExecutable) {
    if (!(Test-Path -LiteralPath $PackageExecutable -PathType Leaf)) { throw "Packaged executable missing: $PackageExecutable" }
    $editorExe = (Resolve-Path -LiteralPath $PackageExecutable).Path
}
[string[]]$launchPrefix = if ($PackageExecutable) { @() } else { @("`"$projectFile`"") }
if (!$LogRoot) { $LogRoot = Join-Path $projectRoot 'Saved\FPSFlowTests' }
$LogRoot = [System.IO.Path]::GetFullPath($LogRoot)
New-Item -ItemType Directory -Path $logRoot -Force | Out-Null
$testName = if ($Recoil) { 'Recoil' } elseif ($Presentation) { 'Presentation' } elseif ($Damage) { 'Damage' } elseif ($Teaching) { 'Teaching' } elseif ($Abilities) { 'Abilities' } else { 'Weapons' }
$marker = if ($Recoil) { 'FPS_RECOIL_SMOKE' } elseif ($Presentation) { 'FPS_PRESENTATION_SMOKE' } elseif ($Damage) { 'FPS_DAMAGE_SMOKE' } elseif ($Teaching) { 'FPS_TEACHING_SMOKE' } elseif ($Abilities) { 'FPS_ABILITY_SMOKE' } else { 'FPS_WEAPON_SMOKE' }
$flag = if ($Recoil) { '-FPSRecoilSmoke' } elseif ($Presentation) { '-FPSWeaponPresentationSmoke' } elseif ($Damage) { '-FPSWeaponDamageSmoke' } elseif ($Teaching) { '-FPSTeachingSmoke' } elseif ($Abilities) { '-FPSWeaponAbilitySmoke' } else { '-FPSWeaponSmoke' }
$serverLog = Join-Path $logRoot ($testName + '-server.log')
$clientLog = Join-Path $logRoot ($testName + '-client.log')
$ownedProcesses = @()
$observerLog = Join-Path $logRoot 'Presentation-observer.log'
$observerStarted = $false
$runStarted = Get-Date
foreach ($testLog in @($serverLog, $clientLog)) { [System.IO.File]::WriteAllText($testLog, '') }
if ($Presentation) { [System.IO.File]::WriteAllText($observerLog, '') }
try {
    $common = @('-game', '-nullrhi', '-unattended', '-nosplash', '-ExecCmds="t.MaxFPS 60"', $flag, '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
    if ($PackageExecutable) { $common += "-UserDir=`"$(Join-Path $LogRoot 'User')`"" }
    if ($Uncompensated) { $common += "-FPSRecoilUncompensated" }
    if ($InvalidWeapon) { $common += "-FPSInvalidWeapon=$InvalidWeapon" }
    if ($Latency -gt 0) { $common += "-PktLag=$Latency" }
    if ($PacketLoss -gt 0) { $common += "-PktLoss=$PacketLoss" }
    if ($Audio) { $common += @('-AudioMixer', '-FPSAudioTrace') } else { $common += '-nosound' }
    $map = if ($Teaching) { '/Game/Maps/LV_NetSyncFPS?listen' } else { '/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=LAN?listen' }
    $ownedProcesses += Start-Process -FilePath $editorExe -ArgumentList ($launchPrefix + @($map, '-port=17779', "-abslog=`"$serverLog`"") + $common) -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 6
    $clientOptions = $common
    if ($Visual) { $clientOptions = @($common | Where-Object { $_ -ne '-nullrhi' }) + @('-RenderOffscreen', '-ResX=1280', '-ResY=720', '-FPSWeaponCapture') }
    $ownedProcesses += Start-Process -FilePath $editorExe -ArgumentList ($launchPrefix + @('127.0.0.1:17779', "-abslog=`"$clientLog`"") + $clientOptions) -PassThru -WindowStyle Hidden
    $deadline = (Get-Date).AddSeconds(180)
    do {
        Start-Sleep -Seconds 1
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if ($Presentation -and !$observerStarted -and $serverText -match 'FPS_PRESENTATION_LATE_WINDOW') {
            $observerOptions = @($common | Where-Object { $_ -ne $flag -and $_ -ne '-FPSAudioTrace' -and $_ -ne '-AudioMixer' }) + @('-FPSWeaponPresentationObserver', '-nosound')
            $ownedProcesses += Start-Process -FilePath $editorExe -ArgumentList ($launchPrefix + @('127.0.0.1:17779', "-abslog=`"$observerLog`"") + $observerOptions) -PassThru -WindowStyle Hidden
            $observerStarted = $true
        }
        $complete = (Test-Path $serverLog) -and (Test-Path $clientLog) -and (Get-Item $serverLog).LastWriteTime -ge $runStarted -and (Get-Item $clientLog).LastWriteTime -ge $runStarted -and $serverText -match "$marker (PASS|FAIL) SERVER COMPLETE\b" -and $clientText -match "$marker (PASS|FAIL) CLIENT COMPLETE\b"
        if ($Presentation) {
            $observerText = if (Test-Path $observerLog) { Get-Content $observerLog -Raw } else { '' }
            $complete = $complete -and $observerStarted -and $observerText -match 'FPS_PRESENTATION_OBSERVER (PASS|FAIL) CLIENT COMPLETE\b'
        }
        foreach ($testProcess in $ownedProcesses) { if ($testProcess.HasExited) { throw "Weapon test process exited before completion: $($testProcess.Id)" } }
    } while (!$complete -and (Get-Date) -lt $deadline)
} finally {
    foreach ($testProcess in $ownedProcesses) {
        if (!$testProcess.HasExited) { Stop-Process -Id $testProcess.Id -ErrorAction SilentlyContinue }
    }
}
foreach ($testLog in @($serverLog, $clientLog)) {
    $testText = Get-Content -LiteralPath $testLog -Raw
    Select-String -LiteralPath $testLog -Pattern $marker
    if ((Get-Item -LiteralPath $testLog).LastWriteTime -lt $runStarted -or $testText -match "$marker FAIL|Fatal error:|Assertion failed:|TravelFailure:" -or $testText -notmatch "$marker PASS (SERVER|CLIENT) COMPLETE\b") {
        throw "Weapon checks failed or incomplete: $testLog"
    }
    if ($Audio -and ($testText -match 'FPS_AUDIO FAIL|FPS_AUDIO missing' -or $testText -notmatch 'FPS_AUDIO PASS cue=S_Fire_' -or $testText -notmatch 'FPS_AUDIO RECORD exported')) {
        throw "Audio checks failed or incomplete: $testLog"
    }
}
if ($Presentation) {
    $observerText = if (Test-Path $observerLog) { Get-Content $observerLog -Raw } else { '' }
    Select-String -LiteralPath $observerLog -Pattern 'FPS_PRESENTATION_OBSERVER'
    if (!$observerStarted -or (Get-Item $observerLog).LastWriteTime -lt $runStarted -or $observerText -match 'FPS_PRESENTATION_OBSERVER FAIL|Fatal error:|Assertion failed:|TravelFailure:' -or $observerText -notmatch 'FPS_PRESENTATION_OBSERVER PASS CLIENT COMPLETE\b') {
        throw 'Late observer checks failed or incomplete.'
    }
}
Write-Output 'PASS: real client GAS abilities, inventory replication and movement prediction.'
