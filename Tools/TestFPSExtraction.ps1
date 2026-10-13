param([string]$EngineRoot='D:\UE\UE_5.8', [switch]$Visual)
$ErrorActionPreference='Stop'
$labRoot=Split-Path $PSScriptRoot -Parent
$labRunId=Get-Date -Format 'yyyyMMdd-HHmmssfff'
$labOutput=Join-Path $labRoot "Saved/ExtractionTests/$labRunId"
New-Item -ItemType Directory -Path $labOutput -Force | Out-Null
$labExe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$labURL='/Game/FPSDemo/Maps/Gameplay/IndustrialCrossing/LV_FPSIndustrialCrossing?game=/Script/MCP_Test.FPSExtractionMode?Mode=Solo?Bots=0?LabPhase=Base'
$labModuleName=(Get-Content (Join-Path $labRoot 'Binaries/Win64/UnrealEditor.modules') -Raw | ConvertFrom-Json).Modules.MCP_Test
$labModulePath=Join-Path $labRoot "Binaries/Win64/$labModuleName"
function Read-LabInputs {
    @((Get-ChildItem -LiteralPath (Join-Path $labRoot 'Source') -Recurse -File)+(Get-ChildItem -LiteralPath (Join-Path $labRoot 'Config') -Recurse -File)+(Get-Item -LiteralPath (Join-Path $labRoot 'MCP_Test.uproject')) |
        Sort-Object FullName | ForEach-Object {@{path=$_.FullName.Substring($labRoot.Length+1);sha256=(Get-FileHash -LiteralPath $_.FullName).Hash}})
}
$labRecord=[ordered]@{started=(Get-Date).ToString('o');status='running';module=$labModuleName;module_sha256=(Get-FileHash -LiteralPath $labModulePath).Hash;inputs=@(Read-LabInputs);visual=[bool]$Visual;runs=@()}
try {
    foreach($labStage in @('loop','reload','combat')) {
        $labLog=Join-Path $labOutput "$labStage.log"
        $labFlag=switch($labStage){'loop'{'-FPSExtractionSmoke'}'reload'{'-FPSExtractionReloadCheck'}'combat'{'-FPSExtractionCombatCheck'}}
        $labProfile=if($labStage -eq 'combat'){"Combat_$labRunId"}else{"Test_$labRunId"}
        $labArgs=@("`"$labRoot/MCP_Test.uproject`"",$labURL,'-game','-nosound','-unattended','-nosplash','-DDC=InstalledNoZenLocalFallback','-NoZenAutoLaunch',
            '-ExecCmds="t.MaxFPS 60"',"-ExtractionProfile=$labProfile",$labFlag,"-abslog=`"$labLog`"",
            '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
        if($Visual){$labArgs+=@('-RenderOffscreen','-windowed','-ForceRes','-ResX=1600','-ResY=900','-FPSExtractionCapture',"-ExtractionEvidence=`"$labOutput`"")}
        else{$labArgs+='-nullrhi'}
        $labProcess=Start-Process -FilePath $labExe -WindowStyle Hidden -PassThru -ArgumentList $labArgs
        try {
            $labDeadline=(Get-Date).AddSeconds(240)
            while(!$labProcess.HasExited -and (Get-Date) -lt $labDeadline){Start-Sleep -Seconds 2;$labProcess.Refresh()}
            if(!$labProcess.HasExited){throw "Extraction $labStage timed out"}
            $labProcess.WaitForExit()
            $labText=Get-Content -LiteralPath $labLog -Raw
            $labMarker=switch($labStage){'loop'{'FPS_EXTRACTION_TEST COMPLETE'}'reload'{'FPS_EXTRACTION_TEST RELOAD COMPLETE'}'combat'{'FPS_EXTRACTION_COMBAT COMPLETE'}}
            if($labProcess.ExitCode -ne 0 -or !$labText.Contains($labMarker) -or $labText -match 'FPS_EXTRACTION_(TEST|COMBAT) FAIL|Fatal error:|Assertion failed:'){throw "Extraction $labStage failed; inspect $labLog"}
            $labRecord.runs+=@{stage=$labStage;exit_code=$labProcess.ExitCode;log=$labLog;sha256=(Get-FileHash -LiteralPath $labLog).Hash}
            Select-String -LiteralPath $labLog -Pattern 'FPS_EXTRACTION_TEST'
        } finally {if(!$labProcess.HasExited){Stop-Process -Id $labProcess.Id}}
    }
    if((Get-FileHash -LiteralPath $labModulePath).Hash -ne $labRecord.module_sha256){throw 'Module changed while testing'}
    if(($labRecord.inputs | ConvertTo-Json -Compress) -cne (@(Read-LabInputs) | ConvertTo-Json -Compress)){throw 'Source/config changed while testing'}
    if($Visual){foreach($labImage in @('Base','Raid','Success','Upgraded','Failure')){if(!(Test-Path -LiteralPath (Join-Path $labOutput "$labImage.png"))){throw "Missing capture $labImage"}}}
    $labRecord.status='passed'
    Write-Output "FPS_EXTRACTION_TEST PASS $labOutput"
} catch {$labRecord.status='failed';$labRecord.error=$_.Exception.Message;throw}
finally {$labRecord.finished=(Get-Date).ToString('o');$labRecord | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $labOutput 'run.json') -Encoding UTF8}
