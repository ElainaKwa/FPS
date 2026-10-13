param([string]$EngineRoot='D:\UE\UE_5.8',[Parameter(Mandatory=$true)][string]$Profile,[ValidateSet(1280,1920,2560)][int[]]$Widths=@(1280,1920,2560))
$ErrorActionPreference='Stop'
if($Profile -notmatch '^[A-Za-z0-9_-]{1,64}$'){throw 'Invalid profile name'}
$uiRoot=Split-Path $PSScriptRoot -Parent
$uiRun=Join-Path $uiRoot ('Saved/ExtractionTests/UI-'+(Get-Date -Format 'yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $uiRun -Force | Out-Null
$uiExe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$uiURL='/Game/FPSDemo/Maps/Gameplay/IndustrialCrossing/LV_FPSIndustrialCrossing?game=/Script/MCP_Test.FPSExtractionMode?Mode=Solo?Bots=0?LabPhase=Base'
$uiModule=Join-Path $uiRoot 'Binaries/Win64/UnrealEditor-MCP_Test.dll'
$uiHash=(Get-FileHash -LiteralPath $uiModule -Algorithm SHA256).Hash
$uiRecord=[ordered]@{started=(Get-Date).ToString('o');status='running';module_sha256=$uiHash;source_profile=$Profile;runs=@()}
try{
    foreach($uiWidth in $Widths){
        $uiHeight=if($uiWidth -eq 1280){800}else{[int]($uiWidth*9/16)};$uiLog=Join-Path $uiRun "$uiWidth-$uiHeight.log"
        $uiProfile='UI_'+(Split-Path $uiRun -Leaf).Substring(3)+"_$uiWidth"
        $uiCopied=0
        foreach($uiSlot in @(0,1)) {
            $uiSource=Join-Path $uiRoot "Saved/SaveGames/ExtractionLab_${Profile}_$uiSlot.sav"
            if(Test-Path -LiteralPath $uiSource){Copy-Item -LiteralPath $uiSource -Destination (Join-Path $uiRoot "Saved/SaveGames/ExtractionLab_${uiProfile}_$uiSlot.sav");$uiCopied++}
        }
        if(!$uiCopied){throw 'Source test profile has no snapshots'}
        $uiArgs=@("`"$uiRoot/MCP_Test.uproject`"",$uiURL,'-game','-nosound','-nosplash','-unattended','-RenderOffscreen','-windowed','-ForceRes',"-ResX=$uiWidth","-ResY=$uiHeight",
            '-DDC=InstalledNoZenLocalFallback','-NoZenAutoLaunch','-FPSExtractionUICapture',"-ExtractionProfile=$uiProfile",'-ExecCmds="t.MaxFPS 60"',"-ExtractionEvidence=`"$uiRun`"","-abslog=`"$uiLog`"",
            '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
        $uiProcess=Start-Process -FilePath $uiExe -WindowStyle Hidden -PassThru -ArgumentList $uiArgs
        try{
            $uiDeadline=(Get-Date).AddSeconds(120)
            while(!$uiProcess.HasExited -and (Get-Date) -lt $uiDeadline){Start-Sleep -Seconds 2;$uiProcess.Refresh()}
            if(!$uiProcess.HasExited){throw 'UI capture timed out'}
            $uiProcess.WaitForExit();$uiText=Get-Content -LiteralPath $uiLog -Raw
            if($uiProcess.ExitCode -ne 0 -or !$uiText.Contains('FPS_EXTRACTION_UI COMPLETE') -or $uiText -match 'FPS_EXTRACTION_UI FAIL|Fatal error:|Assertion failed:'){throw "UI validation failed: $uiLog"}
            foreach($uiPage in @('Overview','Warehouse','Production','Modification','ModificationOptic','ModificationEquipped','ModificationOrbit')){if(!(Test-Path -LiteralPath (Join-Path $uiRun "$uiPage-${uiWidth}x$uiHeight.png"))){throw "Missing screenshot: $uiPage"}}
            $uiRecord.runs+=@{width=$uiWidth;height=$uiHeight;exit_code=$uiProcess.ExitCode;log=$uiLog}
        }finally{if(!$uiProcess.HasExited){Stop-Process -Id $uiProcess.Id}}
    }
    if((Get-FileHash -LiteralPath $uiModule -Algorithm SHA256).Hash -ne $uiHash){throw 'Module changed during capture'}
    $uiRecord.status='passed';Write-Output "FPS_EXTRACTION_UI PASS $uiRun"
}catch{$uiRecord.status='failed';$uiRecord.error=$_.Exception.Message;throw}
finally{$uiRecord.finished=(Get-Date).ToString('o');$uiRecord | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $uiRun 'run.json') -Encoding UTF8}
