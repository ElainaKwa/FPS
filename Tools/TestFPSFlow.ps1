param(
    [ValidateSet('LAN','Solo','Training')] [string]$Mode = 'LAN',
    [string]$EngineRoot = 'D:\UE\UE_5.8',
    [switch]$Audio,
    [switch]$Visual
)
$projectRoot = Split-Path $PSScriptRoot -Parent
$projectFile = Join-Path $projectRoot 'MCP_Test.uproject'
$editorExe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$logRoot = Join-Path $projectRoot 'Saved\FPSFlowTests'
New-Item -ItemType Directory -Path $logRoot -Force | Out-Null
$serverLog = Join-Path $logRoot "$Mode-server.log"
$clientLog = Join-Path $logRoot "$Mode-client.log"
$ownedProcesses = @()
$runStarted = Get-Date
try {
    $testMap = if ($Mode -eq 'Training') { '/Game/FPSDemo/Maps/LV_FPSTraining' } else { '/Game/FPSDemo/Maps/LV_FPSFoundry' }
    $mapURL = "${testMap}?game=/Script/MCP_Test.FPSMatchMode?Mode=${Mode}?Bots=1"
    if ($Mode -eq 'LAN') { $mapURL += '?listen' }
    $common = @('-game', '-nullrhi', '-unattended', '-nosplash', '-ExecCmds="t.MaxFPS 60"', '-FPSFlowSmoke')
    if ($Visual) { $common = @($common | Where-Object { $_ -ne '-nullrhi' }) + @('-RenderOffscreen', '-ResX=1280', '-ResY=800', '-FPSFeedbackCapture') }
    if ($Audio) { $common += @('-AudioMixer', '-FPSAudioTrace') } else { $common += '-nosound' }
    $ownedProcesses += Start-Process -FilePath $editorExe -ArgumentList (@("`"$projectFile`"", $mapURL, '-port=17777', "-abslog=`"$serverLog`"") + $common) -PassThru -WindowStyle Hidden
    if ($Mode -eq 'LAN') {
        Start-Sleep -Seconds 6
        $ownedProcesses += Start-Process -FilePath $editorExe -ArgumentList (@("`"$projectFile`"", '127.0.0.1:17777', "-abslog=`"$clientLog`"") + $common) -PassThru -WindowStyle Hidden
    }
    $waitStarted = Get-Date
    do {
        Start-Sleep -Seconds 1
        $complete = (Test-Path -LiteralPath $serverLog) -and (Get-Item -LiteralPath $serverLog).LastWriteTime -ge $runStarted -and ((Get-Content -LiteralPath $serverLog -Raw) -match 'FPS_SMOKE (PASS|FAIL) finished round preserves death view')
        if ($Mode -eq 'LAN') {
            $complete = $complete -and (Test-Path -LiteralPath $clientLog) -and ((Get-Content -LiteralPath $clientLog -Raw) -match 'SNAPSHOT CLIENT waiting=0 finished=1 players=2 podium=2')
        }
    } while (!$complete -and ((Get-Date) - $waitStarted).TotalSeconds -lt 100)
} finally {
    # Only terminate the temporary game processes created by this invocation.
    foreach ($testProcess in $ownedProcesses) {
        if (!$testProcess.HasExited) { Stop-Process -Id $testProcess.Id -ErrorAction SilentlyContinue }
    }
}
$serverText = Get-Content -LiteralPath $serverLog -Raw
Select-String -LiteralPath $serverLog -Pattern 'FPS_SMOKE'
if ($serverText -match 'FPS_SMOKE FAIL|Fatal error:|Assertion failed:' -or $serverText -notmatch 'FPS_SMOKE PASS finished round preserves death view') { throw "Server checks failed or incomplete. See $serverLog" }
if ($Mode -eq 'LAN') {
    $clientText = Get-Content -LiteralPath $clientLog -Raw
    Select-String -LiteralPath $clientLog -Pattern 'FPS_SMOKE'
    if ($clientText -match 'Fatal error:|Assertion failed:' -or $clientText -notmatch 'SNAPSHOT CLIENT waiting=0 finished=1 players=2 podium=2.*score=1') { throw "Client final replication check failed. See $clientLog" }
    foreach ($marker in @('damage-flash CLIENT', 'death-info CLIENT', 'respawn-reset CLIENT')) {
        if (!$clientText.Contains("FPS_FEEDBACK PASS $marker")) { throw "Client feedback check missing: $marker" }
    }
    Select-String -LiteralPath $clientLog -Pattern 'FPS_FEEDBACK'
    if ($Visual) {
        foreach ($captureName in @('FeedbackDamage.png', 'FeedbackDeath.png')) {
            $capture = Join-Path $projectRoot "Saved/Screenshots/WindowsEditor/$captureName"
            if (!(Test-Path -LiteralPath $capture) -or (Get-Item -LiteralPath $capture).LastWriteTime -lt $runStarted) { throw "Fresh feedback screenshot missing: $captureName" }
        }
    }
}
Write-Output "FPS flow checks passed: $Mode"
if ($Audio) {
    $audioText = $serverText
    if ($Mode -eq 'LAN') { $audioText += $clientText }
    if ($audioText -match 'FPS_AUDIO FAIL|FPS_AUDIO missing' -or $audioText -notmatch 'FPS_AUDIO PASS cue=Hit route=LOCAL' -or $audioText -notmatch 'FPS_AUDIO PASS cue=Kill route=LOCAL') {
        throw 'Hit/kill audio was missing or failed to start.'
    }
    Write-Output 'PASS: authoritative hit and kill confirmations started local audio.'
}
