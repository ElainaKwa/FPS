param([string]$EngineRoot='D:\UE\UE_5.8', [string]$RunId=(Get-Date -Format 'yyyyMMdd-HHmmssfff'), [switch]$CaptureOnly, [string]$DataCachePath='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$output=Join-Path $root "Saved/FPSAnimationTests/$RunId"
if(Test-Path -LiteralPath $output){throw "Refusing to overwrite evidence: $output"}
New-Item -ItemType Directory -Path $output | Out-Null
$exe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$module=Join-Path $root 'Binaries/Win64/UnrealEditor-MCP_Test.dll'
if(!$DataCachePath){$DataCachePath=Join-Path $root 'Saved/IndustrialArt/DDC'}
$record=[ordered]@{result='RUNNING';started=(Get-Date).ToString('o');module_sha256=(Get-FileHash -LiteralPath $module).Hash;arguments=@();error=$null}
$common=@("`"$root/MCP_Test.uproject`"",'-unattended','-nosound','-nosplash','-DDC=InstalledNoZenLocalFallback',
    "-LocalDataCachePath=`"$DataCachePath`"",
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
function Run([string[]]$Arguments,[int]$Timeout=180) {
    $record.arguments+=,@($Arguments)
    $prior=[Environment]::GetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP','Process')
    try {
        [Environment]::SetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP','1','Process')
        $process=Start-Process -FilePath $exe -ArgumentList $Arguments -WindowStyle Hidden -PassThru
    } finally { [Environment]::SetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP',$prior,'Process') }
    try {
        $deadline=(Get-Date).AddSeconds($Timeout)
        while(!$process.HasExited -and (Get-Date) -lt $deadline){Start-Sleep -Milliseconds 500}
        if(!$process.HasExited){throw 'Animation validation timed out'}
        if($process.ExitCode -ne 0){throw "Animation validation exited $($process.ExitCode)"}
    } finally {if(!$process.HasExited){Stop-Process -Id $process.Id}}
}
try {
    if(!$CaptureOnly) {
        $log=Join-Path $output 'Automation.log'
        Run ($common+@('-nullrhi','-ExecCmds="Automation RunTests FPS.Animation"','-TestExit="Automation Test Queue Empty"',
            "-ReportExportPath=`"$output/Automation`"","-abslog=`"$log`""))
        $text=Get-Content -LiteralPath $log -Raw
        if($text -notmatch 'Result=\{Success\}.*MotionAndActions' -or $text -notmatch 'Result=\{Success\}.*ReloadAssets' -or $text -match 'Result=\{Fail|Fatal error:|Assertion failed:'){throw 'Animation or reload-asset automation failed or did not run'}
    }
    $log=Join-Path $output 'Capture.log'
    Run ($common+@('/Game/FPSDemo/Maps/LV_FPSTraining?game=/Script/MCP_Test.FPSMatchMode?Mode=Training','-game',
        '-RenderOffscreen','-windowed','-ForceRes','-ResX=960','-ResY=540','-FPSAnimationCapture',
        # PNG capture can take longer than a short reload phase. Keep simulation sampling at 60 Hz.
        '-UseFixedTimeStep','-FPS=60','-ExecCmds="t.MaxFPS 60"',
        "-FPSAnimationOutput=`"$output/Frames`"","-abslog=`"$log`"")) 300
    $text=Get-Content -LiteralPath $log -Raw
    if($text -notmatch 'FPS_ANIMATION_CAPTURE PASS COMPLETE' -or $text -match 'FPS_ANIMATION_CAPTURE FAIL|Fatal error:|Assertion failed:'){throw 'Actual-world animation checks failed or incomplete'}
    $frames=@(Get-ChildItem -LiteralPath (Join-Path $output 'Frames') -Filter '*.png')
    if($frames.Count -lt 100){throw "Too few captured frames: $($frames.Count)"}
    if((Get-FileHash -LiteralPath $module).Hash -ne $record.module_sha256){throw 'Module changed during validation'}
    $record.result='PASS';$record.frames=$frames.Count
    Write-Output "FPS_ANIMATION_TEST PASS output=$output frames=$($frames.Count)"
} catch {$record.result='FAIL';$record.error=$_.Exception.Message;throw}
finally {$record.finished=(Get-Date).ToString('o');$record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'run.json') -Encoding UTF8}
