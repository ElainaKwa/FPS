param([string]$EngineRoot = 'D:\UE\UE_5.8')
$projectRoot = Split-Path $PSScriptRoot -Parent
$projectFile = Join-Path $projectRoot 'MCP_Test.uproject'
$editorExe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$logRoot = Join-Path $projectRoot 'Saved\FPSFlowTests'
New-Item -ItemType Directory -Path $logRoot -Force | Out-Null
$serverLog = Join-Path $logRoot 'LobbyIdle-server.log'
$clientLog = Join-Path $logRoot 'LobbyIdle-client.log'
$ownedProcesses = @()
try {
    $common = @('-game', '-nullrhi', '-nosound', '-unattended', '-nosplash', '-FPSLobbySmoke')
    $ownedProcesses += Start-Process -FilePath $editorExe -ArgumentList (@("`"$projectFile`"", '/Engine/Maps/Entry?game=/Script/MCP_Test.FPSLobbyMode?listen', '-port=17778', "-abslog=`"$serverLog`"") + $common) -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 6
    $ownedProcesses += Start-Process -FilePath $editorExe -ArgumentList (@("`"$projectFile`"", '127.0.0.1:17778', "-abslog=`"$clientLog`"") + $common) -PassThru -WindowStyle Hidden
    # No server input, menu selection, start-round command or travel is performed.
    Start-Sleep -Seconds 25
} finally {
    foreach ($testProcess in $ownedProcesses) {
        if (!$testProcess.HasExited) { Stop-Process -Id $testProcess.Id -ErrorAction SilentlyContinue }
    }
}
$serverText = Get-Content -LiteralPath $serverLog -Raw
$clientText = Get-Content -LiteralPath $clientLog -Raw
Select-String -LiteralPath $serverLog,$clientLog -Pattern 'FPS_LOBBY_SMOKE'
if ($serverText -notmatch 'FPS_LOBBY_SMOKE PASS menu-ready role=SERVER' -or
    $clientText -notmatch 'FPS_LOBBY_SMOKE PASS menu-ready role=CLIENT auth-mode=0') {
    throw "Lobby initialization failed. Inspect $logRoot"
}
if ($serverText -match 'FPS_LOBBY_SMOKE FAIL' -or $clientText -match 'FPS_LOBBY_SMOKE FAIL') {
    throw 'Incomplete lobby menu detected.'
}
Write-Output 'PASS: client lobby initialized while the server remained idle.'
