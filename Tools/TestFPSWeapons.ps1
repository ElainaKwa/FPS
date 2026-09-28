param([string]$EngineRoot = 'D:\UE\UE_5.8', [switch]$Audio)
$projectRoot = Split-Path $PSScriptRoot -Parent
$editorExe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$projectFile = Join-Path $projectRoot 'MCP_Test.uproject'
$logRoot = Join-Path $projectRoot 'Saved\FPSFlowTests'
New-Item -ItemType Directory -Path $logRoot -Force | Out-Null
$serverLog = Join-Path $logRoot 'Weapons-server.log'
$clientLog = Join-Path $logRoot 'Weapons-client.log'
$ownedProcesses = @()
try {
    $common = @('-game', '-nullrhi', '-unattended', '-nosplash', '-ExecCmds="t.MaxFPS 60"', '-FPSWeaponSmoke')
    if ($Audio) { $common += @('-AudioMixer', '-FPSAudioTrace') } else { $common += '-nosound' }
    $ownedProcesses += Start-Process -FilePath $editorExe -ArgumentList (@("`"$projectFile`"", '/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=LAN?listen', '-port=17779', "-abslog=`"$serverLog`"") + $common) -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 6
    $ownedProcesses += Start-Process -FilePath $editorExe -ArgumentList (@("`"$projectFile`"", '127.0.0.1:17779', "-abslog=`"$clientLog`"") + $common) -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 40
} finally {
    foreach ($testProcess in $ownedProcesses) {
        if (!$testProcess.HasExited) { Stop-Process -Id $testProcess.Id -ErrorAction SilentlyContinue }
    }
}
foreach ($testLog in @($serverLog, $clientLog)) {
    $testText = Get-Content -LiteralPath $testLog -Raw
    Select-String -LiteralPath $testLog -Pattern 'FPS_WEAPON_SMOKE'
    if ($testText -match 'FPS_WEAPON_SMOKE FAIL|Fatal error:|Assertion failed:' -or $testText -notmatch 'FPS_WEAPON_SMOKE PASS (SERVER|CLIENT) COMPLETE') {
        throw "Weapon checks failed or incomplete: $testLog"
    }
    if ($Audio -and ($testText -match 'FPS_AUDIO FAIL|FPS_AUDIO missing' -or $testText -notmatch 'FPS_AUDIO PASS cue=Fire_' -or $testText -notmatch 'FPS_AUDIO RECORD exported')) {
        throw "Audio checks failed or incomplete: $testLog"
    }
}
Write-Output 'PASS: real client weapon RPCs, inventory replication and movement prediction.'
