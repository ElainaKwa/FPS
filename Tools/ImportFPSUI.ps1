param()
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
& 'D:/UE/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' "$root/MCP_Test.uproject" -run=pythonscript "-script=$root/Tools/build_fps_ui_assets.py" -unattended -nop4 -nullrhi -nosound -DDC=InstalledNoZenLocalFallback "-LocalDataCachePath=$root/Saved/IndustrialArt/DDC" '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False' "-abslog=$root/Saved/IndustrialArt/UIImport.log" *> "$root/Saved/IndustrialArt/UIImport.stdout.log"
if($LASTEXITCODE -ne 0){throw "UI asset import failed: $LASTEXITCODE"}
if((Get-Content "$root/Saved/IndustrialArt/UIImport.log" -Raw) -notmatch 'FPS_UI_ASSETS PASS'){throw 'UI assets did not report success'}
Write-Output 'FPS_UI_ASSETS PASS'
