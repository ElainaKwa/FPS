param([string]$EngineRoot='D:\UE\UE_5.8', [string]$RunId=(Get-Date -Format 'yyyyMMdd-HHmmssfff'), [string]$RecoveryFixture='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$output=Join-Path $root "Saved/FPSAttachmentTests/$RunId"
if(Test-Path -LiteralPath $output){throw "Evidence directory already exists: $output"}
New-Item -ItemType Directory -Path $output -Force | Out-Null
$log=Join-Path $output 'Editor.log'
$arguments=@("`"$root/MCP_Test.uproject`"",'-unattended','-nosplash','-nosound','-RenderOffscreen','-NoVSync','-ExecCmds="FPS.WorkbenchSmoke"','-FPSWorkbenchExit',"-FPSWorkbenchOutput=`"$output`"","-abslog=`"$log`"",'-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if($RecoveryFixture){$arguments += "-FPSWorkbenchCleanup=`"$RecoveryFixture`""}
$process=Start-Process -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe') -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    $deadline=(Get-Date).AddSeconds(240)
    while(!$process.HasExited -and (Get-Date) -lt $deadline){Start-Sleep -Seconds 1}
    if(!$process.HasExited){throw "Workbench editor timed out: $log"}
    $text=Get-Content -LiteralPath $log -Raw
    Select-String -LiteralPath $log -Pattern 'FPS_WORKBENCH_TEST'
    if($process.ExitCode -ne 0 -or $text -match 'FPS_WORKBENCH_TEST FAIL|Fatal error:|Assertion failed:' -or $text -notmatch 'FPS_WORKBENCH_TEST PASS COMPLETE'){throw "Workbench test failed: $log"}
    foreach($name in @('Weapons.png','Attachments.png','Preview.png','Validation.png')){if(!(Test-Path -LiteralPath (Join-Path $output $name))){throw "Screenshot missing: $name"}}
    Write-Output "PASS: actual editor authoring and screenshots=$output"
} finally { if(!$process.HasExited){Stop-Process -Id $process.Id -ErrorAction SilentlyContinue} }
