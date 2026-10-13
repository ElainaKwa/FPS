#include "FPSSettingsSmoke.h"
#include "FPSDemoPlayerController.h"
#include "FPSSystemSettings.h"
#include "FPSSettingsUI.h"
#include "FPSInterface.h"
#include "FPSDemoCharacter.h"
#include "FPSMatchFlow.h"
#include "FPSDemoGameState.h"
#include "FPSWeaponComponent.h"
#include "FPSMovementComponent.h"
#include "FPSAudioSubsystem.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "InputKeyEventArgs.h"
#include "Misc/App.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"
#include "Components/ScrollBox.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"

void FFPSSettingsSmoke::Tick(AFPSDemoPlayerController* PC)
{
#if !UE_BUILD_SHIPPING
    NetworkTick(PC);
    if(!PC||!PC->IsLocalController()||PC->GetWorld()->GetTimeSeconds()<4)return;
    const bool Reload=FParse::Param(FCommandLine::Get(),TEXT("FPSSettingsReload"));
    if(!Reload&&!FParse::Param(FCommandLine::Get(),TEXT("FPSSettingsSmoke")))return;
    auto* S=UFPSSystemSettings::Get(PC);if(!S)return;
    static FName PendingPage;
    if(!PendingPage.IsNone()&&PC->SettingsUI){PC->SettingsUI->Dispatch(PendingPage);PendingPage=NAME_None;}
    bool Captured=false;
    static int32 Stage=0;static double At=0;static bool Failed=false;static double DisplayStarted=0;
    if(Stage>=99||FPlatformTime::Seconds()-At<.8)return;At=FPlatformTime::Seconds();
    auto Check=[&](bool Pass,const TCHAR* Text){UE_LOG(LogTemp,Display,TEXT("FPS_SETTINGS %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),Text);Failed|=!Pass;return Pass;};
    auto End=[&](){if(!Failed)UE_LOG(LogTemp,Display,TEXT("FPS_SETTINGS COMPLETE"));Stage=99;FPlatformMisc::RequestExit(false);};
    if(Reload)
    {
        Check(FMath::IsNearlyEqual(S->Sensitivity,.65f)&&S->InvertY&&FMath::IsNearlyEqual(S->BaseFOV(),95.f)&&S->Binding("Inspect")->Primary==EKeys::ThumbMouseButton,TEXT("fresh process restores personal settings and side mouse binding"));
        Check(S->Graphics.FrameLimit==120&&S->Graphics.Quality[5]==S->Defaults.Quality[5],TEXT("invalid individual stored values recover without resetting valid values"));End();return;
    }
    auto* UI=PC->SettingsUI.Get();
    auto Capture=[&](const TCHAR* Name){Captured=true;FString Dir;FParse::Value(FCommandLine::Get(),TEXT("SettingsEvidence="),Dir);if(!Dir.IsEmpty()){int32 W,H;PC->GetViewportSize(W,H);FScreenshotRequest::RequestScreenshot(Dir/FString::Printf(TEXT("%s-%dx%d.png"),Name,W,H),true,false);}};
    auto Page=[&](FName Name){if(Captured)PendingPage=Name;else if(UI)UI->Dispatch(Name);};
    if(Stage==0){PC->OpenSettings();Check(PC->SettingsUI&&PC->IsMoveInputIgnored(),TEXT("shared settings opens and blocks game input"));}
    if(Stage>0&&Stage<=9&&!Check(UI!=nullptr,TEXT("settings surface remains available"))){End();return;}
    if(Stage==1)
    {
        Capture(TEXT("Graphics"));Check(S->Options(0).Num()==15,TEXT("supported graphics controls present"));
        auto* Scroll=Cast<UScrollBox>(UI->WidgetTree->FindWidget(TEXT("SettingsRows")));Check(Scroll&&Scroll->GetChildrenCount()==15,TEXT("graphics rows use saved scroll area"));
        S->bForceSaveFailure=true;S->SetValue("GameVolume",.6);Check(!S->LastSaveSucceeded()&&FMath::IsNearlyEqual(S->GameVolume,.6f),TEXT("failed save retains session value and reports failure"));S->bForceSaveFailure=false;S->Save();
        Page("PageSound");
    }
    if(Stage==2)
    {
        if(auto* Row=UI->FindOptionRow("MasterVolume"))if(auto* Slider=Cast<USlider>(Row->WidgetTree->FindWidget(TEXT("OptionSlider"))))Slider->OnValueChanged.Broadcast(.75f);
        Check(FMath::IsNearlyEqual(S->MasterVolume,.75f)&&S->LastSaveSucceeded(),TEXT("native slider persists volume"));
        S->SetValue("BackgroundMute",0);S->PreviewSound();auto* Audio=UFPSAudioSubsystem::Get(PC);
        Check(Audio&&Audio->ActiveVoices.Num()>0,TEXT("explicit preview creates a real audio voice"));
        S->SetValue("GameVolume",0);bool Muted=true;if(Audio)for(auto& Pair:Audio->ActiveVoices)if(auto* Voice=Pair.Key.Get())Muted&=Voice->VolumeMultiplier==0;
        Check(Muted,TEXT("volume change updates currently playing voices"));S->SetValue("GameVolume",.6);S->SetValue("BackgroundMute",1);
        Check(FApp::GetUnfocusedVolumeMultiplier()==0,TEXT("background mute applies engine focus multiplier"));Capture(TEXT("Sound"));Page("PageControls");
    }
    if(Stage==3){UI->SetOptionValue("Sensitivity",.65);UI->SetOptionValue("InvertY",1);UI->SetOptionValue("AimToggle",1);Capture(TEXT("Controls"));Page("PageBindings");}
    if(Stage==4)
    {
        UI->ActivateOption("Bind.Inspect.0",EFPSSettingKind::Binding);
        const FKeyEvent Esc(EKeys::Escape,FModifierKeysState(),0,false,0,0);UI->NativeOnPreviewKeyDown(UI->GetCachedGeometry(),Esc);
        Check(!S->IsCapturing()&&PC->IsSettingsOpen(),TEXT("Escape cancels capture without closing settings"));
        UI->ActivateOption("Bind.Inspect.0",EFPSSettingKind::Binding);
        const FKeyEvent Conflict(EKeys::R,FModifierKeysState(),0,false,0,0);UI->NativeOnPreviewKeyDown(UI->GetCachedGeometry(),Conflict);
        Check(S->HasBindingConflict()&&S->Binding("Reload")->Primary==EKeys::R,TEXT("conflict waits for explicit replace"));UI->Dispatch("ConflictCancel");
        UI->ActivateOption("Bind.Inspect.0",EFPSSettingKind::Binding);UI->NativeOnPreviewKeyDown(UI->GetCachedGeometry(),Conflict);UI->Dispatch("ConflictReplace");
        Check(S->Binding("Inspect")->Primary==EKeys::R&&!S->Binding("Reload")->Primary.IsValid(),TEXT("replace clears only old conflicting binding"));
        UI->ActivateOption("Bind.Inspect.0",EFPSSettingKind::Binding);
        const FPointerEvent Mouse(0,FVector2D(-100,-100),FVector2D(-100,-100),{EKeys::ThumbMouseButton},EKeys::ThumbMouseButton,0,FModifierKeysState());UI->NativeOnPreviewMouseButtonDown(UI->GetCachedGeometry(),Mouse);
        Check(S->Binding("Inspect")->Primary==EKeys::ThumbMouseButton,TEXT("side mouse capture reaches binding backend"));
        S->BeginCapture("Bind.Reload.0");S->CaptureKey(EKeys::R);UI->Refresh();Capture(TEXT("Bindings"));Page("PageHUD");
    }
    if(Stage==5){UI->SetOptionValue("HUDScale",1.2);UI->SetOptionValue("BobScale",0);Capture(TEXT("HUD"));}
    if(Stage==6){UI->Dispatch("ResetPage");Check(FMath::IsNearlyEqual(S->HUDScale,1.2f),TEXT("reset waits for second confirmation"));UI->Dispatch("ResetConfirm");Check(S->HUDScale==1,TEXT("page default affects selected category"));Page("PageGraphics");UI->SetOptionValue("FOV",95);Page("PageSound");UI->SetOptionValue("GameVolume",.55);UI->RequestBack();Capture(TEXT("Unsaved"));}
    if(Stage==7){UI->Dispatch("ExitContinue");Check(S->HasPendingGraphics()&&PC->IsSettingsOpen(),TEXT("draft survives navigation and continue editing"));S->DiscardGraphics();Check(S->BaseFOV()==92&&FMath::IsNearlyEqual(S->GameVolume,.55f),TEXT("discard does not roll back immediate category"));Page("PageGraphics");UI->SetOptionValue("FOV",95);UI->SetOptionValue("Preset",1);UI->SetOptionValue("Textures",2);UI->Dispatch("ApplyGraphics");Check(S->BaseFOV()==95&&GEngine->GetGameUserSettings()->GetTextureQuality()==2,TEXT("graphics apply changes runtime quality and FOV"));}
    const bool Display=FParse::Param(FCommandLine::Get(),TEXT("FPSSettingsDisplayCheck"));
    if(Stage==7&&Display)
    {
        S->SetValue("DisplayMode",0);S->ApplyGraphics();DisplayStarted=FPlatformTime::Seconds();UI->Refresh();Capture(TEXT("DisplayConfirm"));Check(S->IsConfirmingDisplay()&&GEngine->GetGameUserSettings()->GetFullscreenMode()==EWindowMode::Windowed,TEXT("display change starts 15 second confirmation"));
    }
    if(Stage==8&&Display&&S->IsConfirmingDisplay()){return;}
    if(Stage==8&&Display){Check(FPlatformTime::Seconds()-DisplayStarted>=15&&S->Graphics.Mode==1,TEXT("natural timeout restores previous display"));S->SetValue("DisplayMode",0);S->ApplyGraphics();UI->Refresh();UI->Dispatch("DisplayConfirm");Check(!S->IsConfirmingDisplay()&&S->Graphics.Mode==0,TEXT("display explicit confirmation persists"));}
    if(Stage==9)
    {UI->RequestBack();if(auto* Front=Cast<AFPSFrontController>(PC))Front->HideMenu();S->SetValue("FOV",85);S->ApplyGraphics();S->SetValue("HUDScale",.75);}
    auto* Body=Cast<AFPSDemoCharacter>(PC->GetPawn());auto* Camera=Body?Body->FindComponentByClass<UCameraComponent>():nullptr;
    if(Stage==10){Check(Camera&&FMath::IsNearlyEqual(Camera->FieldOfView,85.f,.1f),TEXT("minimum base FOV reaches actual camera"));Capture(TEXT("Gameplay-FOV85-HUD75"));}
    if(Stage==11){S->SetValue("FOV",100);S->ApplyGraphics();S->SetValue("HUDScale",1.2);S->SetValue("BobScale",0);}
    if(Stage==12){Check(Camera&&FMath::IsNearlyEqual(Camera->FieldOfView,100.f,.1f),TEXT("maximum base FOV reaches actual camera"));Capture(TEXT("Gameplay-FOV100-HUD120"));}
    if(Stage==13&&Body)Body->GetWeapons()->AimOn();
    if(Stage==14){Check(Camera&&Body&&FMath::IsNearlyEqual(Camera->FieldOfView,Body->GetWeapons()->EffectiveStats().AimFOV,.1f),TEXT("ADS retains weapon FOV regardless of base FOV"));Capture(TEXT("Gameplay-ADS-HUD120"));}
    if(Stage==15)
    {
        S->SetValue("FOV",95);S->ApplyGraphics();S->SetValue("HUDScale",1);
        S->Save();FString Text;FFileHelper::LoadFileToString(Text,*S->StoragePath());Text.ReplaceInline(TEXT("\"FrameLimit\": 120"),TEXT("\"FrameLimit\": -3"));Text.ReplaceInline(TEXT("\"Textures\": 2"),TEXT("\"Textures\": 88"));FFileHelper::SaveStringToFile(Text,*S->StoragePath());End();return;
    }
    ++Stage;
#endif
}
void AFPSDemoPlayerController::ServerSettingsTest_Implementation(int32 Stage){FFPSSettingsSmoke::ServerProbe(this,Stage);}
void FFPSSettingsSmoke::ServerProbe(AFPSDemoPlayerController* PC,int32 Stage)
{
#if !UE_BUILD_SHIPPING
    if(!FParse::Param(FCommandLine::Get(),TEXT("FPSSettingsNetwork")))return;
    auto* C=Cast<AFPSDemoCharacter>(PC->GetPawn());if(!C)return;auto* W=C->GetWeapons();
    auto Check=[&](bool Pass,const TCHAR* Text){UE_LOG(LogTemp,Display,TEXT("FPS_SETTINGS_NET %s authority=%s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*PC->GetName(),Text);};
    if(Stage==0)
    {
        C->SetActorLocation(FVector(0,PC->IsLocalController()?-1500:1500,180));PC->SetControlRotation(FRotator(0,0,0));
        W->StopAllActions();W->bMenuInputBlocked=false;
        W->Loadout.Primary=W->Catalog()->CreateInstance(TEXT("Weapon.BR3"));W->Loadout.ActiveSlot=EFPSWeaponSlot::Primary;W->Loadout.Action=EFPSWeaponAction::Idle;W->Changed();
    }
    if(Stage==1)Check(W->Equipped().ShotSerial==3&&W->Equipped().Magazine==27&&W->bMenuInputBlocked,TEXT("menu preserves exactly one started burst"));
    if(Stage==2)Check(W->GetLoadout().Action==EFPSWeaponAction::Idle&&W->Equipped().Magazine==30&&W->bMenuInputBlocked,TEXT("reload completes while menu blocks new commands"));
    if(Stage==3)Check(W->GetLoadout().Action==EFPSWeaponAction::Idle&&W->GetLoadout().ActiveSlot==EFPSWeaponSlot::Secondary,TEXT("switch completes while menu open"));
    if(Stage==4)Check(W->GetLoadout().Action==EFPSWeaponAction::Idle,TEXT("inspect completes while menu open"));
    if(Stage==5)
    {W->StopAllActions();W->Loadout.Primary=W->Catalog()->CreateInstance(TEXT("Weapon.M4"));W->Loadout.ActiveSlot=EFPSWeaponSlot::Primary;W->Loadout.Action=EFPSWeaponAction::Idle;W->Changed();}
    if(Stage==6)Check(!W->GetLoadout().bFireActive,TEXT("automatic fire stops on menu opening"));
    if(Stage==7){Check(!W->bMenuInputBlocked,TEXT("close releases authoritative input block"));UE_LOG(LogTemp,Display,TEXT("FPS_SETTINGS_NET AUTH_COMPLETE %s"),*PC->GetName());}
    if(Stage==8)
    {
        // An isolated flat collision fixture keeps native key checks independent of level walls.
        auto* Floor=PC->GetWorld()->SpawnActor<AActor>();
        auto* Box=NewObject<UBoxComponent>(Floor);Floor->SetRootComponent(Box);Box->SetBoxExtent(FVector(3000,500,20));Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();
        const float Y=PC->IsLocalController()?-1500.f:1500.f;Floor->SetActorLocation(FVector(10000,Y,1000));
        C->SetActorLocation(FVector(9000,Y,1120));C->GetCharacterMovement()->StopMovementImmediately();PC->SetControlRotation(FRotator::ZeroRotator);
    }
#endif
}
void FFPSSettingsSmoke::NetworkTick(AFPSDemoPlayerController* PC)
{
#if !UE_BUILD_SHIPPING
    if(!PC||!FParse::Param(FCommandLine::Get(),TEXT("FPSSettingsNetwork")))return;
    auto* Front=Cast<AFPSFrontController>(PC);auto* GS=PC->GetWorld()->GetGameState<AFPSDemoGameState>();
    if(!Front||!GS)return;
    if(PC->HasAuthority()&&PC->IsLocalController()&&GS->bWaiting&&GS->PlayerArray.Num()==2)Front->ServerStartRound();
    if(!PC->IsLocalController()||GS->bWaiting||!Front->IsWeaponContentVerified())return;
    auto* C=Cast<AFPSDemoCharacter>(PC->GetPawn());if(!C||!C->GetWeapons()->IsEnabled()||C->IsEliminated())return;auto* W=C->GetWeapons();
    struct FState{int32 Stage=0;double At=0;bool Failed=false;uint32 Shots=0;};static TMap<TWeakObjectPtr<AFPSDemoPlayerController>,FState> States;auto& P=States.FindOrAdd(PC);
    if(P.Stage>=99)return;const double Now=FPlatformTime::Seconds();if(Now-P.At<(P.Stage==6?3.0:P.Stage==10?4.0:1.0))return;
    auto Check=[&](bool Pass,const TCHAR* Text){P.Failed|=!Pass;UE_LOG(LogTemp,Display,TEXT("FPS_SETTINGS_NET %s role=%s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),PC->HasAuthority()?TEXT("HOST"):TEXT("CLIENT"),Text);};
    auto Open=[&](){Front->ShowMenu();};auto Close=[&](){Front->HideMenu();};
    if(P.Stage==0){PC->ServerSettingsTest(0);}
    if(P.Stage==1){if(W->Equipped().WeaponId!=TEXT("Weapon.BR3"))return;Close();}
    if(P.Stage==2){W->StartFire();Open();}
    if(P.Stage==3){Check(W->Equipped().ShotSerial==3&&W->Equipped().Magazine==27,TEXT("owner burst completes behind menu"));PC->ServerSettingsTest(1);Close();}
    if(P.Stage==4){W->Reload();Check(W->GetLoadout().Action==EFPSWeaponAction::Reloading,TEXT("reload began"));Open();}
    if(P.Stage==5){PC->OpenSettings(Front->MenuAsset);Check(PC->IsSettingsOpen()&&W->GetLoadout().Action==EFPSWeaponAction::Reloading,TEXT("nested settings preserve reload"));}
    if(P.Stage==6){Check(W->Equipped().Magazine==30&&W->GetLoadout().Action==EFPSWeaponAction::Idle,TEXT("reload completes in nested settings"));PC->ServerSettingsTest(2);PC->SettingsBack();Close();}
    if(P.Stage==7){W->Secondary();Open();}
    if(P.Stage==8){Check(W->GetLoadout().ActiveSlot==EFPSWeaponSlot::Secondary&&W->GetLoadout().Action==EFPSWeaponAction::Idle,TEXT("owner switch completes"));PC->ServerSettingsTest(3);Close();}
    if(P.Stage==9){W->Inspect();Check(W->GetLoadout().Action==EFPSWeaponAction::Inspecting,TEXT("inspect began"));Open();}
    if(P.Stage==10){Check(W->GetLoadout().Action==EFPSWeaponAction::Idle,TEXT("inspect completes behind menu"));PC->ServerSettingsTest(4);Close();PC->ServerSettingsTest(5);}
    if(P.Stage==11){if(W->Equipped().WeaponId!=TEXT("Weapon.M4"))return;W->StartFire();}
    if(P.Stage==12){Open();P.Shots=W->Equipped().ShotSerial;}
    if(P.Stage==13){Check(!W->GetLoadout().bFireActive,TEXT("automatic input ended"));P.Shots=W->Equipped().ShotSerial;PC->ServerSettingsTest(6);}
    if(P.Stage==14){Check(W->Equipped().ShotSerial==P.Shots,TEXT("menu does not continue automatic fire"));Close();}
    if(P.Stage==15){Check(!W->GetLoadout().bFireActive,TEXT("close does not resume old trigger"));PC->ServerSettingsTest(7);}
    auto* S=UFPSSystemSettings::Get(PC);
    auto Key=[&](FKey K,bool Down){PC->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),K,Down?IE_Pressed:IE_Released,Down?1.f:0.f,false,FPlatformTime::Cycles64()));};
    if(P.Stage==16)
    {
        PC->ServerSettingsTest(8);
        if(!PC->HasAuthority())
        {auto* Floor=PC->GetWorld()->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Floor);Floor->SetRootComponent(Box);Box->SetBoxExtent(FVector(3000,500,20));Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();Floor->SetActorLocation(FVector(10000,1500,1000));}
        S->BeginCapture("Bind.Forward.0");S->CaptureKey(EKeys::I);
    }
    if(P.Stage==17)Key(EKeys::W,true);
    if(P.Stage==18){Check(C->GetVelocity().Size2D()<1,TEXT("old movement binding no longer moves"));Key(EKeys::W,false);Key(EKeys::I,true);}
    if(P.Stage==19){Check(C->GetVelocity().Size2D()>100,TEXT("new movement binding drives actual movement"));Open();}
    if(P.Stage==20){Check(C->GetVelocity().Size2D()<1,TEXT("menu clears movement intent and naturally brakes"));Close();}
    if(P.Stage==21)Check(C->GetVelocity().Size2D()<1,TEXT("held movement key cannot resume after menu close"));
    if(P.Stage==22)Key(EKeys::I,false);
    if(P.Stage==23)Key(EKeys::I,true);
    if(P.Stage==24){UE_LOG(LogTemp,Display,TEXT("FPS_SETTINGS_NET INPUT position=%s velocity=%s acceleration=%s held=%d blocked=%d weaponBlock=%d"),*C->GetActorLocation().ToString(),*C->GetVelocity().ToString(),*C->GetCharacterMovement()->GetCurrentAcceleration().ToString(),PC->IsInputKeyDown(EKeys::I),PC->IsMoveInputIgnored(),W->bMenuInputBlocked);Check(C->GetCharacterMovement()->GetCurrentAcceleration().Size2D()>100,TEXT("released and repressed movement key generates native acceleration"));Key(EKeys::I,false);S->ResetPage(3);S->SetValue("AimToggle",1);}
    if(P.Stage==25)Key(EKeys::RightMouseButton,true);
    if(P.Stage==26){Check(W->GetLoadout().bAiming,TEXT("new aim press enters ADS"));Key(EKeys::RightMouseButton,false);}
    if(P.Stage==27){Check(W->GetLoadout().bAiming,TEXT("toggle ADS survives release"));Open();}
    if(P.Stage==28){Check(!W->GetLoadout().bAiming,TEXT("menu clears toggle ADS"));Close();S->SetValue("AimToggle",0);}
    if(P.Stage==29)Key(EKeys::RightMouseButton,true);
    if(P.Stage==30){Check(W->GetLoadout().bAiming,TEXT("hold ADS begins"));Key(EKeys::RightMouseButton,false);}
    if(P.Stage==31)Check(!W->GetLoadout().bAiming,TEXT("hold ADS ends on release"));
    if(P.Stage==32){if(!P.Failed)UE_LOG(LogTemp,Display,TEXT("FPS_SETTINGS_NET COMPLETE role=%s"),PC->HasAuthority()?TEXT("HOST"):TEXT("CLIENT"));P.Stage=99;return;}
    ++P.Stage;P.At=Now;
#endif
}
