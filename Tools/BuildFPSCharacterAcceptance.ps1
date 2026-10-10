param([string]$EngineRoot = 'D:\UE\UE_5.8',
    [ValidatePattern('^[A-Za-z0-9_-]+$')][string]$RunId = (Get-Date -Format 'yyyyMMdd-HHmmssfff'))
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$project = Join-Path $root 'MCP_Test.uproject'
$archive = Join-Path $root 'Saved/Packages/CharacterAcceptance'
$logs = Join-Path $root "Saved/FPSCharacterAcceptance/Build/$RunId"
if (Test-Path -LiteralPath $logs) { throw "Refusing to overwrite earlier build evidence: $logs" }
New-Item -ItemType Directory -Path $logs -Force | Out-Null
function Read-BuildInputs {
    $paths = @(Get-ChildItem -LiteralPath (Join-Path $root 'Source') -Recurse -File) +
        @(Get-ChildItem -LiteralPath (Join-Path $root 'Config') -Recurse -File) + @(Get-Item -LiteralPath $project)
    @($paths | Sort-Object FullName | ForEach-Object {
        [ordered]@{ path=$_.FullName.Substring($root.Length + 1).Replace('\','/');
            sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
    })
}
$inputs = @(Read-BuildInputs)
$buildArgs = @('MCP_TestEditor','Win64','Development',"-Project=$project",'-WaitMutex','-NoHotReload')
$uatArgs = @('BuildCookRun',"-project=$project",'-noP4','-platform=Win64','-clientconfig=Development',
    '-build','-skipbuildeditor','-cook','-AdditionalCookerOptions=-SkipZenStore',
    '-stage','-pak','-archive',"-archivedirectory=$archive",'-unattended','-utf8output')
$record = [ordered]@{ started=(Get-Date).ToString('o'); project=$project; engine=$EngineRoot;
    engine_version=(Get-Content -LiteralPath (Join-Path $EngineRoot 'Engine/Build/Build.version') -Raw | ConvertFrom-Json);
    archive=$archive; build_arguments=$buildArgs; package_arguments=$uatArgs; input_files=$inputs;
    editor_exit_code=$null; package_exit_code=$null; result='RUNNING'; artifacts=@() }
$recordFile = Join-Path $logs 'run.json'
$record | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $recordFile -Encoding UTF8
try {
    & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') @buildArgs 2>&1 |
        Tee-Object -FilePath (Join-Path $logs 'EditorBuild.log')
    $record.editor_exit_code = $LASTEXITCODE
    if ($record.editor_exit_code -ne 0) { throw "Editor build failed: $($record.editor_exit_code)" }
    Copy-Item -LiteralPath (Join-Path $root 'Saved/Logs/FPSWeaponBuildValidation.log') -Destination (Join-Path $logs 'WeaponBuildValidation.log')
    & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat') @uatArgs 2>&1 |
        Tee-Object -FilePath (Join-Path $logs 'BuildCookRun.log')
    $record.package_exit_code = $LASTEXITCODE
    if ($record.package_exit_code -ne 0) { throw "BuildCookRun failed: $($record.package_exit_code)" }
    $after = @(Read-BuildInputs)
    if (($inputs | ConvertTo-Json -Depth 5 -Compress) -cne ($after | ConvertTo-Json -Depth 5 -Compress)) {
        throw 'Source or project configuration changed during build; the resulting package cannot identify one frozen version.'
    }
    $paths = @((Join-Path $root 'Binaries/Win64/UnrealEditor-MCP_Test.dll'),
        (Join-Path $archive 'Windows/MCP_Test/Binaries/Win64/MCP_Test.exe'))
    $paths += @(Get-ChildItem -LiteralPath (Join-Path $archive 'Windows/MCP_Test/Content/Paks') -File | ForEach-Object { $_.FullName })
    foreach ($path in $paths) {
        $item = Get-Item -LiteralPath $path
        $record.artifacts += [ordered]@{ path=$item.FullName; bytes=$item.Length; last_modified=$item.LastWriteTime.ToString('o');
            sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() }
    }
    $record.result = 'PASS'
    Write-Output "FPS_CHARACTER_BUILD PASS output=$logs archive=$archive"
} catch {
    $record.result = 'FAIL'; $record.error = $_.Exception.Message
    throw
} finally {
    $record.finished = (Get-Date).ToString('o')
    $record | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $recordFile -Encoding UTF8
}
