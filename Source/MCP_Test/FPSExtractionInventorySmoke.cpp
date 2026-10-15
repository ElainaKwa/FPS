#include "FPSExtractionSmoke.h"
#include "FPSExtractionMode.h"
#include "FPSExtractionSubsystem.h"
#include "FPSExtractionUI.h"
#include "FPSExtractionGridUI.h"
#include "FPSFactoryBase.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SpinBox.h"
#include "Components/Button.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Widgets/SWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/WidgetPath.h"

void FFPSExtractionSmoke::InventoryTick(AFPSExtractionMode* Mode)
{
#if !UE_BUILD_SHIPPING
    const bool Reload=FParse::Param(FCommandLine::Get(),TEXT("FPSInventoryReload"));
    if(!Mode || (!Reload&&!FParse::Param(FCommandLine::Get(),TEXT("FPSInventorySmoke"))) || Mode->GetWorld()->GetTimeSeconds()<3)return;
    auto* S=Mode->GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>();
    if(!S || !S->Profile || !S->SlotPrefix.Contains(TEXT("Inventory_")))return;
    static int32 Stage=0;static double At=0;static FGuid SourceId,SplitId;static FString Before;
    const double Now=FPlatformTime::Seconds();if(Now-At<.65 || Stage>30)return;
    FString Directory;FParse::Value(FCommandLine::Get(),TEXT("InventoryEvidence="),Directory);if(Directory.IsEmpty())return;
    auto Check=[&](bool Good,const TCHAR* What)
    {UE_LOG(LogTemp,Display,TEXT("FPS_INVENTORY %s %s stage=%d"),Good?TEXT("PASS"):TEXT("FAIL"),What,Stage);if(!Good){Stage=99;FPlatformMisc::RequestExitWithStatus(false,1);}return Good;};
    auto State=[&]()
    {
        FString Value=FString::Printf(TEXT("credits=%d;"),S->Profile->Credits);const auto View=S->GetView();
        for(const auto* List:{&View.Warehouse,&View.Backpack})
        {Value+=TEXT("|");for(const auto& X:*List)Value+=FString::Printf(TEXT("%s:%s:%d:%d:%d:%d;"),*X.StackId.ToString(),*X.ItemId.ToString(),X.Quantity,X.X,X.Y,X.bRotated);}
        return Value;
    };
    auto Capture=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Directory/(FString(Name)+TEXT(".png")),true,false);};
    if(Reload)
    {
        FString Expected;
        if(Check(FFileHelper::LoadFileToString(Expected,*(Directory/TEXT("expected.txt"))) && Expected==State() &&
            S->Count(S->GetView().Warehouse,TEXT("Scrap"))==12 && S->Count(S->GetView().Backpack,TEXT("Scrap"))==26 &&
            S->ValidateProfile(S->Profile),TEXT("fresh process restores exact quantities IDs positions and rotations")))
        {UE_LOG(LogTemp,Display,TEXT("FPS_INVENTORY RELOAD COMPLETE"));Stage=99;FPlatformMisc::RequestExit(false);}return;
    }
    auto* FactoryPC=Cast<AFPSFactoryController>(Mode->GetWorld()->GetFirstPlayerController());
    auto* UI=FactoryPC?FactoryPC->GetTerminal():nullptr;
    if(!FactoryPC)
    {
        TArray<UUserWidget*> Widgets;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(Mode,Widgets,UFPSExtractionUI::StaticClass());
        if(Widgets.Num()==1)UI=Cast<UFPSExtractionUI>(Widgets[0]);
    }
    if(!UI)return;
    auto* Store=Cast<UFPSExtractionGridUI>(UI->WidgetTree->FindWidget(TEXT("WarehouseGrid")));
    auto* Bag=Cast<UFPSExtractionGridUI>(UI->WidgetTree->FindWidget(Mode->IsRaid()?TEXT("RaidBackpackGrid"):TEXT("BaseBackpackGrid")));
    auto Click=[&](UFPSExtractionGridUI* Grid,const FFPSExtractionStack& Stack,FKey Key,bool Ctrl=false)
    {
        const auto G=Grid->GetCachedGeometry();const auto P=G.LocalToAbsolute(FVector2D((Stack.X+.5f)*Grid->CellSize,(Stack.Y+.5f)*Grid->CellSize));
        const FModifierKeysState Mods(false,false,Ctrl,false,false,false,false,false,false);
        const FPointerEvent Event(0,P,P,{Key},Key,0,Mods);return Grid->NativeOnMouseButtonDown(G,Event).IsEventHandled();
    };
    if(Stage==0)
    {
        if(FParse::Param(FCommandLine::Get(),TEXT("FactoryLobby")))
        {
            auto* Overview=UI->GetWidgetFromName(TEXT("OverviewPanel"));
            if(!Check(FactoryPC&&Overview&&Overview->IsVisible(),TEXT("inventory starts in actual factory lobby")))return;
        }
        if(!Check(S->Profile->Warehouse.IsEmpty()&&S->Profile->PreparedBackpack.IsEmpty(),TEXT("unique empty test profile")))return;
        if(!Check(GridChecks(S),TEXT("existing grid migration rotation and transaction regression")))return;
        auto* Seed=S->CopyProfile();Seed->Warehouse.Reset();Seed->PreparedBackpack.Reset();
        if(!Check(S->TryAdd(Seed->Warehouse,TEXT("Scrap"),25,64)&&S->Commit(Seed),TEXT("seed isolated quantities")))return;
        SourceId=S->Profile->Warehouse[0].StackId;Before=State();const int32 Revision=S->Profile->Revision;
        if(!Check(!S->SplitStack(EFPSExtractionContainer::Warehouse,SourceId,0)&&!S->SplitStack(EFPSExtractionContainer::Warehouse,SourceId,-1)&&
            !S->SplitStack(EFPSExtractionContainer::Warehouse,SourceId,20)&&!S->SplitStack(EFPSExtractionContainer::Warehouse,SourceId,MAX_int32)&&
            !S->SplitStack(EFPSExtractionContainer(255),SourceId,1)&&!S->SplitStack(EFPSExtractionContainer::Warehouse,FGuid::NewGuid(),1)&&
            Before==State()&&Revision==S->Profile->Revision,TEXT("invalid quantity container and stale ID reject atomically")))return;
        if(!Check(S->SplitStack(EFPSExtractionContainer::Warehouse,SourceId,7),TEXT("split exact requested amount")))return;
        auto View=S->GetView();const auto Split=View.Warehouse.Last();
        if(!Check(View.Warehouse[0].StackId==SourceId&&View.Warehouse[0].Quantity==13&&Split.Quantity==7&&Split.StackId!=SourceId&&
            S->Count(View.Warehouse,TEXT("Scrap"))==25&&S->ValidateGrid(View.Warehouse,64),TEXT("split conserves total with unique ID and nonoverlap")))return;
        if(!Check(S->MoveStack(EFPSExtractionContainer::Warehouse,Split.StackId,EFPSExtractionContainer::Warehouse,0,0,false)&&
            S->MoveStack(EFPSExtractionContainer::Warehouse,SourceId,EFPSExtractionContainer::Warehouse,2,0,false)&&
            S->Profile->Warehouse[0].Quantity==5&&S->Profile->Warehouse[0].StackId==SourceId&&S->Profile->Warehouse[1].Quantity==20,
            TEXT("same-container merge supports aliasing and preserves remainder")))return;
        // A directory at the isolated save-file path induces a real save failure.
        const FString OriginalPrefix=S->SlotPrefix;
        S->SlotPrefix=OriginalPrefix+TEXT("_Blocked");
        const FString Blocked=FPaths::ProjectSavedDir()/TEXT("SaveGames")/(S->SlotPrefix+FString::Printf(TEXT("_%d.sav"),(S->Profile->Revision+1)%2));
        auto& Files=FPlatformFileManager::Get().GetPlatformFile();Before=State();
        const bool BlockCreated=Files.CreateDirectory(*Blocked);
        const bool SaveRejected=BlockCreated&&!S->SplitStack(EFPSExtractionContainer::Warehouse,SourceId,1)&&Before==State();
        S->SlotPrefix=OriginalPrefix;if(BlockCreated)Files.DeleteDirectory(*Blocked);
        if(!Check(SaveRejected,TEXT("failed disk write leaves original inventory intact")))return;
        Seed=S->CopyProfile();Seed->PreparedBackpack.Reset();
        if(!Check(S->TryAdd(Seed->PreparedBackpack,TEXT("Wire"),400,20)&&S->Commit(Seed),TEXT("full bag fixture")))return;
        Before=State();const auto Full=S->Profile->PreparedBackpack[0].StackId;
        if(!Check(!S->SplitStack(EFPSExtractionContainer::Backpack,Full,1)&&
            !S->AutoTransferStack(EFPSExtractionContainer::Warehouse,SourceId,EFPSExtractionContainer::Backpack)&&Before==State(),
            TEXT("full container split and transfer preserve both containers")))return;
        Seed=S->CopyProfile();Seed->Warehouse.Reset();Seed->PreparedBackpack.Last().Quantity=19;
        if(!Check(S->TryAdd(Seed->Warehouse,TEXT("Wire"),5,64)&&S->Commit(Seed),TEXT("partial capacity fixture")))return;
        Before=State();
        if(!Check(!S->AutoTransferStack(EFPSExtractionContainer::Warehouse,S->Profile->Warehouse[0].StackId,EFPSExtractionContainer::Backpack)&&Before==State(),
            TEXT("automatic transfer rolls back tentative merge when remainder has no space")))return;
        Seed=S->CopyProfile();Seed->Warehouse.Reset();Seed->PreparedBackpack.Reset();
        if(!Check(S->TryAdd(Seed->Warehouse,TEXT("Scrap"),20,64)&&S->TryAdd(Seed->PreparedBackpack,TEXT("Scrap"),18,20)&&S->Commit(Seed),TEXT("UI fixture committed")))return;
        SourceId=S->Profile->Warehouse[0].StackId;
        if(FactoryPC)
        {
            auto* Button=UI->GetWidgetFromName(TEXT("LobbyWarehouse"));
            auto& App=FSlateApplication::Get();FWidgetPath OwnerPath;
            if(!Check(Button&&Button->GetCachedWidget().IsValid()&&App.GeneratePathToWidgetUnchecked(Button->GetCachedWidget().ToSharedRef(),OwnerPath),TEXT("factory warehouse button is routed")))return;
            const auto Pos=Button->GetCachedGeometry().GetAbsolutePosition()+Button->GetCachedGeometry().GetAbsoluteSize()*.5;
            const auto Path=App.LocateWindowUnderMouse(Pos,{OwnerPath.GetWindow()},false);
            if(!Check(Path.ContainsWidget(Button->GetCachedWidget().Get()),TEXT("factory warehouse button is hit-testable")))return;
            const FPointerEvent Down(0,Pos,Pos,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());
            const FPointerEvent Up(0,Pos,Pos,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState());
            App.RoutePointerMoveEvent(Path,Up,false);App.RoutePointerDownEvent(Path,Down);App.RoutePointerUpEvent(Path,Up);
        }
        else UI->Dispatch(TEXT("Warehouse"));
    }
    if(Stage==1)
    {
        Before=State();
        if(!Check(Store&&Bag&&Store->GetCachedGeometry().GetLocalSize().X>0&&Click(Store,S->GetView().Warehouse[0],EKeys::RightMouseButton)&&Store->GetSplitDialog()&&Before==State(),TEXT("native right click opens quantity editor without mutation")))return;
    }
    if(Stage==2)
    {
        auto* Dialog=Store->GetSplitDialog();if(!Check(Dialog!=nullptr,TEXT("quantity editor survives refresh")))return;
        auto* Input=Cast<USpinBox>(Dialog->WidgetTree->FindWidget(TEXT("SplitQuantity")));
        if(!Check(Input&&Input->GetMinValue()==1&&Input->GetMaxValue()==19,TEXT("quantity limits reflect selected stack")))return;
        Input->SetValue(6);Capture(TEXT("SplitDialog"));
    }
    if(Stage==3)
    {
        auto* Dialog=Store->GetSplitDialog();const FKeyEvent Escape(EKeys::Escape,FModifierKeysState(),0,false,0,0);
        if(!Check(Dialog&&Dialog->TakeWidget()->OnPreviewKeyDown(Dialog->GetCachedGeometry(),Escape).IsEventHandled()&&!Store->GetSplitDialog()&&Before==State(),TEXT("Escape cancels without writes")))return;
        Click(Store,S->GetView().Warehouse[0],EKeys::RightMouseButton);Dialog=Store->GetSplitDialog();
        auto* Input=Dialog?Cast<USpinBox>(Dialog->WidgetTree->FindWidget(TEXT("SplitQuantity"))):nullptr;
        auto* Confirm=Dialog?Cast<UButton>(Dialog->WidgetTree->FindWidget(TEXT("SplitConfirm"))):nullptr;
        if(!Check(Input&&Confirm,TEXT("quantity confirm controls exist")))return;Input->SetValue(6);Confirm->OnClicked.Broadcast();
        auto View=S->GetView();
        if(!Check(View.Warehouse.Num()==2&&View.Warehouse[0].Quantity==14&&View.Warehouse[1].Quantity==6&&!Store->GetSplitDialog(),TEXT("confirm commits exact split and dismisses modal")))return;
        SplitId=View.Warehouse[1].StackId;
    }
    if(Stage==4)
    {
        auto View=S->GetView();const auto Source=View.Warehouse[0];
        const auto G=Store->GetCachedGeometry();const auto P=G.LocalToAbsolute(FVector2D((Source.X+.5f)*Store->CellSize,(Source.Y+.5f)*Store->CellSize));
        const FPointerEvent Press(0,P,P,{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());Store->NativeOnMouseButtonDown(G,Press);
        UDragDropOperation* Operation=nullptr;Store->NativeOnDragDetected(G,Press,Operation);
        auto* Drag=Cast<UFPSExtractionGridDrag>(Operation);if(!Check(Drag!=nullptr,TEXT("real drag created")))return;
        const auto Target=Bag->GetCachedGeometry();const auto Q=Target.LocalToAbsolute(FVector2D(.5f*Bag->CellSize,.5f*Bag->CellSize));
        const FPointerEvent Drop(0,Q,Q,{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());const FDragDropEvent Event(Drop,nullptr);
        if(!Check(Bag->NativeOnDragOver(Target,Event,Drag)&&Bag->NativeOnDrop(Target,Event,Drag),TEXT("partial merge uses native preview and drop")))return;
        View=S->GetView();
        if(!Check(View.Warehouse[0].Quantity==12&&View.Warehouse[0].StackId==SourceId&&View.Warehouse[0].X==Source.X&&View.Backpack[0].Quantity==20&&
            UI->IsInventorySelected(EFPSExtractionContainer::Warehouse,SourceId),TEXT("cross-container remainder stays selected in original cell")))return;
        Before=State();
        if(!Check(!S->MoveStack(EFPSExtractionContainer::Warehouse,SourceId,EFPSExtractionContainer::Backpack,0,0,false)&&Before==State(),TEXT("full target rejects without loss")))return;
    }
    if(Stage==5)
    {
        const auto Split=S->GetView().Warehouse[1];
        if(!Check(Click(Store,Split,EKeys::LeftMouseButton,true),TEXT("native Ctrl click quick transfer")))return;
        auto View=S->GetView();
        if(!Check(View.Warehouse.Num()==1&&S->Count(View.Backpack,TEXT("Scrap"))==26&&View.Backpack[1].StackId==SplitId&&
            UI->IsInventorySelected(EFPSExtractionContainer::Backpack,SplitId),TEXT("quick transfer retains physical ID and target selection")))return;
        Before=State();Click(Store,View.Warehouse[0],EKeys::RightMouseButton);
        auto* Dialog=Store->GetSplitDialog();auto* Cancel=Dialog?Cast<UButton>(Dialog->WidgetTree->FindWidget(TEXT("SplitCancel"))):nullptr;
        if(!Check(Cancel!=nullptr,TEXT("cancel control exists")))return;Cancel->OnClicked.Broadcast();
        if(!Check(Before==State()&&!Store->GetSplitDialog(),TEXT("cancel button leaves quantities unchanged")))return;
    }
    if(Stage==6)Capture(TEXT("WarehouseQuantities"));
    if(Stage==7){if(!Check(S->StartExpedition(),TEXT("enter actual raid with prepared bag")))return;}
    if(Stage==8)
    {
        if(!Mode->IsRaid()||!Mode->bRestored||S->GetPhase()!=EFPSExtractionPhase::Raid)return;
        UI->SetPanelOpen(true);Before=State();
        if(!Check(!S->SplitStack(EFPSExtractionContainer::Warehouse,SourceId,1)&&Before==State(),TEXT("raid cannot split remote warehouse")))return;
    }
    if(Stage==9)
    {
        const auto View=S->GetView();Before=State();
        if(!Check(Bag&&Click(Bag,View.Backpack[0],EKeys::LeftMouseButton,true)&&Before==State(),TEXT("raid Ctrl transfer cannot access warehouse")))return;
        if(!Check(Click(Bag,View.Backpack[0],EKeys::RightMouseButton)&&Bag->GetSplitDialog(),TEXT("actual raid backpack opens split editor")))return;
    }
    if(Stage==10)Capture(TEXT("RaidSplitDialog"));
    if(Stage==11)
    {
        auto* Dialog=Bag->GetSplitDialog();auto* Input=Dialog?Cast<USpinBox>(Dialog->WidgetTree->FindWidget(TEXT("SplitQuantity"))):nullptr;
        auto* Confirm=Dialog?Cast<UButton>(Dialog->WidgetTree->FindWidget(TEXT("SplitConfirm"))):nullptr;
        if(!Check(Input&&Confirm,TEXT("raid quantity controls ready")))return;Input->SetValue(7);Confirm->OnClicked.Broadcast();
        auto View=S->GetView();
        if(!Check(View.Backpack.Num()==3&&S->Count(View.Backpack,TEXT("Scrap"))==26&&View.Backpack[0].Quantity==13&&View.Backpack[2].Quantity==7&&
            S->Profile->PreparedBackpack.Num()==2,TEXT("raid split modifies current bag and preserves departure snapshot")))return;
    }
    if(Stage==12)Capture(TEXT("RaidBackpackQuantities"));
    if(Stage==13){if(!Check(S->FinishExpedition(false),TEXT("failed expedition settles after split")))return;}
    if(Stage==14){if(!Check(S->ReturnToBase(),TEXT("return to base after failed expedition")))return;}
    if(Stage==15)
    {
        if(Mode->IsRaid())return;auto View=S->GetView();
        if(FParse::Param(FCommandLine::Get(),TEXT("FactoryLobby")))
        {
            auto* Overview=UI->GetWidgetFromName(TEXT("OverviewPanel"));
            if(!Check(FactoryPC&&Overview&&Overview->IsVisible(),TEXT("failed expedition returns to actual factory lobby")))return;
        }
        if(!Check(View.Backpack.Num()==2&&S->Count(View.Backpack,TEXT("Scrap"))==26&&S->Count(View.Warehouse,TEXT("Scrap"))==12&&
            View.Backpack[1].StackId==SplitId&&S->ValidateProfile(S->Profile),TEXT("failure restores original prepared stacks")))return;
        if(!Check(FFileHelper::SaveStringToFile(State(),*(Directory/TEXT("expected.txt"))),TEXT("persist reload expectation")))return;
        UE_LOG(LogTemp,Display,TEXT("FPS_INVENTORY COMPLETE"));Stage=99;FPlatformMisc::RequestExit(false);return;
    }
    ++Stage;At=Now;
#endif
}
