param([string]$EngineRoot='D:\UE\UE_5.8')
$ErrorActionPreference='Stop'
$artRoot=Split-Path $PSScriptRoot -Parent
$artExe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$artLog=Join-Path $artRoot 'Saved/IndustrialArt/Import.log'
$artCache=Join-Path $artRoot 'Saved/IndustrialArt/DDC'
New-Item -ItemType Directory -Path $artCache -Force | Out-Null
$artArgs=@("$artRoot/MCP_Test.uproject",'-run=pythonscript',"-script=$PSScriptRoot/import_industrial_art.py",'-unattended','-nop4','-nullrhi','-nosound','-DDC=InstalledNoZenLocalFallback',"-LocalDataCachePath=$artCache",'-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0','-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False',"-abslog=$artLog")
& $artExe @artArgs
if($LASTEXITCODE -ne 0){throw "Industrial asset import failed. See $artLog"}
$artReport=Get-Content -LiteralPath (Join-Path $artRoot 'Saved/IndustrialArt/ImportReport.json') -Raw | ConvertFrom-Json
if(-not $artReport.success){throw 'Import report did not confirm success'}
Write-Output "Saved $($artReport.assets.Count) mesh/texture assets and $($artReport.showcase)"
