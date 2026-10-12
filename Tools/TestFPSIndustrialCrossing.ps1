param([string]$EngineRoot='D:\UE\UE_5.8', [switch]$LAN, [switch]$Visual)
$ErrorActionPreference='Stop'
$crossingRoot=Split-Path $PSScriptRoot -Parent
$crossingExe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$crossingRun=Join-Path $crossingRoot ('Saved/IndustrialCrossing/Tests/'+(Get-Date -Format 'yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $crossingRun -Force | Out-Null
$crossingServerLog=Join-Path $crossingRun 'server.log'
$crossingClientLog=Join-Path $crossingRun 'client.log'
$crossingOwned=@()
$crossingStarted=Get-Date
$crossingMode=if($LAN){'LAN'}else{'Solo'}
$crossingBots=if($LAN){0}else{9}
$crossingURL="/Game/FPSDemo/Maps/Gameplay/IndustrialCrossing/LV_FPSIndustrialCrossing?game=/Script/MCP_Test.FPSMatchMode?Mode=${crossingMode}?Bots=${crossingBots}"
if($LAN){$crossingURL+='?listen'}
$crossingArgs=@('-game','-nosound','-unattended','-nosplash','-DDC=InstalledNoZenLocalFallback','-NoZenAutoLaunch',
    '-ExecCmds="t.MaxFPS 60"','-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if($Visual){$crossingArgs+=@('-RenderOffscreen','-windowed','-ForceRes','-ResX=1600','-ResY=900')}else{$crossingArgs+='-nullrhi'}
if($LAN){$crossingArgs+='-FPSFlowSmoke'}else{
    $crossingArgs+='-FPSLargeMapSmoke'
    if($Visual){$crossingArgs+='-FPSLargeMapCapture'}
}
$crossingModule=(Get-Content (Join-Path $crossingRoot 'Binaries/Win64/UnrealEditor.modules') -Raw | ConvertFrom-Json).Modules.MCP_Test
$crossingRecord=[ordered]@{started=$crossingStarted.ToString('o');mode=$crossingMode;visual=[bool]$Visual;status='running';
    map=$crossingURL;module=$crossingModule;module_sha256=(Get-FileHash -LiteralPath (Join-Path $crossingRoot "Binaries/Win64/$crossingModule")).Hash;logs=$crossingRun}
try {
    $crossingOwned+=Start-Process -FilePath $crossingExe -WindowStyle Hidden -PassThru -ArgumentList (@("`"$crossingRoot/MCP_Test.uproject`"",$crossingURL,'-port=17831',"-abslog=`"$crossingServerLog`"")+ $crossingArgs)
    if($LAN){
        Start-Sleep -Seconds 7
        $crossingOwned+=Start-Process -FilePath $crossingExe -WindowStyle Hidden -PassThru -ArgumentList (@("`"$crossingRoot/MCP_Test.uproject`"",'127.0.0.1:17831',"-abslog=`"$crossingClientLog`"")+ $crossingArgs)
    }
    $crossingDeadline=(Get-Date).AddSeconds(360)
    $crossingComplete=$false
    do {
        Start-Sleep -Seconds 2
        $crossingText=if(Test-Path -LiteralPath $crossingServerLog){Get-Content -LiteralPath $crossingServerLog -Raw}else{''}
        if($crossingText -match 'FPS_LARGE_MAP FAIL|FPS_SMOKE FAIL|MATCH BLOCKED|Fatal error:|Assertion failed:'){throw 'Map runtime validation failed; inspect server.log'}
        if($LAN){
            $crossingGuest=if(Test-Path -LiteralPath $crossingClientLog){Get-Content -LiteralPath $crossingClientLog -Raw}else{''}
            $crossingComplete=$crossingText -match 'FPS_SMOKE PASS finished round preserves death view' -and $crossingGuest -match 'SNAPSHOT CLIENT waiting=0 finished=1 players=2 podium=2.*score=1'
        }else{$crossingComplete=$crossingText -match 'FPS_LARGE_MAP PASS COMPLETE'}
        if(!$crossingComplete -and $crossingOwned[0].HasExited){throw 'Game exited before completing verification'}
    }while(!$crossingComplete -and (Get-Date) -lt $crossingDeadline)
    if(!$crossingComplete){throw 'Map validation timed out'}
    if($LAN){
        foreach($marker in @('damage-flash CLIENT','death-info CLIENT','respawn-reset CLIENT')){
            if(!$crossingGuest.Contains("FPS_FEEDBACK PASS $marker")){throw "Missing replicated feedback: $marker"}
        }
    }elseif($Visual){
        foreach($index in 0..4){
            $crossingFrame=Join-Path $crossingRoot "Saved/IndustrialCrossing/CrossingCapture$index.png"
            if(!(Test-Path -LiteralPath $crossingFrame) -or (Get-Item -LiteralPath $crossingFrame).LastWriteTime -lt $crossingStarted){throw "Fresh screenshot missing: $index"}
        }
    }
    $crossingRecord.status='passed'
    Select-String -LiteralPath $crossingServerLog -Pattern 'FPS_LARGE_MAP|FPS_SMOKE (PASS|FAIL)'
    Write-Output "INDUSTRIAL_CROSSING TEST PASS $crossingRun"
}catch{$crossingRecord.status='failed';$crossingRecord.error=$_.Exception.Message;throw}
finally{
    foreach($crossingProcess in $crossingOwned){if(!$crossingProcess.HasExited){Stop-Process -Id $crossingProcess.Id -ErrorAction SilentlyContinue}}
    $crossingRecord.completed=(Get-Date).ToString('o')
    $crossingRecord | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $crossingRun 'run.json') -Encoding UTF8
}
