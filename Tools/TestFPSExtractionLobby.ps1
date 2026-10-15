param([int]$Width=1920,[int]$Height=1080,[string]$EngineRoot='D:\UE\UE_5.8',[string]$WeaponContentRoot='',[string]$ModulePath='')
$ErrorActionPreference='Stop'
$lobbyRoot=Split-Path $PSScriptRoot -Parent
$lobbyRun=Join-Path $lobbyRoot ('Saved/FPSExtraction/UI/LobbyTests/'+(Get-Date -Format 'yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $lobbyRun -Force | Out-Null
$lobbyProfile='Lobby_'+(Split-Path $lobbyRun -Leaf)
$lobbyRuntime=Join-Path $lobbyRun 'Runtime'
New-Item -ItemType Directory -Path (Join-Path $lobbyRuntime 'Binaries/Win64') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $lobbyRoot 'MCP_Test.uproject') -Destination $lobbyRuntime
Copy-Item -LiteralPath (Join-Path $lobbyRoot 'Config') -Destination $lobbyRuntime -Recurse
$lobbyModuleMap=Join-Path $lobbyRoot 'Binaries/Win64/UnrealEditor.modules'
$lobbyModuleName=(Get-Content -LiteralPath $lobbyModuleMap -Raw | ConvertFrom-Json).Modules.MCP_Test
Copy-Item -LiteralPath $lobbyModuleMap -Destination (Join-Path $lobbyRuntime 'Binaries/Win64')
if(!$ModulePath){$ModulePath=Join-Path $lobbyRoot "Binaries/Win64/$lobbyModuleName"}
$ModulePath=(Resolve-Path -LiteralPath $ModulePath).Path
Copy-Item -LiteralPath $ModulePath -Destination (Join-Path $lobbyRuntime "Binaries/Win64/$lobbyModuleName")
$lobbyModule=Join-Path $lobbyRuntime "Binaries/Win64/$lobbyModuleName"
if(!$WeaponContentRoot){New-Item -ItemType Junction -Path (Join-Path $lobbyRuntime 'Content') -Target (Join-Path $lobbyRoot 'Content') | Out-Null}
else {
 $WeaponContentRoot=(Resolve-Path -LiteralPath $WeaponContentRoot).Path
 # Only the isolated copy receives alternate weapon definitions. The main Content is untouched.
 New-Item -ItemType Directory -Path (Join-Path $lobbyRuntime 'Content/FPSDemo/Weapons') -Force | Out-Null
 foreach($lobbyTop in Get-ChildItem -LiteralPath (Join-Path $lobbyRoot 'Content') -Directory){if($lobbyTop.Name -ne 'FPSDemo'){New-Item -ItemType Junction -Path (Join-Path $lobbyRuntime "Content/$($lobbyTop.Name)") -Target $lobbyTop.FullName | Out-Null}}
 foreach($lobbyPart in Get-ChildItem -LiteralPath (Join-Path $lobbyRoot 'Content/FPSDemo') -Directory){if($lobbyPart.Name -ne 'Weapons'){New-Item -ItemType Junction -Path (Join-Path $lobbyRuntime "Content/FPSDemo/$($lobbyPart.Name)") -Target $lobbyPart.FullName | Out-Null}}
 foreach($lobbyPart in Get-ChildItem -LiteralPath $WeaponContentRoot -Directory){
  $lobbyDestination=Join-Path $lobbyRuntime "Content/FPSDemo/Weapons/$($lobbyPart.Name)"
  if($lobbyPart.Name -in @('Definitions','Damage')){Copy-Item -LiteralPath $lobbyPart.FullName -Destination $lobbyDestination -Recurse}
  else{New-Item -ItemType Junction -Path $lobbyDestination -Target $lobbyPart.FullName | Out-Null}
 }
}
$lobbyHash=(Get-FileHash -LiteralPath $lobbyModule -Algorithm SHA256).Hash
$lobbyAsset=Join-Path $lobbyRoot 'Content/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal.uasset'
$lobbyAssetHash=(Get-FileHash -LiteralPath $lobbyAsset -Algorithm SHA256).Hash
$lobbyLog=Join-Path $lobbyRun 'Game.log'
$lobbyRecord=[ordered]@{status='running';profile=$lobbyProfile;runtime=$lobbyRuntime;weapon_content_override=$WeaponContentRoot;module_source=$ModulePath;module_sha256=$lobbyHash;asset_sha256=$lobbyAssetHash;resolution=@($Width,$Height);started=(Get-Date).ToString('o');scope='Standalone, actual Slate pointer routing through saved buttons; isolated profile and test settlement. No hardware input, PIE, cook or multiplayer claim.'}
$lobbyExe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$lobbyArguments=@("`"$lobbyRoot/MCP_Test.uproject`"",'/Game/FPSDemo/Maps/Gameplay/FactoryBase/LV_FPSFactoryBase?game=/Script/MCP_Test.FPSFactoryMode?Mode=Solo?Bots=0','-game','-windowed','-ForceRes',"-ResX=$Width","-ResY=$Height",'-RenderOffscreen','-unattended','-nosound','-nosplash','-DDC=InstalledNoZenLocalFallback','-NoZenAutoLaunch','-FactoryLobby','-FPSLobbySmoke',"-ExtractionProfile=$lobbyProfile","-FPSSettingsProfile=$lobbyProfile",'-ExecCmds="t.MaxFPS 60"',"-GameUserSettingsINI=`"$lobbyRun/GameUserSettings.ini`"","-LobbyEvidence=`"$lobbyRun`"","-abslog=`"$lobbyLog`"",'-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
$lobbyArguments[0]="`"$lobbyRuntime/MCP_Test.uproject`""
$lobbyProcess=$null
$lobbyArguments += "-LocalDataCachePath=`"$lobbyRoot/Saved/IndustrialArt/DDC`""
try {
 $lobbyPriorSDK=[Environment]::GetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP','Process')
 try { [Environment]::SetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP','1','Process');$lobbyProcess=Start-Process -FilePath $lobbyExe -WindowStyle Hidden -PassThru -ArgumentList $lobbyArguments } finally { [Environment]::SetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP',$lobbyPriorSDK,'Process') }
 Write-Output "LOBBY_TEST_STARTED pid=$($lobbyProcess.Id) output=$lobbyRun"
 $lobbyDeadline=(Get-Date).AddSeconds(210)
 while(!$lobbyProcess.HasExited -and (Get-Date) -lt $lobbyDeadline){Start-Sleep -Seconds 1;$lobbyProcess.Refresh();if((Test-Path -LiteralPath $lobbyLog) -and (Select-String -LiteralPath $lobbyLog -Pattern 'Fatal error:|Assertion failed:' -Quiet)){throw 'Runtime failure; see Game.log'}}
 if(!$lobbyProcess.HasExited){throw 'Lobby test timed out'}
 $lobbyProcess.WaitForExit();$lobbyText=Get-Content -LiteralPath $lobbyLog -Raw
 if($lobbyProcess.ExitCode -ne 0 -or $lobbyText -match 'FPS_LOBBY FAIL|Fatal error:|Assertion failed:' -or !$lobbyText.Contains('FPS_LOBBY PASS COMPLETE')){throw "Lobby test failed: $lobbyLog"}
 foreach($lobbyName in @('Lobby','Warehouse','Workbench','Settings','Production','Factory','Raid','Returned')){if(!(Test-Path -LiteralPath (Join-Path $lobbyRun "$lobbyName.png"))){throw "Missing screenshot $lobbyName"}}
 if((Get-FileHash -LiteralPath $lobbyModule -Algorithm SHA256).Hash -ne $lobbyHash){throw 'Module changed during test'}
 if((Get-FileHash -LiteralPath $lobbyAsset -Algorithm SHA256).Hash -ne $lobbyAssetHash){throw 'Lobby asset changed during test'}
 $lobbyRecord.status='passed';Write-Output "FPS_LOBBY PASS $lobbyRun"
 Select-String -LiteralPath $lobbyLog -Pattern 'FPS_LOBBY ' | ForEach-Object Line
} catch {$lobbyRecord.status='failed';$lobbyRecord.error=$_.Exception.Message;throw}
finally {
 if($lobbyProcess -and !$lobbyProcess.HasExited){Stop-Process -Id $lobbyProcess.Id}
 $lobbyRecord.finished=(Get-Date).ToString('o');$lobbyRecord | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $lobbyRun 'run.json') -Encoding UTF8
}
