param([string]$EngineRoot='D:\UE\UE_5.8', [switch]$CreateM4, [switch]$SelfTest, [switch]$Export, [string]$RunId=(Get-Date -Format 'yyyyMMdd-HHmmssfff'))
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$output=Join-Path $root "Saved/FPSAttachmentTests/$RunId"
if (Test-Path -LiteralPath $output) { throw "Evidence directory already exists: $output" }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$log=Join-Path $output 'Data.log'
$arguments=@("`"$root/MCP_Test.uproject`"",'-run=FPSAttachmentData','-unattended','-nullrhi','-nosound',"-abslog=`"$log`"",'-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if($CreateM4){$arguments+='-CreateM4'}
if($SelfTest){$arguments+='-SelfTest'}
if($Export){$arguments+='-Export'}
$process=Start-Process -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $arguments -WindowStyle Hidden -PassThru
$process.WaitForExit()
$text=Get-Content -LiteralPath $log -Raw
Select-String -LiteralPath $log -Pattern 'FPS_ATTACHMENT_(DATA|TEST|CONTENT)'
if($process.ExitCode -ne 0 -or $text -match 'FPS_ATTACHMENT_(DATA|TEST) FAIL|Fatal error:|Assertion failed:'){throw "Attachment commandlet failed: $log"}
if($SelfTest){if($text -notmatch 'FPS_ATTACHMENT_TEST PASS COMPLETE'){throw 'Self-test completion missing'}}
elseif($text -notmatch 'FPS_ATTACHMENT_DATA PASS'){throw 'Data validation completion missing'}
Write-Output "PASS: attachment data evidence=$output"
