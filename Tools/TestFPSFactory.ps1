param([string]$EngineRoot='D:\UE\UE_5.8')
$ErrorActionPreference='Stop'
$factoryRoot=Split-Path $PSScriptRoot -Parent
$factoryRun=Join-Path $factoryRoot ('Saved/FactoryBase/Tests/'+(Get-Date -Format 'yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $factoryRun -Force | Out-Null
$factoryProfile='Factory_'+(Split-Path $factoryRun -Leaf)
$factoryDll=Join-Path $factoryRoot 'Binaries/Win64/UnrealEditor-MCP_Test.dll'
$factoryHash=(Get-FileHash -LiteralPath $factoryDll -Algorithm SHA256).Hash
$factoryLog=Join-Path $factoryRun 'Game.log'
$factoryRecord=[ordered]@{status='running';profile=$factoryProfile;module_sha256=$factoryHash;started=(Get-Date).ToString('o')}
$factoryExe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$factoryArgs=@("`"$factoryRoot/MCP_Test.uproject`"",'/Game/FPSDemo/Maps/Gameplay/FactoryBase/LV_FPSFactoryBase?game=/Script/MCP_Test.FPSFactoryMode?Mode=Solo?Bots=0','-game','-windowed','-ForceRes','-ResX=1600','-ResY=900','-RenderOffscreen','-unattended','-nosound','-nosplash','-DDC=InstalledNoZenLocalFallback','-NoZenAutoLaunch','-FPSFactorySmoke',"-ExtractionProfile=$factoryProfile","-FPSSettingsProfile=$factoryProfile",'-ExecCmds="t.MaxFPS 60"',"-FactoryEvidence=`"$factoryRun`"","-abslog=`"$factoryLog`"",'-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
$factoryProcess=$null
try {
 $factoryProcess=Start-Process -FilePath $factoryExe -WindowStyle Hidden -PassThru -ArgumentList $factoryArgs
 $factoryDeadline=(Get-Date).AddSeconds(180)
 while(!$factoryProcess.HasExited -and (Get-Date) -lt $factoryDeadline){Start-Sleep -Seconds 2;$factoryProcess.Refresh()}
 if(!$factoryProcess.HasExited){throw 'Factory smoke timed out'}
 $factoryProcess.WaitForExit();$factoryText=Get-Content -LiteralPath $factoryLog -Raw
 if($factoryProcess.ExitCode -ne 0 -or $factoryText -match 'FPS_FACTORY FAIL|Fatal error:|Assertion failed:' -or !$factoryText.Contains('FPS_FACTORY COMPLETE')){throw "Factory validation failed: $factoryLog"}
 foreach($factoryName in @('FactoryArrival','FactoryWarehouse','FactoryProduction','FactoryGunsmith','FactoryDeparture','FactoryWalkable','FactoryWorkshop','FactoryBench','FactoryFabrication','FactoryStorage')){if(!(Test-Path -LiteralPath (Join-Path $factoryRun "$factoryName.png"))){throw "Missing screenshot $factoryName"}}
 if((Get-FileHash -LiteralPath $factoryDll -Algorithm SHA256).Hash -ne $factoryHash){throw 'Module changed while testing; repeat after build finishes'}
 $factoryRecord.status='passed';Write-Output "FPS_FACTORY PASS $factoryRun"
} catch {$factoryRecord.status='failed';$factoryRecord.error=$_.Exception.Message;throw}
finally {
 if($factoryProcess -and !$factoryProcess.HasExited){Stop-Process -Id $factoryProcess.Id}
 $factoryRecord.finished=(Get-Date).ToString('o');$factoryRecord | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $factoryRun 'run.json') -Encoding UTF8
}
