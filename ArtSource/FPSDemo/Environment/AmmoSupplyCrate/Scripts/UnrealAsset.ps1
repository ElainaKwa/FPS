param([ValidateSet('Import','Capture')][string]$Action='Import',[string]$EngineRoot='D:/UE/UE_5.8')
$ErrorActionPreference='Stop'
$assetRoot=Split-Path $PSScriptRoot -Parent
$projectRoot=[IO.Path]::GetFullPath((Join-Path $assetRoot '../../../..'))
$logRoot=Join-Path $projectRoot 'Saved/AmmoSupplyCrate'
New-Item -ItemType Directory -Path $logRoot -Force | Out-Null
$assetArgs=@("$projectRoot/MCP_Test.uproject",'-unattended','-nop4','-nosound','-DDC=InstalledNoZenLocalFallback',"-LocalDataCachePath=$projectRoot/Saved/IndustrialArt/DDC",'-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False',"-abslog=$logRoot/$Action.log")
if($Action -eq 'Import') {
    $assetArgs+=@('-run=pythonscript',"-script=$PSScriptRoot/import_crate.py",'-nullrhi','-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0')
} else {
    $assetArgs+=@("-ExecutePythonScript=$PSScriptRoot/capture_crate.py",'-RenderOffscreen','-nosplash','-windowed','-ResX=1600','-ResY=1200')
}
& "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" @assetArgs *> "$logRoot/$Action.stdout.log"
if($LASTEXITCODE -ne 0){throw "Crate $Action failed: $LASTEXITCODE"}
$marker=if($Action -eq 'Import'){'AMMO_CRATE_IMPORT_PASS'}else{'AMMO_CRATE_CAPTURE_PASS'}
if(!(Select-String -LiteralPath "$logRoot/$Action.log" -Pattern $marker -Quiet)){throw "Missing success marker: $marker"}
Write-Output "$marker"
