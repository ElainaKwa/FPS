param([string]$EngineRoot='D:\UE\UE_5.8',[string]$RuntimeRoot='', [int[]]$Widths=@(1920,1280),[switch]$SkipBase,[switch]$SkipNetwork,[int]$Latency=0,[switch]$Visual)
$ErrorActionPreference='Stop'
$partsSource=Split-Path $PSScriptRoot -Parent
if(!$RuntimeRoot){$RuntimeRoot=$partsSource}
$partsRun=Join-Path $partsSource ('Saved/ModularPartsTests/'+(Get-Date -Format 'yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $partsRun | Out-Null
$partsExe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$partsHash=(Get-FileHash -LiteralPath (Join-Path $RuntimeRoot 'Binaries/Win64/UnrealEditor-MCP_Test.dll')).Hash
$partsRecord=[ordered]@{started=(Get-Date).ToString('o');runtime=$RuntimeRoot;module_sha256=$partsHash;status='running';runs=@()}
$partsCommon=@("`"$RuntimeRoot/MCP_Test.uproject`"",'-game','-unattended','-nosplash','-nosound','-Multiprocess','-DDC=InstalledNoZenLocalFallback','-NoZenAutoLaunch','-NoLiveCoding','-ExecCmds="t.MaxFPS 60"','-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
try {
    if(!$SkipBase){foreach($partsWidth in $Widths){
        $partsHeight=if($partsWidth -eq 1280){800}else{1080}
        $partsDir=Join-Path $partsRun "Base-$partsWidth";New-Item -ItemType Directory -Path $partsDir | Out-Null
        $partsProfile='Parts_'+(Split-Path $partsRun -Leaf)+"_$partsWidth"
        foreach($partsPhase in @('play','reload')){
            $partsLog=Join-Path $partsDir "$partsPhase.log"
            $partsFlag=if($partsPhase -eq 'play'){'-FPSModularPartsSmoke'}else{'-FPSModularPartsReload'}
            $partsArgs=$partsCommon+@('/Game/FPSDemo/Maps/Gameplay/FactoryBase/LV_FPSFactoryBase?game=/Script/MCP_Test.FPSFactoryMode?Mode=Solo?Bots=0?LabPhase=Base','-FactoryLobby',"-ExtractionProfile=$partsProfile","-FPSSettingsProfile=$partsProfile",$partsFlag,"-ModularEvidence=`"$partsDir`"","-abslog=`"$partsLog`"")
            if($partsPhase -eq 'play'){$partsArgs+=@('-RenderOffscreen','-Windowed','-ForceRes',"-ResX=$partsWidth","-ResY=$partsHeight")}else{$partsArgs+='-nullrhi'}
            $partsProcess=Start-Process -FilePath $partsExe -ArgumentList $partsArgs -WindowStyle Hidden -PassThru
            try{
                $partsDeadline=(Get-Date).AddSeconds(180)
                while(!$partsProcess.HasExited -and (Get-Date) -lt $partsDeadline){Start-Sleep -Seconds 1;$partsProcess.Refresh()}
                if(!$partsProcess.HasExited){throw 'Base validation timed out'}
                $partsProcess.WaitForExit();$partsText=Get-Content -LiteralPath $partsLog -Raw
                $partsMarker=if($partsPhase -eq 'play'){'FPS_MODULAR_BASE COMPLETE'}else{'FPS_MODULAR_BASE RELOAD COMPLETE'}
                if($partsProcess.ExitCode -ne 0 -or !$partsText.Contains($partsMarker) -or $partsText -match 'FPS_MODULAR_BASE FAIL|Fatal error:|Assertion failed:'){throw "Base $partsPhase failed: $partsLog"}
                $partsRecord.runs+=@{type='base';width=$partsWidth;height=$partsHeight;phase=$partsPhase;log=$partsLog;profile=$partsProfile;status='passed'}
                Write-Output "FPS_MODULAR_BASE PASS $partsPhase $partsWidth"
            }finally{if(!$partsProcess.HasExited){Stop-Process -Id $partsProcess.Id}}
        }
        if(!(Test-Path -LiteralPath (Join-Path $partsDir 'SixSlots.png'))){throw 'Six-slot screenshot missing'}
    }}
    if(!$SkipNetwork){
        $partsNet=Join-Path $partsRun 'Network';New-Item -ItemType Directory -Path $partsNet | Out-Null
        $partsArgs=$partsCommon+@('-nullrhi','-FPSAttachmentSmoke','-FPSModularPartsNetwork')
        if($Latency -gt 0){$partsArgs+="-PktLag=$Latency"}
        $partsOwned=@()
        try{
            $partsOwned+=Start-Process -FilePath $partsExe -ArgumentList ($partsArgs+@('/Game/FPSDemo/Maps/LV_FPSTraining?game=/Script/MCP_Test.FPSMatchMode?Mode=LAN?Bots=0?listen','-port=18864',"-abslog=`"$partsNet/server.log`"")) -WindowStyle Hidden -PassThru
            Start-Sleep -Seconds 5
            $partsClient=$partsArgs
            if($Visual){$partsClient=@($partsArgs|Where-Object{$_ -ne '-nullrhi'})+@('-RenderOffscreen','-ForceRes','-ResX=1280','-ResY=800',"-FPSModularCapture=`"$partsNet/WorldParts.png`"")}
            $partsOwned+=Start-Process -FilePath $partsExe -ArgumentList ($partsClient+@('127.0.0.1:18864',"-abslog=`"$partsNet/client.log`"")) -WindowStyle Hidden -PassThru
            Start-Sleep -Seconds 5
            $partsObserver=@($partsArgs|Where-Object{$_ -ne '-FPSAttachmentSmoke' -and $_ -ne '-FPSModularPartsNetwork'})+@('-FPSModularPartsObserver','127.0.0.1:18864',"-abslog=`"$partsNet/observer.log`"")
            $partsOwned+=Start-Process -FilePath $partsExe -ArgumentList $partsObserver -WindowStyle Hidden -PassThru
            $partsDeadline=(Get-Date).AddSeconds(110)
            do{
                Start-Sleep -Seconds 1;$partsComplete=$true
                foreach($partsSide in @('server','client','observer')){
                    $partsLog=Join-Path $partsNet "$partsSide.log";$partsText=if(Test-Path $partsLog){Get-Content -LiteralPath $partsLog -Raw}else{''}
                    if($partsText -match 'FPS_MODULAR_NET FAIL|Fatal error:|Assertion failed:|PreLogin failure:'){throw "Network failure: $partsLog"}
                    $partsComplete=$partsComplete -and ($partsText -match 'FPS_MODULAR_NET PASS (SERVER|CLIENT|OBSERVER) COMPLETE')
                }
                foreach($partsProcess in $partsOwned){if($partsProcess.HasExited){throw 'Network process exited early'}}
            }while(!$partsComplete -and (Get-Date) -lt $partsDeadline)
            if(!$partsComplete){throw 'Network test incomplete'}
            if($Visual -and !(Test-Path -LiteralPath "$partsNet/WorldParts.png")){throw 'Gameplay screenshot missing'}
            if($Visual -and !(Test-Path -LiteralPath "$partsNet/DeathParts.png")){throw 'Death screenshot missing'}
            $partsRecord.runs+=@{type='listen plus owner and observer';latency=$Latency;visual=[bool]$Visual;directory=$partsNet;status='passed'}
            Write-Output 'FPS_MODULAR_NET PASS server, owning client and observer'
        }finally{foreach($partsProcess in $partsOwned){if(!$partsProcess.HasExited){Stop-Process -Id $partsProcess.Id}}}
    }
    if((Get-FileHash -LiteralPath (Join-Path $RuntimeRoot 'Binaries/Win64/UnrealEditor-MCP_Test.dll')).Hash -ne $partsHash){throw 'Module changed during verification'}
    $partsRecord.status='passed'
}catch{$partsRecord.status='failed';$partsRecord.error=$_.Exception.Message;throw}
finally{$partsRecord.finished=(Get-Date).ToString('o');$partsRecord|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $partsRun 'run.json') -Encoding UTF8;Write-Output "Evidence: $partsRun"}
