param([string]$EngineRoot='D:\UE\UE_5.8',[Parameter(Mandatory=$true)][string]$RuntimeRoot,[switch]$Listen,[switch]$Visual,[int]$Latency=0)
$ErrorActionPreference='Stop'
$sourceRoot=Split-Path $PSScriptRoot -Parent
$run=Join-Path $sourceRoot ('Saved/HitDamageTests/'+(Get-Date -Format 'yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $run -Force | Out-Null
$exe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$module=Join-Path $RuntimeRoot 'Binaries/Win64/UnrealEditor-MCP_Test.dll'
$hash=(Get-FileHash -LiteralPath $module -Algorithm SHA256).Hash
$record=[ordered]@{started=(Get-Date).ToString('o');status='running';runtime=$RuntimeRoot;module_sha256=$hash;dedicated=(!$Listen);latency=$Latency;visual=[bool]$Visual}
$owned=@();$serverLog=Join-Path $run 'server.log';$clientLog=Join-Path $run 'shooter.log'
try {
    $common=@("`"$RuntimeRoot/MCP_Test.uproject`"",'-nullrhi','-nosound','-unattended','-nosplash','-Multiprocess','-NoLiveCoding','-FPSHitDamageSmoke','-ExecCmds="t.MaxFPS 60"','-DDC=InstalledNoZenLocalFallback','-NoZenAutoLaunch','-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
    if($Latency -gt 0){$common+="-PktLag=$Latency"}
    $url='/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=LAN?Bots=0'
    $serverArgs=if($Listen){@($url+'?listen','-game')}else{@($url,'-server')}
    $owned+=Start-Process -FilePath $exe -WindowStyle Hidden -PassThru -ArgumentList ($common+$serverArgs+@('-port=18852',"-abslog=`"$serverLog`""))
    Start-Sleep -Seconds 5
    if(!$Listen){
        $owned+=Start-Process -FilePath $exe -WindowStyle Hidden -PassThru -ArgumentList ($common+@('127.0.0.1:18852','-game',"-abslog=`"$run/target.log`""))
        Start-Sleep -Seconds 5
    }
    $clientArgs=$common
    if($Visual){$clientArgs=@($common|Where-Object{$_ -ne '-nullrhi'})+@('-RenderOffscreen','-ResX=1280','-ResY=720',"-FPSHitCapture=`"$run/HeadKill.png`"")}
    $owned+=Start-Process -FilePath $exe -WindowStyle Hidden -PassThru -ArgumentList ($clientArgs+@('127.0.0.1:18852','-game',"-abslog=`"$clientLog`""))
    $deadline=(Get-Date).AddSeconds(120)
    do {
        Start-Sleep -Seconds 1
        $server=if(Test-Path $serverLog){Get-Content -LiteralPath $serverLog -Raw}else{''}
        $client=if(Test-Path $clientLog){Get-Content -LiteralPath $clientLog -Raw}else{''}
        $done=$server -match 'FPS_HIT_SMOKE (PASS|FAIL) SERVER COMPLETE' -and $client -match 'FPS_HIT_SMOKE (PASS|FAIL) CLIENT COMPLETE'
        foreach($process in $owned){if($process.HasExited){throw "Test process exited early: $($process.Id)"}}
    }while(!$done -and (Get-Date) -lt $deadline)
    if(!$done -or ($server+$client) -match 'FPS_HIT_SMOKE FAIL|Fatal error:|Assertion failed:'){throw 'Hit damage verification failed or incomplete'}
    if($Visual){Start-Sleep -Seconds 2;if(!(Test-Path "$run/HeadKill.png")){throw 'Head kill screenshot missing'}}
    if((Get-FileHash -LiteralPath $module -Algorithm SHA256).Hash -ne $hash){throw 'Module changed during test'}
    $record.status='passed'
}catch{$record.status='failed';$record.error=$_.Exception.Message;throw}
finally{
    foreach($process in $owned){if(!$process.HasExited){Stop-Process -Id $process.Id -ErrorAction SilentlyContinue}}
    $record.finished=(Get-Date).ToString('o');$record|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
    Write-Output "Evidence: $run"
    Select-String -LiteralPath $serverLog,$clientLog -Pattern 'FPS_HIT_SMOKE' -ErrorAction SilentlyContinue
}
