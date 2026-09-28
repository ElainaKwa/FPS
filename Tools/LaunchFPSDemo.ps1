param([string]$EngineRoot = 'D:\UE\UE_5.8')
$projectFile = Join-Path (Split-Path $PSScriptRoot -Parent) 'MCP_Test.uproject'
$editorExe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
if (!(Test-Path -LiteralPath $editorExe)) { throw "UnrealEditor.exe not found: $editorExe" }
# This is an interactive game window explicitly launched by the user.
Start-Process -FilePath $editorExe -ArgumentList @("`"$projectFile`"", '/Engine/Maps/Entry?game=/Script/MCP_Test.FPSLobbyMode', '-game', '-windowed', '-ResX=1280', '-ResY=800')
