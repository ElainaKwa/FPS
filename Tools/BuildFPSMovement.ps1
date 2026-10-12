param([string]$EngineRoot='D:\UE\UE_5.8', [string]$RunId=(Get-Date -Format 'yyyyMMdd-HHmmssfff'))
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$output=Join-Path $root "Saved/FPSMovementTests/Build/$RunId"
if(Test-Path -LiteralPath $output){throw "Refusing to overwrite build evidence: $output"}
New-Item -ItemType Directory -Path $output -Force | Out-Null
function Read-Inputs {
    $files=@(Get-ChildItem -LiteralPath (Join-Path $root 'Source') -Recurse -File)+
        @(Get-ChildItem -LiteralPath (Join-Path $root 'Config') -Recurse -File)+@(Get-Item -LiteralPath (Join-Path $root 'MCP_Test.uproject'))
    @($files | Sort-Object FullName | ForEach-Object {
        [ordered]@{path=$_.FullName.Substring($root.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
    })
}
$arguments=@('MCP_TestEditor','Win64','Development',"-Project=$(Join-Path $root 'MCP_Test.uproject')",'-WaitMutex','-NoHotReload')
$record=[ordered]@{started=(Get-Date).ToString('o');engine_version=(Get-Content (Join-Path $EngineRoot 'Engine/Build/Build.version') -Raw | ConvertFrom-Json);
    inputs=@(Read-Inputs);arguments=$arguments;result='RUNNING';exit_code=$null}
$recordPath=Join-Path $output 'run.json'
$record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $recordPath -Encoding UTF8
try {
    & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') @arguments 2>&1 | Tee-Object -FilePath (Join-Path $output 'Build.log')
    $record.exit_code=$LASTEXITCODE
    if($record.exit_code -ne 0){throw "Editor build failed: $($record.exit_code)"}
    if(($record.inputs | ConvertTo-Json -Depth 5 -Compress) -cne (@(Read-Inputs) | ConvertTo-Json -Depth 5 -Compress)){
        throw 'Build inputs changed while compiling; this is not a frozen build.'
    }
    $module=Get-Item -LiteralPath (Join-Path $root 'Binaries/Win64/UnrealEditor-MCP_Test.dll')
    $record.module=[ordered]@{path=$module.FullName;sha256=(Get-FileHash -LiteralPath $module.FullName).Hash.ToLowerInvariant();modified=$module.LastWriteTime.ToString('o')}
    $record.result='PASS'
    Write-Output "FPS_MOVEMENT_BUILD PASS output=$output"
} catch { $record.result='FAIL';$record.error=$_.Exception.Message;throw }
finally { $record.finished=(Get-Date).ToString('o');$record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $recordPath -Encoding UTF8 }
