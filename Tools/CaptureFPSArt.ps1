param([ValidateSet('Lobby','Training','Match','Waiting','Report')][string]$Scene='Training',
    [int]$Width=1920,[int]$Height=1080,[switch]$Quick,[string]$PackageExecutable='',
    [string]$OutputRoot='Saved/IndustrialArt/Captures',[switch]$Interactions,[switch]$ScrollChecks,
    [ValidateSet('','MiSans','HarmonyOS')][string]$FontVariant='', [string]$MapOverride='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$output=Join-Path ([System.IO.Path]::GetFullPath((Join-Path $root $OutputRoot))) "${Scene}-${Width}x${Height}"
New-Item -ItemType Directory -Path $output -Force | Out-Null
$exe='D:/UE/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe'
$prefix=@("`"$root/MCP_Test.uproject`"")
if($PackageExecutable){$exe=$PackageExecutable;$prefix=@()}
$map=switch($Scene){
    'Lobby' {'/Engine/Maps/Entry?game=/Script/MCP_Test.FPSLobbyMode'}
    'Training' {'/Game/FPSDemo/Maps/LV_FPSTraining?game=/Script/MCP_Test.FPSMatchMode?Mode=Training'}
    'Match' {'/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=Solo?Bots=0'}
    'Waiting' {'/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=LAN?listen'}
    'Report' {'/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=Solo?Bots=0'}
}
$log=Join-Path $output 'Capture.log'
if($MapOverride){
    if($Scene -ne 'Match' -or !$MapOverride.StartsWith('/Game/')){throw 'MapOverride requires Scene Match and a /Game/ map path'}
    $map="${MapOverride}?game=/Script/MCP_Test.FPSMatchMode?Mode=Solo?Bots=0"
}
$arguments=$prefix+@($map,'-game','-RenderOffscreen','-windowed','-ForceRes',"-ResX=$Width","-ResY=$Height",'-nosound','-unattended','-nosplash','-FPSArtCapture',
    '-DDC=InstalledNoZenLocalFallback',"-LocalDataCachePath=`"$root/Saved/IndustrialArt/DDC`"",
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False',
    '-ExecCmds="t.MaxFPS 60"',"-FPSArtOutput=`"$output`"","-abslog=`"$log`"")
if($Quick){$arguments+='-FPSArtQuick'}
if($Interactions){$arguments+='-FPSArtInteractions'}
if($ScrollChecks){$arguments+='-FPSUIScroll'}
if($FontVariant){$arguments+="-FPSUIFontVariant=$FontVariant";$arguments+='-FPSArtFontCompare'}
if($Scene -eq 'Report'){$arguments+='-FPSArtReport'}
$start=Get-Date
$game=Start-Process -FilePath $exe -ArgumentList $arguments -PassThru -WindowStyle Hidden
try{
    $deadline=(Get-Date).AddSeconds(210)
    while(!$game.HasExited -and (Get-Date) -lt $deadline){Start-Sleep -Seconds 1}
    if(!$game.HasExited){throw 'Art capture timed out'}
    $text=Get-Content -LiteralPath $log -Raw
    if($text -notmatch 'FPS_ART_CAPTURE PASS sequence-complete' -or $text -match 'FPS_ART_CAPTURE FAIL|Fatal error:|Assertion failed:'){throw "Art capture failed: $log"}
    if(($Interactions -or $ScrollChecks) -and ($text -match 'FPS_UI_ACTION FAIL' -or $text -notmatch 'FPS_UI_ACTION PASS')){throw "UI button interaction check failed: $log"}
    $frames=@(Get-ChildItem -LiteralPath $output -Filter '*.png' | Where-Object {$_.LastWriteTime -ge $start})
    if(!$frames.Count){throw 'No fresh frame captures'}
    Write-Output "FPS_ART_CAPTURE PASS scene=$Scene size=${Width}x${Height} frames=$($frames.Count) output=$output"
}finally{if(!$game.HasExited){Stop-Process -Id $game.Id}}
