#include "FPSExtractionLobby.h"
#include "FPSFactoryBase.h"
#include "FPSExtractionUI.h"
#include "FPSExtractionSubsystem.h"
#include "FPSExtractionWeaponViewport.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/WidgetPath.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"

bool UFPSLobbyVerification::IsTickable() const
{
#if !UE_BUILD_SHIPPING
    return !IsTemplate()&&!bDone&&FParse::Param(FCommandLine::Get(),TEXT("FPSLobbySmoke"));
#else
    return false;
#endif
}
void UFPSLobbyVerification::Tick(float)
{
#if !UE_BUILD_SHIPPING
    auto* World=GetWorld();if(!World||World->GetTimeSeconds()<3||FPlatformTime::Seconds()-LastStep<1.1)return;
    auto* PC=Cast<AFPSFactoryController>(World->GetFirstPlayerController());
    auto* S=GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>();if(!S)return;
    auto* UI=PC?PC->GetTerminal():nullptr;
    auto Check=[&](bool OK,const TCHAR* What){UE_LOG(LogTemp,Display,TEXT("FPS_LOBBY %s stage=%d %s"),OK?TEXT("PASS"):TEXT("FAIL"),Stage,What);bFailed|=!OK;return OK;};
    auto Shown=[&](const TCHAR* Name){auto* W=UI?UI->GetWidgetFromName(Name):nullptr;return W&&W->IsVisible();};
    auto Click=[&](const TCHAR* Name)
    {
        auto* Button=UI?Cast<UFPSExtractionButton>(UI->GetWidgetFromName(Name)):nullptr;bool Routed=false;
        if(Button&&Button->GetCachedWidget().IsValid())
        {
            auto& App=FSlateApplication::Get();FWidgetPath OwnerPath;
            if(App.GeneratePathToWidgetUnchecked(Button->GetCachedWidget().ToSharedRef(),OwnerPath))
            {
                const auto Pos=Button->GetCachedGeometry().GetAbsolutePosition()+Button->GetCachedGeometry().GetAbsoluteSize()*.5;
                const auto Path=App.LocateWindowUnderMouse(Pos,{OwnerPath.GetWindow()},false);
                if(Path.ContainsWidget(Button->GetCachedWidget().Get()))
                {
                    const FPointerEvent Down(0,Pos,Pos,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());
                    const FPointerEvent Up(0,Pos,Pos,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState());
                    App.RoutePointerMoveEvent(Path,Up,false);App.RoutePointerDownEvent(Path,Down);App.RoutePointerUpEvent(Path,Up);Routed=true;
                }
            }
        }
        Check(Routed,Name);
    };
    auto Capture=[&](const TCHAR* Name){FString Out;FParse::Value(FCommandLine::Get(),TEXT("LobbyEvidence="),Out);if(!Out.IsEmpty())FScreenshotRequest::RequestScreenshot(Out/FString(Name)+TEXT(".png"),true,false);};
    if(Stage==0)
    {
        Check(UI&&Shown(TEXT("OverviewPanel"))&&!Shown(TEXT("BasePanel")),TEXT("lobby replaces old facility overview"));
        Check(PC&&PC->GetViewTarget()!=PC->GetPawn()&&PC->GetPawn()->IsHidden()&&PC->IsMoveInputIgnored(),TEXT("live factory presentation and movement lock"));
        auto* Credits=UI?Cast<UTextBlock>(UI->GetWidgetFromName(TEXT("LobbyCredits"))):nullptr;
        Check(Credits&&Credits->GetText().ToString()==FText::AsNumber(S->GetView().Credits).ToString(),TEXT("actual profile funds"));
    }
    if(Stage==1){Capture(TEXT("Lobby"));}
    if(Stage==2){Click(TEXT("LobbyWarehouse"));}
    if(Stage==3){Check(Shown(TEXT("InventoryPanel"))&&!Shown(TEXT("OverviewPanel")),TEXT("warehouse entry"));Capture(TEXT("Warehouse"));}
    if(Stage==4){Click(TEXT("Overview"));}
    if(Stage==5){Click(TEXT("LobbyModification"));}
    if(Stage==6){Check(Shown(TEXT("ModificationPanel"))&&Shown(TEXT("WeaponOverviewPanel")),TEXT("existing v4 workbench"));Capture(TEXT("Workbench"));}
    if(Stage==7){Click(TEXT("WorkbenchBack"));}
    if(Stage==8){Click(TEXT("LobbySettings"));}
    if(Stage==9){Check(PC&&PC->IsSettingsOpen(),TEXT("settings opens"));Capture(TEXT("Settings"));}
    if(Stage==10){if(PC)PC->SettingsBack();}
    if(Stage==11){Check(PC&&!PC->IsSettingsOpen()&&PC->IsMoveInputIgnored(),TEXT("settings returns to locked lobby"));Click(TEXT("LobbyProduction"));}
    if(Stage==12){Check(Shown(TEXT("WorkbenchPanel")),TEXT("production entry"));Capture(TEXT("Production"));}
    if(Stage==13){Click(TEXT("Overview"));}
    if(Stage==14){Click(TEXT("LobbyFactory"));}
    if(Stage==15)
    {Check(PC&&!PC->IsTerminalOpen()&&!PC->IsMoveInputIgnored()&&!PC->GetPawn()->IsHidden()&&PC->GetViewTarget()==PC->GetPawn(),TEXT("factory entry restores playable robot and camera"));Capture(TEXT("Factory"));}
    if(Stage==16){Check(PC&&PC->OpenLobby(),TEXT("return from factory to lobby"));}
    if(Stage==17){Click(TEXT("LobbyPrepare"));}
    if(Stage==18){Check(Shown(TEXT("InventoryPanel")),TEXT("prepare opens existing backpack and equipment"));Click(TEXT("Overview"));}
    if(Stage==19){Click(TEXT("LobbyDepart"));}
    if(Stage==20)
    {
        auto* Mode=World->GetAuthGameMode<AFPSExtractionMode>();
        Check(S->GetPhase()==EFPSExtractionPhase::Raid&&Mode&&Mode->IsRaid()&&World->GetMapName().Contains(TEXT("LV_FPSIndustrialCrossing")),TEXT("depart loads actual supported raid map"));
        Capture(TEXT("Raid"));
    }
    if(Stage==21){Check(S->FinishExpedition(true),TEXT("test settlement through existing authority"));}
    if(Stage==22)
    {
        TArray<UUserWidget*> Widgets;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this,Widgets,UFPSExtractionUI::StaticClass());
        if(Widgets.Num()==1)UI=Cast<UFPSExtractionUI>(Widgets[0]);
        Check(S->GetPhase()==EFPSExtractionPhase::Result,TEXT("result state"));Click(TEXT("Return"));
    }
    if(Stage==23)
    {Check(PC&&UI&&Shown(TEXT("OverviewPanel"))&&S->GetPhase()==EFPSExtractionPhase::Base,TEXT("settlement returns to factory lobby"));Capture(TEXT("Returned"));}
    if(Stage==24)
    {UE_LOG(LogTemp,Display,TEXT("FPS_LOBBY %s COMPLETE"),bFailed?TEXT("FAIL"):TEXT("PASS"));bDone=true;FPlatformMisc::RequestExit(false);}
    if(bFailed&&Stage<19){UE_LOG(LogTemp,Error,TEXT("FPS_LOBBY FAIL stopping before travel"));bDone=true;FPlatformMisc::RequestExit(false);}
    ++Stage;LastStep=FPlatformTime::Seconds();
#endif
}
