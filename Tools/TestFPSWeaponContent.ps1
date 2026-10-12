param([string]$EngineRoot = 'D:\UE\UE_5.8', [string]$PackageExecutable = '', [string]$LogRoot = '', [switch]$Attachments)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$launchPrefix = @("`"$root/MCP_Test.uproject`"")
if ($PackageExecutable) { $exe = [System.IO.Path]::GetFullPath($PackageExecutable); $launchPrefix = @() }
if (!(Test-Path -LiteralPath $exe)) { throw "Unreal executable missing: $exe" }
if (!$LogRoot) { $LogRoot = Join-Path $root 'Saved/FPSFlowTests' }
$LogRoot = [System.IO.Path]::GetFullPath($LogRoot)
New-Item -ItemType Directory -Path $LogRoot -Force | Out-Null
$changes = if ($Attachments) { @('attachment') } else { @('capacity', 'interval') }
foreach ($change in $changes) {
    $serverLog = Join-Path $LogRoot "Content-$change-server.log"
    $clientLog = Join-Path $LogRoot "Content-$change-client.log"
    $owned = @()
    $started = Get-Date
    try {
        $common = @('-game', '-nullrhi', '-nosound', '-unattended', '-nosplash', '-FPSWeaponDamageSmoke', '-ExecCmds="t.MaxFPS 60"', '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
        if ($PackageExecutable) { $common += "-UserDir=`"$LogRoot/$change/User`"" }
        $owned += Start-Process -FilePath $exe -ArgumentList ($launchPrefix + @('/Game/FPSDemo/Maps/LV_FPSFoundry?game=/Script/MCP_Test.FPSMatchMode?Mode=LAN?listen', '-port=17781', "-abslog=`"$serverLog`"") + $common) -PassThru -WindowStyle Hidden
        Start-Sleep -Seconds 6
        $owned += Start-Process -FilePath $exe -ArgumentList ($launchPrefix + @('127.0.0.1:17781', "-FPSContentChange=$change", "-abslog=`"$clientLog`"") + $common) -PassThru -WindowStyle Hidden
        $deadline = (Get-Date).AddSeconds(60)
        do {
            Start-Sleep -Seconds 1
            $server = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
            $client = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
            $done = (Test-Path $clientLog) -and (Get-Item $clientLog).LastWriteTime -ge $started -and $server -match 'SERVER REJECT pawn=0 reason=Weapon content mismatch' -and $client -match 'FPS_WEAPON_CONTENT RETURN reason=Weapon content mismatch' -and $client -match 'FPS_WEAPON_CONTENT RELEASE definitions=7 handles=0'
        } while (!$done -and (Get-Date) -lt $deadline)
    } finally {
        foreach ($process in $owned) { if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue } }
    }
    if (!$done -or ($server + $client) -match 'Fatal error:|Assertion failed:' -or $server -match 'FPS_DAMAGE_SMOKE') { throw "Content rejection failed: $change" }
    Select-String -LiteralPath $serverLog,$clientLog -Pattern 'FPS_WEAPON_CONTENT'
    Write-Output "PASS: $change mismatch rejected before pawn, reason delivered and session handles released."
}
