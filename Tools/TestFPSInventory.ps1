param([string]$EngineRoot='D:\UE\UE_5.8',[ValidateSet(1280,1920)][int[]]$Widths=@(1280,1920),[string]$ContentRoot='',[string]$ModulePath='')
$ErrorActionPreference='Stop'
$inventoryRoot=Split-Path $PSScriptRoot -Parent
$inventoryContent=if($ContentRoot){(Resolve-Path -LiteralPath $ContentRoot).Path}else{Join-Path $inventoryRoot 'Content'}
$inventoryRun=Join-Path $inventoryRoot ('Saved/InventoryTests/'+(Get-Date -Format 'yyyyMMdd-HHmmssfff'))
$inventoryRuntime=Join-Path $inventoryRun 'Runtime'
New-Item -ItemType Directory -Path (Join-Path $inventoryRuntime 'Binaries/Win64') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $inventoryRoot 'MCP_Test.uproject') -Destination $inventoryRuntime
Copy-Item -LiteralPath (Join-Path $inventoryRoot 'Config') -Destination $inventoryRuntime -Recurse
$inventoryModules=Join-Path $inventoryRoot 'Binaries/Win64/UnrealEditor.modules'
$inventoryModuleName=(Get-Content -LiteralPath $inventoryModules -Raw | ConvertFrom-Json).Modules.MCP_Test
$inventorySourceModule=if($ModulePath){(Resolve-Path -LiteralPath $ModulePath).Path}else{Join-Path $inventoryRoot "Binaries/Win64/$inventoryModuleName"}
Copy-Item -LiteralPath $inventoryModules -Destination (Join-Path $inventoryRuntime 'Binaries/Win64')
Copy-Item -LiteralPath $inventorySourceModule -Destination (Join-Path $inventoryRuntime "Binaries/Win64/$inventoryModuleName")
New-Item -ItemType Junction -Path (Join-Path $inventoryRuntime 'Content') -Target $inventoryContent | Out-Null
$inventoryInputs=@('FPSExtractionGrid.cpp','FPSExtractionGridUI.h','FPSExtractionGridUI.cpp','FPSExtractionSplitDialog.cpp','FPSExtractionInventorySmoke.cpp','FPSExtractionGridSmoke.cpp','FPSExtractionSubsystem.h','FPSExtractionSubsystem.cpp','FPSExtractionMode.cpp','FPSExtractionUI.cpp','FPSFactoryBase.cpp')
$inventoryHashes=@($inventoryInputs | ForEach-Object { $inventorySource=Join-Path $inventoryRoot "Source/MCP_Test/$_"; @{path=$inventorySource;sha256=(Get-FileHash -LiteralPath $inventorySource).Hash} })
$inventoryAssetFiles=@((Get-ChildItem -LiteralPath (Join-Path $inventoryContent 'FPSDemo/Art/UI/ExtractionTerminal') -Recurse -File)+(Get-ChildItem -LiteralPath (Join-Path $inventoryContent 'FPSDemo/Weapons') -Recurse -File)+(Get-ChildItem -LiteralPath (Join-Path $inventoryContent 'FPSDemo/Maps/Gameplay/FactoryBase') -Recurse -File))
$inventoryAssets=@($inventoryAssetFiles | ForEach-Object { @{path=$_.FullName;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash} })
$inventoryModule=Join-Path $inventoryRuntime "Binaries/Win64/$inventoryModuleName"
$inventoryHash=(Get-FileHash -LiteralPath $inventoryModule).Hash
$inventoryRecord=[ordered]@{status='running';started=(Get-Date).ToString('o');runtime=$inventoryRuntime;content_root=$inventoryContent;source_module=$inventorySourceModule;module_sha256=$inventoryHash;inputs=$inventoryHashes;assets=$inventoryAssets;runs=@()}
$inventoryExe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$inventoryMap='/Game/FPSDemo/Maps/Gameplay/FactoryBase/LV_FPSFactoryBase?game=/Script/MCP_Test.FPSFactoryMode?Mode=Solo?Bots=0?LabPhase=Base'
$inventoryRecord.entry_map=$inventoryMap
try {
    foreach($inventoryWidth in $Widths) {
        $inventoryHeight=if($inventoryWidth -eq 1280){800}else{1080}
        $inventoryEvidence=Join-Path $inventoryRun "$inventoryWidth"
        New-Item -ItemType Directory -Path $inventoryEvidence -Force | Out-Null
        $inventoryProfile='Inventory_'+(Split-Path $inventoryRun -Leaf)+"_$inventoryWidth"
        foreach($inventoryStage in @('play','reload')) {
            $inventoryLog=Join-Path $inventoryEvidence "$inventoryStage.log"
            $inventoryFlag=if($inventoryStage -eq 'play'){'-FPSInventorySmoke'}else{'-FPSInventoryReload'}
            $inventoryArgs=@("`"$inventoryRuntime/MCP_Test.uproject`"",$inventoryMap,'-game','-unattended','-nosplash','-nosound','-Multiprocess','-DDC=InstalledNoZenLocalFallback','-NoZenAutoLaunch',
                '-FactoryLobby','-ExecCmds="t.MaxFPS 60"',"-ExtractionProfile=$inventoryProfile","-FPSSettingsProfile=$inventoryProfile",$inventoryFlag,"-InventoryEvidence=`"$inventoryEvidence`"","-abslog=`"$inventoryLog`"",
                '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
            if($inventoryStage -eq 'play'){$inventoryArgs+=@('-RenderOffscreen','-windowed','-ForceRes',"-ResX=$inventoryWidth","-ResY=$inventoryHeight")}
            else{$inventoryArgs+='-nullrhi'}
            $inventoryPriorSDK=[Environment]::GetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP','Process')
            try {
                [Environment]::SetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP','1','Process')
                $inventoryProcess=Start-Process -FilePath $inventoryExe -WindowStyle Hidden -PassThru -ArgumentList $inventoryArgs
            } finally {[Environment]::SetEnvironmentVariable('UE_SKIP_UBT_SDK_SETUP',$inventoryPriorSDK,'Process')}
            try {
                $inventoryDeadline=(Get-Date).AddSeconds(180)
                while(!$inventoryProcess.HasExited -and (Get-Date) -lt $inventoryDeadline){Start-Sleep -Seconds 1;$inventoryProcess.Refresh()}
                if(!$inventoryProcess.HasExited){throw 'Inventory validation timed out'}
                $inventoryProcess.WaitForExit()
                $inventoryText=Get-Content -LiteralPath $inventoryLog -Raw
                $inventoryMarker=if($inventoryStage -eq 'play'){'FPS_INVENTORY COMPLETE'}else{'FPS_INVENTORY RELOAD COMPLETE'}
                if($inventoryProcess.ExitCode -ne 0 -or !$inventoryText.Contains($inventoryMarker) -or $inventoryText -match 'FPS_INVENTORY FAIL|Fatal error:|Assertion failed:'){throw "Inventory $inventoryStage failed: $inventoryLog"}
                $inventoryRecord.runs+=@{width=$inventoryWidth;height=$inventoryHeight;stage=$inventoryStage;log=$inventoryLog;profile=$inventoryProfile;exit_code=$inventoryProcess.ExitCode}
                Write-Output "FPS_INVENTORY PASS $inventoryStage $inventoryWidth"
            } finally {if(!$inventoryProcess.HasExited){Stop-Process -Id $inventoryProcess.Id}}
        }
        foreach($inventoryImage in @('SplitDialog','WarehouseQuantities','RaidSplitDialog','RaidBackpackQuantities')) {
            if(!(Test-Path -LiteralPath (Join-Path $inventoryEvidence "$inventoryImage.png"))){throw "Missing screenshot $inventoryImage"}
        }
    }
    if((Get-FileHash -LiteralPath $inventoryModule).Hash -ne $inventoryHash){throw 'Frozen runtime module changed'}
    foreach($inventoryAsset in $inventoryAssets){if((Get-FileHash -LiteralPath $inventoryAsset.path).Hash -ne $inventoryAsset.sha256){throw "Asset changed during validation: $($inventoryAsset.path)"}}
    $inventoryRecord.status='passed'
    Write-Output "FPS_INVENTORY COMPLETE $inventoryRun"
} catch {$inventoryRecord.status='failed';$inventoryRecord.error=$_.Exception.Message;throw}
finally {$inventoryRecord.finished=(Get-Date).ToString('o');$inventoryRecord | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $inventoryRun 'run.json') -Encoding UTF8}
