param([string]$EngineRoot='D:\UE\UE_5.8',[string]$RunId=(Get-Date -Format 'yyyyMMdd-HHmmssfff'),[int]$Latency=0,[int]$PacketLoss=0,[switch]$Visual)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$logs=Join-Path $root "Saved/FPSAttachmentTests/$RunId"
if(Test-Path -LiteralPath $logs){throw "Evidence already exists: $logs"}
New-Item -ItemType Directory -Path $logs | Out-Null
$exe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$prefix=@("`"$root/MCP_Test.uproject`"")
$common=@('-game','-nullrhi','-unattended','-nosplash','-nosound','-FPSAttachmentSmoke','-ExecCmds="t.MaxFPS 60"','-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if($Latency -gt 0){$common+="-PktLag=$Latency"}
if($PacketLoss -gt 0){$common+="-PktLoss=$PacketLoss"}
$owned=@()
$observerStarted=$false
$moduleBefore=(Get-FileHash -LiteralPath (Join-Path $root 'Binaries/Win64/UnrealEditor-MCP_Test.dll')).Hash
@{moduleSha256=$moduleBefore;runId=$RunId;latency=$Latency;packetLoss=$PacketLoss;visual=[bool]$Visual;started=(Get-Date).ToString('o')} | ConvertTo-Json | Set-Content (Join-Path $logs 'Run.json')
try {
    $owned+=Start-Process -FilePath $exe -ArgumentList ($prefix+@('/Game/FPSDemo/Maps/LV_FPSTraining?game=/Script/MCP_Test.FPSMatchMode?Mode=LAN?Bots=0?listen','-port=17801',"-abslog=`"$logs/server.log`"")+$common) -WindowStyle Hidden -PassThru
    Start-Sleep -Seconds 6
    $clientOptions=$common
    if($Visual){$clientOptions=@($common | Where-Object {$_ -ne '-nullrhi'})+@('-RenderOffscreen','-ResX=1280','-ResY=900',"-FPSAttachmentCapture=`"$logs/Menu.png`"")}
    $owned+=Start-Process -FilePath $exe -ArgumentList ($prefix+@('127.0.0.1:17801',"-abslog=`"$logs/client.log`"")+$clientOptions) -WindowStyle Hidden -PassThru
    $deadline=(Get-Date).AddSeconds(180)
    do {
        Start-Sleep -Seconds 1
        $complete=$true
        foreach($side in @('server','client')){
            $path=Join-Path $logs "$side.log"
            $content=if(Test-Path -LiteralPath $path){Get-Content -LiteralPath $path -Raw}else{''}
            if($content -match 'Fatal error:|Assertion failed:|PreLogin failure:|NetworkFailure:|TravelFailure:'){throw "Runtime failure: $path"}
            if($side -eq 'server' -and !$observerStarted -and $content -match 'FPS_ATTACHMENT_LATE_WINDOW'){
                $observerOptions=@($common | Where-Object {$_ -ne '-FPSAttachmentSmoke'})+@('-FPSAttachmentObserver')
                $owned+=Start-Process -FilePath $exe -ArgumentList ($prefix+@('127.0.0.1:17801',"-abslog=`"$logs/observer.log`"")+$observerOptions) -WindowStyle Hidden -PassThru
                $observerStarted=$true
            }
            $complete=$complete -and ($content -match 'FPS_ATTACHMENT_SMOKE (PASS|FAIL) (SERVER|CLIENT) COMPLETE')
        }
        $observerText=if(Test-Path -LiteralPath "$logs/observer.log"){Get-Content -LiteralPath "$logs/observer.log" -Raw}else{''}
        $complete=$complete -and ($observerText -match 'FPS_ATTACHMENT_OBSERVER (PASS|FAIL) CLIENT COMPLETE')
        foreach($process in $owned){if($process.HasExited){throw "Process exited before completion: $($process.Id)"}}
    }while(!$complete -and (Get-Date) -lt $deadline)
    foreach($side in @('server','client')){
        $path=Join-Path $logs "$side.log"
        $content=Get-Content -LiteralPath $path -Raw
        Select-String -LiteralPath $path -Pattern 'FPS_ATTACHMENT_SMOKE'
        if($content -match 'FPS_ATTACHMENT_SMOKE FAIL' -or $content -notmatch 'FPS_ATTACHMENT_SMOKE PASS (SERVER|CLIENT) COMPLETE'){throw "Failed/incomplete: $path"}
    }
    if($Visual -and !(Test-Path -LiteralPath (Join-Path $logs 'Menu.png'))){throw 'Runtime UI screenshot missing'}
    if($observerText -match 'FPS_ATTACHMENT_OBSERVER FAIL|Fatal error:|Assertion failed:' -or $observerText -notmatch 'FPS_ATTACHMENT_OBSERVER PASS CLIENT COMPLETE'){throw 'Late observer failed or incomplete'}
    if((Get-FileHash -LiteralPath (Join-Path $root 'Binaries/Win64/UnrealEditor-MCP_Test.dll')).Hash -ne $moduleBefore){throw 'Module changed during run'}
    Write-Output "PASS: server/client attachment transactions; logs=$logs"
} finally {foreach($process in $owned){if(!$process.HasExited){Stop-Process -Id $process.Id -ErrorAction SilentlyContinue}}}
