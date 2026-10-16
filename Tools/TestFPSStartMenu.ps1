param([int]$Width=1920,[int]$Height=1080,[string]$EngineRoot='D:\UE\UE_5.8')
$ErrorActionPreference='Stop'
$startRoot=Split-Path $PSScriptRoot -Parent
$startRun=Join-Path $startRoot ('Saved/StartMenu/Tests/'+(Get-Date -Format 'yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $startRun -Force | Out-Null
$startProfile='StartMenu_'+(Split-Path $startRun -Leaf)
$startRuntime=Join-Path $startRun 'Runtime'
New-Item -ItemType Directory -Path (Join-Path $startRuntime 'Binaries/Win64') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $startRoot 'MCP_Test.uproject') -Destination $startRuntime
Copy-Item -LiteralPath (Join-Path $startRoot 'Config') -Destination $startRuntime -Recurse
$startModuleMap=Join-Path $startRoot 'Binaries/Win64/UnrealEditor.modules'
$startModuleName=(Get-Content -LiteralPath $startModuleMap -Raw | ConvertFrom-Json).Modules.MCP_Test
Copy-Item -LiteralPath $startModuleMap -Destination (Join-Path $startRuntime 'Binaries/Win64')
Copy-Item -LiteralPath (Join-Path $startRoot "Binaries/Win64/$startModuleName") -Destination (Join-Path $startRuntime 'Binaries/Win64')
New-Item -ItemType Junction -Path (Join-Path $startRuntime 'Content') -Target (Join-Path $startRoot 'Content') | Out-Null
$startModuleHash=(Get-FileHash -LiteralPath (Join-Path $startRuntime "Binaries/Win64/$startModuleName")).Hash
$startAssets=@('Content/FPSDemo/Art/UI/StartMenu/WBP_FPSStart.uasset','Content/FPSDemo/Art/Industrial/UI/Widgets/WBP_FPSLobby.uasset','Content/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal.uasset','Content/FPSDemo/Maps/Gameplay/FactoryBase/LV_FPSFactoryBase.umap','Content/FPSDemo/Maps/Gameplay/IndustrialCrossing/LV_FPSIndustrialCrossing.umap')
$startHashes=@{};foreach($startAsset in $startAssets){$startHashes[$startAsset]=(Get-FileHash -LiteralPath (Join-Path $startRoot $startAsset)).Hash}
$startAssets+=@(Get-ChildItem -LiteralPath (Join-Path $startRoot 'Content/FPSDemo/Weapons/Attachments') -Filter 'DA_M4_*.uasset' | ForEach-Object {$_.FullName.Substring($startRoot.Length+1)})
$startAssets+='Content/FPSDemo/Weapons/Definitions/DA_M4.uasset'
$startAssets+=@(Get-ChildItem -LiteralPath (Join-Path $startRoot 'Content/FPSDemo/Art/Weapons/Rifles/M4/Modular/Meshes') -Filter '*Gameplay*.uasset' | ForEach-Object {$_.FullName.Substring($startRoot.Length+1)})
foreach($startAsset in $startAssets){$startHashes[$startAsset]=(Get-FileHash -LiteralPath (Join-Path $startRoot $startAsset)).Hash}
$startLog=Join-Path $startRun 'Game.log'
$startRecord=[ordered]@{status='running';profile=$startProfile;resolution=@($Width,$Height);module_sha256=$startModuleHash;assets=$startHashes;started=(Get-Date).ToString('o');scope='Standalone Slate pointer routing and capture ownership, first-person factory/loadout/no-fire, raid spawn fire/damage and disabled base boundaries, map travel and isolated settlement. Host room tested; no second client, hardware input, PIE or packaged build claim.'}
$startExe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$startArguments=@("`"$startRuntime/MCP_Test.uproject`"",'/Engine/Maps/Entry?game=/Script/MCP_Test.FPSLobbyMode','-game','-windowed','-ForceRes',"-ResX=$Width","-ResY=$Height",'-RenderOffscreen','-unattended','-nosound','-nosplash','-DDC=InstalledNoZenLocalFallback','-NoZenAutoLaunch','-FPSStartMenuSmoke',"-ExtractionProfile=$startProfile","-FPSSettingsProfile=$startProfile",'-ExecCmds="t.MaxFPS 60"',"-GameUserSettingsINI=`"$startRun/GameUserSettings.ini`"","-StartMenuEvidence=`"$startRun`"","-abslog=`"$startLog`"","-LocalDataCachePath=`"$startRoot/Saved/IndustrialArt/DDC`"",'-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
$startProcess=$null
try {
 $startPriorSDK=[Environment]::GetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP','Process')
 try {[Environment]::SetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP','1','Process');$startProcess=Start-Process -FilePath $startExe -WindowStyle Hidden -PassThru -ArgumentList $startArguments} finally {[Environment]::SetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP',$startPriorSDK,'Process')}
 Write-Output "START_MENU_TEST_STARTED pid=$($startProcess.Id) output=$startRun"
 $startDeadline=(Get-Date).AddSeconds(240)
 while(!$startProcess.HasExited -and (Get-Date) -lt $startDeadline){Start-Sleep -Seconds 1;$startProcess.Refresh();if((Test-Path -LiteralPath $startLog) -and (Select-String -LiteralPath $startLog -Pattern 'Fatal error:|Assertion failed:' -Quiet)){throw 'Runtime failure; see Game.log'}}
 if(!$startProcess.HasExited){throw 'Start menu test timed out'}
 $startProcess.WaitForExit();$startText=Get-Content -LiteralPath $startLog -Raw
 if($startProcess.ExitCode -ne 0 -or $startText -match 'FPS_START_MENU FAIL|Fatal error:|Assertion failed:' -or !$startText.Contains('FPS_START_MENU PASS COMPLETE')){throw "Start menu test failed: $startLog"}
 foreach($startName in @('Start','Multiplayer','Host','Recovery','Inventory','Raid','Result','Returned','ReturnedStart','FactoryFirstPerson','ReturnedFactory','RaidFormerBase','RaidReloaded')){if(!(Test-Path -LiteralPath (Join-Path $startRun "$startName.png"))){throw "Missing screenshot $startName"}}
 foreach($startAsset in $startAssets){if((Get-FileHash -LiteralPath (Join-Path $startRoot $startAsset)).Hash -ne $startHashes[$startAsset]){throw "Asset changed during test: $startAsset"}}
 $startRecord.status='passed';Write-Output "FPS_START_MENU PASS $startRun"
 Select-String -LiteralPath $startLog -Pattern 'FPS_START_MENU ' | ForEach-Object Line
} catch {$startRecord.status='failed';$startRecord.error=$_.Exception.Message;throw}
finally {
 if($startProcess -and !$startProcess.HasExited){Stop-Process -Id $startProcess.Id}
 $startRecord.finished=(Get-Date).ToString('o')
 # Remove only this run's disposable runtime, unlinking Content before recursion.
 $startResolved=[IO.Path]::GetFullPath($startRuntime)
 if($startResolved -ne ([IO.Path]::GetFullPath($startRun)+'\Runtime')){throw 'Unexpected cleanup path'}
 $startLink=Get-Item -LiteralPath (Join-Path $startResolved 'Content')
 if($startLink.LinkType -ne 'Junction'){throw 'Expected runtime Content junction'}
 $startLink.Delete()
 Remove-Item -LiteralPath $startResolved -Recurse -Force
 $startRecord.runtime_cleaned=$true
 $startRecord | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $startRun 'run.json') -Encoding UTF8
}
