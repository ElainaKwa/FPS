param([string]$EngineRoot = 'D:\UE\UE_5.8')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\FPSCharacterTests\Regions.log'
$started = Get-Date
$arguments = @("`"$root\MCP_Test.uproject`"",'/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=Solo?Bots=0',
    '-game','-RenderOffscreen','-windowed','-ResX=1280','-ResY=720','-nosound','-unattended','-nosplash','-FPSMapSmoke','-FPSRegionCapture',
    '-ExecCmds="t.MaxFPS 60"',"-abslog=`"$log`"",'-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
$game = Start-Process $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    $deadline = (Get-Date).AddSeconds(60)
    do {
        Start-Sleep -Seconds 1
        $done = $true
        foreach ($name in @('CharacterBlueRegion.png','CharacterRedRegion.png')) {
            $file = Join-Path $root "Saved\Screenshots\WindowsEditor\$name"
            $done = $done -and (Test-Path $file) -and ((Get-Item $file).LastWriteTime -gt $started)
        }
        if ($game.HasExited) { throw 'Region capture exited early.' }
    } while (!$done -and (Get-Date) -lt $deadline)
    if (!$done -or (Get-Content $log -Raw) -match 'Fatal error:|Assertion failed:|FPS_MAP_SMOKE FAIL') { throw 'Region capture failed.' }
    Write-Output 'Region screenshots captured; inspect the signs and boundaries visually.'
} finally { if (!$game.HasExited) { Stop-Process -Id $game.Id -ErrorAction SilentlyContinue } }
