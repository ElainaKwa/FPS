param([string]$EngineRoot = 'D:\UE\UE_5.8', [string]$PackageExecutable = '', [string]$LogRoot = '', [switch]$Visual)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$prefix = @("`"$projectRoot\MCP_Test.uproject`"")
if ($PackageExecutable) { $exe = [System.IO.Path]::GetFullPath($PackageExecutable); $prefix = @() }
if (!(Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Unreal executable missing: $exe" }
if (!$LogRoot) { $LogRoot = Join-Path $projectRoot 'Saved\FPSCharacterTravelTests' }
$logs = [System.IO.Path]::GetFullPath($LogRoot)
New-Item -ItemType Directory -Path $logs -Force | Out-Null
$log = Join-Path $logs 'travel.log'
$started = Get-Date
[System.IO.File]::WriteAllText($log, '')
$userDir = Join-Path $logs ('User-' + $started.ToString('yyyyMMdd-HHmmss-fff'))
$arguments = $prefix + @('/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=Solo?Bots=0',
    '-game','-nosound','-unattended','-nosplash','-FPSCharacterTravelSmoke','-ExecCmds="t.MaxFPS 60"',
    "-UserDir=`"$userDir`"", "-abslog=`"$log`"",
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if ($Visual) { $arguments += @('-RenderOffscreen','-ResX=1280','-ResY=720') } else { $arguments += '-nullrhi' }
if (!$PackageExecutable) { $arguments += @('-DDC=InstalledNoZenLocalFallback','-DDC-ForceMemoryCache','-NoZenAutoLaunch') }
$binaryHashes = @([ordered]@{ path = $exe; sha256 = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant() })
if (!$PackageExecutable) {
    $module = Join-Path $projectRoot 'Binaries\Win64\UnrealEditor-MCP_Test.dll'
    if (Test-Path -LiteralPath $module) { $binaryHashes += [ordered]@{ path = $module; sha256 = (Get-FileHash -LiteralPath $module -Algorithm SHA256).Hash.ToLowerInvariant() } }
} else {
    $inner = Join-Path (Split-Path $exe -Parent) 'MCP_Test\Binaries\Win64\MCP_Test.exe'
    if ((Test-Path -LiteralPath $inner) -and $inner -ne $exe) { $binaryHashes += [ordered]@{ path = $inner; sha256 = (Get-FileHash -LiteralPath $inner -Algorithm SHA256).Hash.ToLowerInvariant() } }
}
$sourceHashes = @()
foreach ($relative in @('Source/MCP_Test/FPSCharacterTravelSmoke.h','Source/MCP_Test/FPSCharacterTravelSmoke.cpp','Source/MCP_Test/FPSFrontController.cpp','Tools/TestFPSCharacterTravel.ps1')) {
    $path = Join-Path $projectRoot $relative
    $sourceHashes += [ordered]@{ path = $relative; sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() }
}
$run = [ordered]@{ started = $started.ToString('o'); completed = $null; result = 'running'; process_id = $null;
    mode = 'Standalone same-process OpenLevel'; injection = 'Native P/Esc input, public selection, OpenLevel';
    seamless = $false; packaged = [bool]$PackageExecutable; visual = [bool]$Visual; user_dir = $userDir;
    arguments = $arguments; binaries = $binaryHashes; sources = $sourceHashes }
$record = Join-Path $logs 'run.json'
$run | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $record -Encoding UTF8
$owned = $null
try {
    $owned = Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
    $run.process_id = $owned.Id
    $run | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $record -Encoding UTF8
    $deadline = (Get-Date).AddSeconds(180)
    $complete = $false
    do {
        Start-Sleep -Seconds 1
        $text = Get-Content -LiteralPath $log -Raw
        if ($text -match 'FPS_CHARACTER_TRAVEL FAIL|Fatal error:|Assertion failed:|TravelFailure:|Failed to load package') { throw "Travel fixture failed: $log" }
        $complete = (Get-Item -LiteralPath $log).LastWriteTime -ge $started -and $text -match 'FPS_CHARACTER_TRAVEL PASS SERVER COMPLETE\b.*phases=5\b'
        if (!$complete -and $owned.HasExited) { throw "Travel process exited before completion: $($owned.Id)" }
    } while (!$complete -and (Get-Date) -lt $deadline)
    if (!$complete) { throw "Travel fixture deadline expired: $log" }
    Select-String -LiteralPath $log -Pattern 'FPS_CHARACTER_TRAVEL'
    $run.result = 'passed'
    Write-Output 'PASS: current OpenLevel path and native input injection across five same-process worlds; no seamless or hardware-click claim.'
} catch {
    $run.result = 'failed'; $run.error = $_.Exception.Message
    throw
} finally {
    if ($owned -and !$owned.HasExited) {
        Stop-Process -Id $owned.Id -ErrorAction SilentlyContinue
        [void]$owned.WaitForExit(5000)
    }
    $run.completed = (Get-Date).ToString('o')
    if (Test-Path -LiteralPath $log) { $run.log_sha256 = (Get-FileHash -LiteralPath $log -Algorithm SHA256).Hash.ToLowerInvariant() }
    $run | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $record -Encoding UTF8
}
