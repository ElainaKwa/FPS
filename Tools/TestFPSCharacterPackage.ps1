param(
    [Parameter(Mandatory = $true)][string]$PackageExecutable,
    [Parameter(Mandatory = $true)][ValidatePattern('^[0-9a-fA-F]{64}$')][string]$ExpectedExecutableSHA256,
    [ValidateSet('All','Drops','DropsLag','DropsLifetime','TrainingDrops','Loadout','TrainingLoadout',
        'Characters','Maps','Content','Weapons','Abilities','Teaching','Presentation','Audio')]
    [string]$Suite = 'All',
    [string]$OutputRoot = '',
    [ValidatePattern('^[A-Za-z0-9_-]+$')][string]$RunId = (Get-Date -Format 'yyyyMMdd-HHmmssfff')
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$acceptanceRoot = [System.IO.Path]::GetFullPath((Join-Path $root 'Saved/FPSCharacterAcceptance'))
if (!$OutputRoot) { $OutputRoot = Join-Path $acceptanceRoot 'Package' }
$OutputRoot = [System.IO.Path]::GetFullPath($OutputRoot)
if ($OutputRoot -ne $acceptanceRoot -and !$OutputRoot.StartsWith($acceptanceRoot + [System.IO.Path]::DirectorySeparatorChar,
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw 'Package evidence must stay inside Saved/FPSCharacterAcceptance.'
}
$PackageExecutable = (Resolve-Path -LiteralPath $PackageExecutable).Path
$expectedHash = $ExpectedExecutableSHA256.ToLowerInvariant()
if ((Get-FileHash -LiteralPath $PackageExecutable -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expectedHash) {
    throw 'Package executable SHA256 differs from the newly built package supplied for acceptance.'
}
$runRoot = Join-Path $OutputRoot $RunId
$powerShell = Join-Path $env:WINDIR 'System32/WindowsPowerShell/v1.0/powershell.exe'
function Get-FinishedEvidenceHash([string]$LiteralPath) {
    # Stop-Process returns before the OS necessarily releases every log handle.
    # Wait for a readable, stable file; never count an incomplete snapshot as evidence.
    $deadline = (Get-Date).AddSeconds(15)
    do {
        try {
            $before = Get-Item -LiteralPath $LiteralPath
            $hash = (Get-FileHash -LiteralPath $LiteralPath -Algorithm SHA256 -ErrorAction Stop).Hash.ToLowerInvariant()
            Start-Sleep -Milliseconds 250
            $after = Get-Item -LiteralPath $LiteralPath
            if ($before.Length -eq $after.Length -and $before.LastWriteTimeUtc -eq $after.LastWriteTimeUtc) { return $hash }
        } catch {
            if ((Get-Date) -ge $deadline) { throw }
        }
        Start-Sleep -Milliseconds 250
    } while ((Get-Date) -lt $deadline)
    throw "Evidence file remained locked or changed after harness completion: $LiteralPath"
}
# Suites are always sequential within this invocation, including all rendering.
# Separate NullRHI invocations may run concurrently only on different suite ports.
$definitions = [ordered]@{
    Drops = @{ Script='TestFPSDrops.ps1'; Args=@('-Visual','-Observer'); Folder='lag0-loss0-visual-observer';
        Logs=@('server.log','client.log','observer.log'); Patterns=@('FPS_DROP_SMOKE PASS SERVER COMPLETE\b',
            'FPS_DROP_SMOKE PASS CLIENT COMPLETE\b','FPS_DROP_OBSERVER PASS CLIENT COMPLETE\b');
        Captures=@('PhysicalDrop.png') }
    DropsLag = @{ Script='TestFPSDrops.ps1'; Args=@('-Latency','100','-PacketLoss','1','-Observer'); Folder='lag100-loss1-observer';
        Logs=@('server.log','client.log','observer.log'); Patterns=@('FPS_DROP_SMOKE PASS SERVER COMPLETE\b',
            'FPS_DROP_SMOKE PASS CLIENT COMPLETE\b','FPS_DROP_OBSERVER PASS CLIENT COMPLETE\b'); Captures=@() }
    DropsLifetime = @{ Script='TestFPSDrops.ps1'; Args=@('-Lifetime'); Folder='lag0-loss0-lifetime';
        Logs=@('server.log','client.log'); Patterns=@('FPS_DROP_SMOKE PASS SERVER COMPLETE\b',
            'FPS_DROP_SMOKE PASS CLIENT COMPLETE\b'); Captures=@() }
    TrainingDrops = @{ Script='TestFPSDrops.ps1'; Args=@('-Training','-Visual'); Folder='training-visual';
        Logs=@('server.log'); Patterns=@('FPS_DROP_SMOKE PASS SERVER COMPLETE\b'); Captures=@('DropLoadoutMenu.png') }
    Loadout = @{ Script='TestFPSLoadout.ps1'; Args=@(); Folder='lag0-loss0'; Logs=@('server.log','client.log');
        Patterns=@('FPS_LOADOUT_SMOKE PASS SERVER COMPLETE\b','FPS_LOADOUT_SMOKE PASS CLIENT COMPLETE\b'); Captures=@() }
    TrainingLoadout = @{ Script='TestFPSLoadout.ps1'; Args=@('-Training','-Visual'); Folder='training-visual';
        Logs=@('server.log'); Patterns=@('FPS_LOADOUT_SMOKE PASS SERVER COMPLETE\b'); Captures=@('LoadoutMenu.png') }
    Characters = @{ Script='TestFPSCharacters.ps1'; Args=@(); Folder='lag0-loss0'; Logs=@('server.log','client.log');
        Patterns=@('FPS_CHARACTER_SMOKE PASS SERVER COMPLETE\b','FPS_CHARACTER_SMOKE PASS CLIENT COMPLETE\b'); Captures=@() }
    Maps = @{ Script='TestFPSMaps.ps1'; Args=@(); Folder=''; Logs=@('Map-Solo.log','Map-Training.log');
        Patterns=@('FPS_MAP_SMOKE PASS map=LV_FPSFoundry\b','FPS_MAP_SMOKE PASS map=LV_FPSTraining\b');
        Captures=@('FoundryMap.png','TrainingMap.png') }
    Content = @{ Script='TestFPSWeaponContent.ps1'; Args=@(); Folder='';
        Logs=@('Content-capacity-server.log','Content-capacity-client.log','Content-interval-server.log','Content-interval-client.log');
        Patterns=@('FPS_WEAPON_CONTENT SERVER REJECT pawn=0 reason=Weapon content mismatch',
            'FPS_WEAPON_CONTENT RELEASE definitions=7 handles=0',
            'FPS_WEAPON_CONTENT SERVER REJECT pawn=0 reason=Weapon content mismatch',
            'FPS_WEAPON_CONTENT RELEASE definitions=7 handles=0'); Captures=@() }
    Weapons = @{ Script='TestFPSWeapons.ps1'; Args=@(); Folder=''; Logs=@('Weapons-server.log','Weapons-client.log');
        Patterns=@('FPS_WEAPON_SMOKE PASS SERVER COMPLETE\b','FPS_WEAPON_SMOKE PASS CLIENT COMPLETE\b'); Captures=@() }
    Abilities = @{ Script='TestFPSWeapons.ps1'; Args=@('-Abilities'); Folder=''; Logs=@('Abilities-server.log','Abilities-client.log');
        Patterns=@('FPS_ABILITY_SMOKE PASS SERVER COMPLETE\b','FPS_ABILITY_SMOKE PASS CLIENT COMPLETE\b'); Captures=@() }
    Teaching = @{ Script='TestFPSWeapons.ps1'; Args=@('-Teaching'); Folder=''; Logs=@('Teaching-server.log','Teaching-client.log');
        Patterns=@('FPS_TEACHING_SMOKE PASS SERVER COMPLETE\b','FPS_TEACHING_SMOKE PASS CLIENT COMPLETE\b'); Captures=@() }
    Presentation = @{ Script='TestFPSWeapons.ps1'; Args=@('-Presentation','-Visual'); Folder='';
        Logs=@('Presentation-server.log','Presentation-client.log','Presentation-observer.log');
        Patterns=@('FPS_PRESENTATION_SMOKE PASS SERVER COMPLETE\b','FPS_PRESENTATION_SMOKE PASS CLIENT COMPLETE\b',
            'FPS_PRESENTATION_OBSERVER PASS CLIENT COMPLETE\b'); Captures=@('WeaponPresentation.png') }
    Audio = @{ Script='TestFPSWeapons.ps1'; Args=@('-Audio'); Folder=''; Logs=@('Weapons-server.log','Weapons-client.log');
        Patterns=@('FPS_WEAPON_SMOKE PASS SERVER COMPLETE\b','FPS_WEAPON_SMOKE PASS CLIENT COMPLETE\b');
        Captures=@('Audio-server.wav','Audio-client.wav') }
}
$selected = if ($Suite -eq 'All') { @($definitions.Keys) } else { @($Suite) }
foreach ($name in $selected) {
    $definition = $definitions[$name]
    $suiteRoot = Join-Path $runRoot $name
    if (Test-Path -LiteralPath $suiteRoot) { throw "Refusing to overwrite an earlier acceptance run: $suiteRoot" }
    New-Item -ItemType Directory -Path $suiteRoot -Force | Out-Null
    $started = Get-Date
    $stdout = Join-Path $suiteRoot 'harness.stdout.log'
    $arguments = @('-NoProfile','-ExecutionPolicy','Bypass','-File',(Join-Path $PSScriptRoot $definition.Script),
        '-PackageExecutable',$PackageExecutable,'-LogRoot',$suiteRoot) + $definition.Args
    $record = [ordered]@{ suite=$name; started=$started.ToString('o'); executable=$PackageExecutable;
        executable_sha256=$expectedHash; arguments=$arguments; result='RUNNING'; exit_code=$null;
        completion_lines=@(); logs=@(); captures=@(); output=$suiteRoot }
    $recordFile = Join-Path $suiteRoot 'run.json'
    $record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $recordFile -Encoding UTF8
    try {
        if ((Get-FileHash -LiteralPath $PackageExecutable -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expectedHash) {
            throw 'Package executable changed before suite startup.'
        }
        Write-Output "FPS_CHARACTER_PACKAGE START suite=$name output=$suiteRoot"
        & $powerShell @arguments > $stdout 2>&1
        $record.exit_code = $LASTEXITCODE
        if ($record.exit_code -ne 0) {
            Get-Content -LiteralPath $stdout -Tail 25
            throw "Package suite child harness failed: $name exit=$($record.exit_code)"
        }
        $logDirectory = if ($definition.Folder) { Join-Path $suiteRoot $definition.Folder } else { $suiteRoot }
        for ($index = 0; $index -lt $definition.Logs.Count; $index++) {
            $log = Join-Path $logDirectory $definition.Logs[$index]
            if (!(Test-Path -LiteralPath $log) -or (Get-Item -LiteralPath $log).LastWriteTime -lt $started) {
                throw "Fresh package log missing: $log"
            }
            $data = Get-Content -LiteralPath $log -Raw
            if ($data -match 'FPS_[A-Z0-9_]+ FAIL|Fatal error:|Assertion failed:|TravelFailure:' -or
                $data -notmatch $definition.Patterns[$index]) { throw "Package result failed or incomplete: $log" }
            $record.completion_lines += @(Select-String -LiteralPath $log -Pattern $definition.Patterns[$index] | ForEach-Object { $_.Line })
            $record.logs += [ordered]@{ path=$log; sha256=(Get-FinishedEvidenceHash $log) }
        }
        # -UserDir in each packaged harness confines screenshots/audio to this new suite.
        foreach ($captureName in $definition.Captures) {
            $found = @(Get-ChildItem -LiteralPath $suiteRoot -Recurse -File | Where-Object {
                $_.Name -eq $captureName -and $_.LastWriteTime -ge $started -and $_.Length -gt 44
            })
            if ($found.Count -ne 1) { throw "Expected one fresh package capture: $captureName found=$($found.Count)" }
            $captureRoot = Join-Path $suiteRoot 'Captures'
            New-Item -ItemType Directory -Path $captureRoot -Force | Out-Null
            $target = Join-Path $captureRoot $captureName
            Copy-Item -LiteralPath $found[0].FullName -Destination $target
            $record.captures += [ordered]@{ source=$found[0].FullName; snapshot=$target;
                sha256=(Get-FinishedEvidenceHash $target) }
        }
        if ($name -eq 'Audio') {
            $waves = @($record.captures | ForEach-Object { $_.snapshot })
            & python (Join-Path $PSScriptRoot 'check_fps_audio.py') @waves > (Join-Path $suiteRoot 'audio-analysis.log') 2>&1
            if ($LASTEXITCODE -ne 0) { throw 'Package master audio is silent, clipped, invalid or missing.' }
            Get-Content -LiteralPath (Join-Path $suiteRoot 'audio-analysis.log')
        }
        if ((Get-FileHash -LiteralPath $PackageExecutable -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expectedHash) {
            throw 'Package executable changed during suite.'
        }
        $record.result = 'PASS'
        Write-Output "FPS_CHARACTER_PACKAGE PASS suite=$name"
    } catch {
        $record.result = 'FAIL'
        $record.error = $_.Exception.Message
        throw
    } finally {
        $record.finished = (Get-Date).ToString('o')
        $record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $recordFile -Encoding UTF8
    }
}
Write-Output "FPS_CHARACTER_PACKAGE COMPLETE suites=$($selected.Count) output=$runRoot"
