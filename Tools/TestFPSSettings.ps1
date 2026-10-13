param([string]$EngineRoot='D:\UE\UE_5.8',[switch]$Network,[ValidateSet(1280,1600,1920,2560)][int[]]$Widths=@(1280,1920,2560,1600),[switch]$Display)
$ErrorActionPreference='Stop'
$settingsRoot=Split-Path $PSScriptRoot -Parent
$settingsId=Get-Date -Format 'yyyyMMdd-HHmmssfff'
$settingsRun=Join-Path $settingsRoot "Saved/SettingsTests/$settingsId"
New-Item -ItemType Directory -Force $settingsRun | Out-Null
$settingsExe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$settingsModule=Join-Path $settingsRoot 'Binaries/Win64/UnrealEditor-MCP_Test.dll'
$settingsHash=(Get-FileHash -LiteralPath $settingsModule).Hash
$settingsRecord=[ordered]@{started=(Get-Date).ToString('o');status='running';network=[bool]$Network;module_sha256=$settingsHash;runs=@()}
$settingsOwned=@()
function Start-SettingsGame([string]$URL,[string]$Name,[string[]]$Extra){
    $settingsLog=Join-Path $settingsRun "$Name.log"
    $settingsArgs=@("`"$settingsRoot/MCP_Test.uproject`"",$URL,'-game','-unattended','-nosplash','-DDC=InstalledNoZenLocalFallback','-NoZenAutoLaunch',"-abslog=`"$settingsLog`"","-SettingsEvidence=`"$settingsRun`"",
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')+$Extra
    Start-Process -FilePath $settingsExe -WindowStyle Hidden -PassThru -ArgumentList $settingsArgs
}
function Wait-SettingsGame($Process,[string]$Name,[int]$Timeout=150){
    $settingsDeadline=(Get-Date).AddSeconds($Timeout)
    while(!$Process.HasExited -and (Get-Date) -lt $settingsDeadline){Start-Sleep -Seconds 2;$Process.Refresh()}
    if(!$Process.HasExited){throw "Timeout: $Name"}
    $Process.WaitForExit();$settingsText=Get-Content -LiteralPath (Join-Path $settingsRun "$Name.log") -Raw
    if($Process.ExitCode -ne 0 -or !$settingsText.Contains('FPS_SETTINGS COMPLETE') -or $settingsText -match 'FPS_SETTINGS FAIL|Fatal error:|Assertion failed:'){throw "Settings test failed: $Name"}
}
try{
    if($Network){
        $settingsURL='/Game/FPSDemo/Maps/Gameplay/IndustrialCrossing/LV_FPSIndustrialCrossing?game=/Script/MCP_Test.FPSMatchMode?Mode=LAN?Bots=0?listen'
        $settingsOwned+=Start-SettingsGame $settingsURL 'server' @('-port=17941','-nullrhi','-nosound','-FPSSettingsNetwork',"-FPSSettingsProfile=NetHost_$settingsId")
        Start-Sleep -Seconds 7
        $settingsOwned+=Start-SettingsGame '127.0.0.1:17941' 'client' @('-nullrhi','-nosound','-FPSSettingsNetwork',"-FPSSettingsProfile=NetClient_$settingsId")
        $settingsDeadline=(Get-Date).AddSeconds(150);$settingsDone=$false
        while((Get-Date) -lt $settingsDeadline){
            Start-Sleep -Seconds 2
            $settingsServer=Get-Content -LiteralPath (Join-Path $settingsRun 'server.log') -Raw -ErrorAction SilentlyContinue
            $settingsClient=Get-Content -LiteralPath (Join-Path $settingsRun 'client.log') -Raw -ErrorAction SilentlyContinue
            if(($settingsServer+$settingsClient) -match 'FPS_SETTINGS_NET FAIL|Fatal error:|Assertion failed:'){throw 'Network settings assertions failed'}
            $settingsDone=$settingsServer -match 'FPS_SETTINGS_NET COMPLETE role=HOST' -and $settingsClient -match 'FPS_SETTINGS_NET COMPLETE role=CLIENT' -and ([regex]::Matches($settingsServer,'FPS_SETTINGS_NET AUTH_COMPLETE')).Count -eq 2
            if($settingsDone){break}
        }
        if(!$settingsDone){throw 'Network settings test timed out'}
        $settingsRecord.runs+=@{kind='network';status='passed'}
    }else{
        $settingsURL='/Game/FPSDemo/Maps/LV_FPSTraining?game=/Script/MCP_Test.FPSMatchMode?Mode=Training?Bots=0'
        foreach($settingsWidth in $Widths){
            $settingsHeight=switch($settingsWidth){1280{800}1600{1000}1920{1080}2560{1440}}
            $settingsProfile="UI_${settingsId}_$settingsWidth";$settingsName="ui-$settingsWidth"
            $settingsFlags=@('-FPSSettingsSmoke',"-FPSSettingsProfile=$settingsProfile",'-RenderOffscreen','-windowed',"-ResX=$settingsWidth","-ResY=$settingsHeight",'-ForceRes')
            if($Display -and $settingsWidth -eq $Widths[0]){$settingsFlags+='-FPSSettingsDisplayCheck'}
            $settingsProcess=Start-SettingsGame $settingsURL $settingsName $settingsFlags;$settingsOwned+=$settingsProcess
            Wait-SettingsGame $settingsProcess $settingsName
            foreach($settingsPage in @('Graphics','Sound','Controls','Bindings','HUD','Unsaved')){if(!(Test-Path -LiteralPath (Join-Path $settingsRun "$settingsPage-${settingsWidth}x$settingsHeight.png"))){throw "Missing capture: $settingsPage $settingsWidth"}}
            $settingsRecord.runs+=@{kind='UI';width=$settingsWidth;height=$settingsHeight;profile=$settingsProfile}
        }
        $settingsProcess=Start-SettingsGame $settingsURL 'reload' @('-FPSSettingsReload',"-FPSSettingsProfile=$settingsProfile",'-nullrhi','-nosound');$settingsOwned+=$settingsProcess
        Wait-SettingsGame $settingsProcess 'reload';$settingsRecord.runs+=@{kind='fresh-process-reload';profile=$settingsProfile}
    }
    if((Get-FileHash -LiteralPath $settingsModule).Hash -ne $settingsHash){throw 'Module changed during test'}
    $settingsRecord.status='passed';Write-Output "FPS_SETTINGS TEST PASS $settingsRun"
}catch{$settingsRecord.status='failed';$settingsRecord.error=$_.Exception.Message;throw}
finally{
    foreach($settingsProcess in $settingsOwned){if(!$settingsProcess.HasExited){Stop-Process -Id $settingsProcess.Id -ErrorAction SilentlyContinue}}
    $settingsRecord.finished=(Get-Date).ToString('o');$settingsRecord | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $settingsRun 'run.json') -Encoding utf8
}
