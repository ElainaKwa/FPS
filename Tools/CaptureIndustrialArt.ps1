param([string]$EngineRoot='D:\UE\UE_5.8')
$ErrorActionPreference='Stop'
$artRoot=Split-Path $PSScriptRoot -Parent
$artExe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$artLog=Join-Path $artRoot 'Saved/IndustrialArt/Capture.log'
$artCache=Join-Path $artRoot 'Saved/IndustrialArt/DDC'
New-Item -ItemType Directory -Path $artCache -Force | Out-Null
$artArgs=@("$artRoot/MCP_Test.uproject","-ExecutePythonScript=$PSScriptRoot/capture_industrial_art.py",'-RenderOffscreen','-unattended','-nop4','-nosound','-nosplash','-windowed','-ResX=1600','-ResY=1000','-DDC=InstalledNoZenLocalFallback',"-LocalDataCachePath=$artCache",'-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False',"-abslog=$artLog")
& $artExe @artArgs
if($LASTEXITCODE -ne 0){throw "Industrial capture failed. See $artLog"}
if(-not (Select-String -LiteralPath $artLog -Pattern 'INDUSTRIAL_CAPTURE PASS' -Quiet)){throw "Capture did not report success. See $artLog"}
Write-Output 'Saved real Unreal preview: ArtSource/FPSDemo/Industrial/Previews/UnrealShowcase.png'
