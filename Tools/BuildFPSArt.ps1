param([string]$RunId=(Get-Date -Format 'yyyyMMdd-HHmmss'))
$ErrorActionPreference='Stop'
if($RunId -notmatch '^[A-Za-z0-9_-]+$'){throw 'Invalid RunId'}
$root=Split-Path $PSScriptRoot -Parent
$engine='D:/UE/UE_5.8'
$logs=Join-Path $root "Saved/IndustrialArt/Package/$RunId"
$archive=Join-Path $root "Saved/Packages/IndustrialArt/$RunId"
if(Test-Path -LiteralPath $logs){throw "Evidence already exists: $logs"}
New-Item -ItemType Directory -Path $logs -Force | Out-Null
function Inputs {
    $directories=@('Source','Config','Content/FPSDemo/Art/Industrial','Content/FPSDemo/Maps','Content/FPSDemo/Weapons/Definitions','ArtSource/FPSDemo/Industrial/UI/Fonts')
    @($directories | ForEach-Object {Get-ChildItem -LiteralPath (Join-Path $root $_) -Recurse -File} | Sort-Object FullName | ForEach-Object {
        [ordered]@{path=$_.FullName.Substring($root.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}
    })
}
$before=@(Inputs)
$arguments=@('BuildCookRun',"-project=$root/MCP_Test.uproject",'-noP4','-platform=Win64','-clientconfig=Development','-build','-skipbuildeditor','-cook',
    '-AdditionalCookerOptions=-SkipZenStore','-stage','-pak','-archive',"-archivedirectory=$archive",'-unattended','-utf8output')
$record=[ordered]@{started=(Get-Date).ToString('o');result='RUNNING';archive=$archive;arguments=$arguments;inputs=$before;exit_code=$null}
try{
    & "$engine/Engine/Build/BatchFiles/RunUAT.bat" @arguments 2>&1 | Tee-Object -FilePath (Join-Path $logs 'BuildCookRun.log')
    $record.exit_code=$LASTEXITCODE
    if($LASTEXITCODE -ne 0){throw "BuildCookRun failed: $LASTEXITCODE"}
    if(($before | ConvertTo-Json -Depth 4 -Compress) -cne (@(Inputs) | ConvertTo-Json -Depth 4 -Compress)){throw 'Build inputs changed during packaging'}
    $exe=Join-Path $archive 'Windows/MCP_Test/Binaries/Win64/MCP_Test.exe'
    if(!(Test-Path -LiteralPath $exe)){throw 'Packaged executable missing'}
    $notices=Join-Path $archive 'Windows/ThirdPartyNotices/FPSUIFonts'
    New-Item -ItemType Directory -Path $notices -Force | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $root 'ArtSource/FPSDemo/Industrial/UI/Fonts') -File | Where-Object {$_.Extension -in @('.txt','.pdf')} | Copy-Item -Destination $notices
    $record.font_licenses=@(Get-ChildItem -LiteralPath $notices -File | ForEach-Object { [ordered]@{path=$_.FullName;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash} })
    $record.result='PASS';$record.executable=$exe;$record.executable_sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
    Write-Output "FPS_ART_PACKAGE PASS executable=$exe"
}catch{$record.result='FAIL';$record.error=$_.Exception.Message;throw}
finally{$record.finished=(Get-Date).ToString('o');$record | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $logs 'Report.json') -Encoding UTF8}
