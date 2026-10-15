#include "FPSExtractionSmoke.h"
#include "FPSExtractionMode.h"
#include "FPSExtractionUI.h"
#include "FPSExtractionGridUI.h"
#include "FPSExtractionWeaponViewport.h"
#include "Components/ScrollBox.h"
#include "FPSExtractionSubsystem.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Misc/CommandLine.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"
#include "Input/DragAndDrop.h"
#include "Widgets/SWidget.h"

void FFPSExtractionSmoke::CaptureTick(AFPSExtractionMode* Mode)
{
#if !UE_BUILD_SHIPPING
    if(!Mode || !FParse::Param(FCommandLine::Get(),TEXT("FPSExtractionUICapture")) || Mode->GetWorld()->GetTimeSeconds()<3)return;
    static int32 Stage=0; static double At=0; static bool Failed=false;
    if(Stage>=50)return;
    TArray<UUserWidget*> Widgets; UWidgetBlueprintLibrary::GetAllWidgetsOfClass(Mode,Widgets,UFPSExtractionUI::StaticClass());
    if(Widgets.Num()!=1)return;
    auto* UI=Cast<UFPSExtractionUI>(Widgets[0]); auto* S=Mode->GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>();
    const double Now=FPlatformTime::Seconds(); if(Now-At<.8)return;
    FString Directory;FParse::Value(FCommandLine::Get(),TEXT("ExtractionEvidence="),Directory);if(Directory.IsEmpty())return;
    int32 W=0,H=0;Mode->GetWorld()->GetFirstPlayerController()->GetViewportSize(W,H);
    auto Check=[&](bool Pass,const TCHAR* What)
    {
        UE_LOG(LogTemp,Display,TEXT("FPS_EXTRACTION_UI %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),What);
        Failed|=!Pass; return Pass;
    };
    auto Navigate=[&](FName Name)
    {
        auto* B=Cast<UFPSExtractionButton>(UI->WidgetTree->FindWidget(Name));
        if(Check(B!=nullptr,TEXT("navigation control exists")))B->OnClicked.Broadcast();
    };
    auto Capture=[&](const TCHAR* Name)
    {
        FScreenshotRequest::RequestScreenshot(Directory/FString::Printf(TEXT("%s-%dx%d.png"),Name,W,H),true,false);
        UE_LOG(LogTemp,Display,TEXT("FPS_EXTRACTION_UI CAPTURE %s %dx%d"),Name,W,H);
    };
    if(Stage==0)
    {
        // Script copies the supplied save into a unique UI profile. Fixtures cannot mutate the source profile.
        auto* C=S->CopyProfile();
        if(C)
        {
            Check(S->TryAdd(C->Warehouse,TEXT("Optic"),1,64) && S->TryAdd(C->Warehouse,TEXT("Magazine"),1,64) &&
                S->TryAdd(C->Warehouse,TEXT("Parts"),4,64) && S->Commit(C),TEXT("isolated UI fixture inventory"));
        }
        else Check(false,TEXT("profile loaded"));
        Navigate(TEXT("Overview"));
    }
    if(Stage==1){Capture(TEXT("Overview"));}
    if(Stage==2)Navigate(TEXT("Warehouse"));
    if(Stage==3)
    {
        Capture(TEXT("Warehouse"));
        auto* Store=Cast<UFPSExtractionGridUI>(UI->WidgetTree->FindWidget(TEXT("WarehouseGrid")));
        auto* Bag=Cast<UFPSExtractionGridUI>(UI->WidgetTree->FindWidget(TEXT("BaseBackpackGrid")));
        if(Check(Store && Bag && Store->GetCachedGeometry().GetLocalSize().X>0 && Bag->GetCachedGeometry().GetLocalSize().X>0,TEXT("saved grids have live geometry")))
        {
            const auto V=S->GetView(); const auto* Found=V.Warehouse.FindByPredicate([](const auto& X){return X.ItemId==TEXT("Scrap");});
            if(Check(Found!=nullptr,TEXT("drag source present")))
            {
                const auto Original=*Found;
                auto Drag=[&](UFPSExtractionGridUI* From,UFPSExtractionGridUI* To,const FFPSExtractionStack& Stack,int32 X,int32 Y,bool Rotate)
                {
                    const auto G=From->GetCachedGeometry();
                    // Grab the second cell to exercise the real drag offset, not only top-left placement.
                    const int32 Grab=Stack.bRotated?0:1;
                    const FVector2D P=G.LocalToAbsolute(FVector2D((Stack.X+Grab+.5f)*From->CellSize,(Stack.Y+.5f)*From->CellSize));
                    const TSet<FKey> Keys={EKeys::LeftMouseButton};
                    const FPointerEvent Press(0,P,P,Keys,EKeys::LeftMouseButton,0,FModifierKeysState());
                    From->NativeOnMouseButtonDown(G,Press);
                    UDragDropOperation* Op=nullptr; From->NativeOnDragDetected(G,Press,Op);
                    auto* D=Cast<UFPSExtractionGridDrag>(Op);
                    if(!Check(D && D->StackId==Stack.StackId && D->GrabX==Grab,TEXT("native mouse selection and drag offset")))return;
                    if(Rotate)UI->Dispatch(TEXT("SelectedRotate"));
                    UI->Refresh();
                    Check(UI->IsInventoryDragging(),TEXT("refresh preserves active drag"));
                    const auto Target=To->GetCachedGeometry();
                    const FVector2D Q=Target.LocalToAbsolute(FVector2D((X+D->GrabX+.5f)*To->CellSize,(Y+D->GrabY+.5f)*To->CellSize));
                    const FPointerEvent Pointer(0,Q,Q,Keys,EKeys::LeftMouseButton,0,FModifierKeysState());
                    const FDragDropEvent Event(Pointer,nullptr);
                    Check(To->NativeOnDragOver(Target,Event,Op) && To->NativeOnDrop(Target,Event,Op),TEXT("native drag preview and drop handlers"));
                    Check(!UI->IsInventoryDragging(),TEXT("drop clears drag state"));
                };
                Drag(Store,Bag,Original,2,0,true);
                auto Current=S->GetView();
                const auto* Moved=Current.Backpack.FindByPredicate([&](const auto& X){return X.StackId==Original.StackId;});
                if(Check(Moved && Moved->X==2 && Moved->Y==0 && Moved->bRotated,TEXT("native drag commits rotated footprint")))
                {
                    const auto InBag=*Moved;
                    Drag(Bag,Bag,InBag,5,3,false);
                    Current=S->GetView();Moved=Current.Backpack.FindByPredicate([&](const auto& X){return X.StackId==Original.StackId;});
                    Check(Moved && Moved->X==2 && Moved->Y==0 && S->Count(Current.Backpack,TEXT("Scrap"))==Original.Quantity,TEXT("invalid UI drop keeps inventory"));
                    Drag(Bag,Store,InBag,Original.X,Original.Y,true);
                    Current=S->GetView();
                    Check(S->Count(Current.Warehouse,TEXT("Scrap"))==Original.Quantity,TEXT("native return drag conserves quantity"));
                }
            }
        }
    }
    if(Stage==4)Navigate(TEXT("Production"));
    if(Stage==5)Capture(TEXT("Production"));
    if(Stage==6)Navigate(TEXT("Modification"));
    auto* Preview=Cast<UFPSExtractionWeaponViewport>(UI->WidgetTree->FindWidget(TEXT("WeaponViewport")));
    auto SelectSlot=[&](int32 Index)
    {
        if(!Check(Preview && Preview->HasModel(),TEXT("3D preview model and render target ready")))return;
        const auto G=Preview->GetCachedGeometry();const auto P=G.LocalToAbsolute(Preview->SlotCenter(EFPSAttachmentCategory(Index))*G.GetLocalSize());
        const FPointerEvent Click(0,P,P,{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());
        Check(Preview->NativeOnMouseButtonDown(G,Click).IsEventHandled() && int32(UI->GetSelectedAttachmentCategory())==Index,TEXT("model slot click selects compatible category"));
    };
    auto AttachmentAction=[&](FName Action,FName Id)
    {
        auto* Rows=Cast<UScrollBox>(UI->WidgetTree->FindWidget(TEXT("AttachmentRows")));UFPSExtractionButton* Found=nullptr;
        if(Rows)for(auto* Child:Rows->GetAllChildren())if(auto* Row=Cast<UUserWidget>(Child))
            if(auto* B=Cast<UFPSExtractionButton>(Row->WidgetTree->FindWidget(TEXT("RowAction")));B&&B->Action==Action&&B->ItemId==Id)Found=B;
        if(Check(Found&&Found->GetIsEnabled(),TEXT("real compatible inventory action exists")))
        {
            Found->OnHovered.Broadcast();
            auto* Text=Cast<UTextBlock>(UI->WidgetTree->FindWidget(TEXT("CandidateWeaponStats")));
            Check(Text && Text->GetText().ToString().Contains(TEXT("(+")),TEXT("candidate stats resolved before equip"));
            Found->OnClicked.Broadcast();
        }
    };
    auto Escape=[&]()
    {
        const FKeyEvent Key(EKeys::Escape,FModifierKeysState(),0,false,0,0);
        Check(UI->TakeWidget()->OnPreviewKeyDown(UI->GetCachedGeometry(),Key).IsEventHandled(),TEXT("Escape handled by workbench"));
    };
    if(Stage==7)
    {
        Capture(TEXT("WeaponOverview"));
        Check(Preview&&Preview->HasModel()&&!Preview->AreSlotsVisible()&&!UI->IsWeaponEditing(),TEXT("overview has actual model without mount cards"));
        for(const TCHAR* Name:{TEXT("PrimaryWeaponImage"),TEXT("SecondaryWeaponImage")})
        {
            const auto* Icon=Cast<UImage>(UI->WidgetTree->FindWidget(Name));
            Check(Icon&&Icon->GetBrush().GetResourceObject()&&Icon->GetCachedGeometry().GetLocalSize().X>100&&Icon->GetCachedGeometry().GetLocalSize().Y>40,TEXT("weapon card thumbnail has texture and live area"));
        }
    }
    if(Stage==8)Navigate(TEXT("ViewSecondaryWeapon"));
    if(Stage==9)
    {
        Capture(TEXT("WeaponOverviewM9"));
        const auto* Catalog=Mode->GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
        const auto* Definition=Catalog->Find(S->GetView().Loadout.Secondary.WeaponId);
        const auto* Button=Cast<UFPSExtractionButton>(UI->WidgetTree->FindWidget(TEXT("EnterWeaponModification")));
        Check(Preview&&Definition&&Preview->WeaponVisualName().ToString()==Definition->Pickup.Mesh.GetAssetName()&&UI->GetViewedWeaponSlot()==EFPSWeaponSlot::Secondary,TEXT("secondary overview uses actual M9 mesh"));
        Check(Button&&!Button->GetIsEnabled(),TEXT("unsupported secondary modification disabled"));
        UI->Dispatch(TEXT("EnterWeaponModification"));
        Check(!UI->IsWeaponEditing(),TEXT("unsupported modification action cannot enter editor"));
    }
    if(Stage==10)Navigate(TEXT("ViewPrimaryWeapon"));
    if(Stage==11)Navigate(TEXT("EnterWeaponModification"));
    if(Stage==12)
    {
        Capture(TEXT("Modification"));
        Check(UI->IsWeaponEditing()&&!UI->IsAttachmentDrawerOpen()&&Preview&&Preview->AreSlotsVisible(),TEXT("editing opens full weapon canvas"));
        Check(Preview&&Preview->VisualFor(EFPSAttachmentCategory::Optic).IsNone(),TEXT("bare optic slot"));
    }
    if(Stage==13)SelectSlot(0);
    if(Stage==14)
    {
        Capture(TEXT("ModificationDrawer"));
        Check(UI->IsAttachmentDrawerOpen(),TEXT("mount click opens attachment drawer"));
        auto* Drawer=UI->WidgetTree->FindWidget(TEXT("AttachmentDrawer"));
        if(Check(Preview&&Drawer&&Drawer->IsVisible(),TEXT("drawer is visible")))
        {
            const auto G=Preview->GetCachedGeometry(),D=Drawer->GetCachedGeometry();
            const auto Right=G.LocalToAbsolute(FVector2D(G.GetLocalSize().X,0));
            Check(Right.X<=D.GetAbsolutePosition().X+1,TEXT("drawer reserves space beside weapon canvas"));
        }
    }
    if(Stage==15)AttachmentAction(TEXT("Install"),TEXT("Optic"));
    if(Stage==16)
    {
        Check(Preview&&Preview->VisualFor(EFPSAttachmentCategory::Optic)==TEXT("SM_EXP_M4_Optic") && S->GetView().Loadout.Primary.AttachmentIds.Contains(TEXT("Attachment.M4_OpticFixed")),TEXT("optic inventory and preview agree"));
        Capture(TEXT("ModificationOptic"));
    }
    if(Stage==17)SelectSlot(2);
    if(Stage==18)AttachmentAction(TEXT("Install"),TEXT("Magazine"));
    if(Stage==19)
    {
        Check(Preview&&Preview->VisualFor(EFPSAttachmentCategory::Magazine)==TEXT("SM_EXP_M4_MagazineExtended"),TEXT("magazine mesh changes after install"));
        Navigate(TEXT("CloseAttachmentDrawer"));
        Check(UI->IsWeaponEditing()&&!UI->IsAttachmentDrawerOpen(),TEXT("drawer close keeps weapon editing"));
    }
    if(Stage==20)Capture(TEXT("ModificationEquipped"));
    if(Stage==21)SelectSlot(1);
    if(Stage==22)AttachmentAction(TEXT("Remove"),TEXT("Attachment.M4_MuzzleBrake"));
    if(Stage==23)
    {
        Check(Preview&&Preview->VisualFor(EFPSAttachmentCategory::Muzzle)==TEXT("SM_EXP_M4_MuzzleStandard"),TEXT("removal restores standard muzzle"));
        AttachmentAction(TEXT("Install"),TEXT("Brake"));
    }
    if(Stage==24)
    {
        Check(Preview&&Preview->VisualFor(EFPSAttachmentCategory::Muzzle)==TEXT("SM_EXP_M4_Brake"),TEXT("muzzle reinstall updates model"));
        SelectSlot(0);
    }
    if(Stage==25)AttachmentAction(TEXT("Remove"),TEXT("Attachment.M4_OpticFixed"));
    if(Stage==26)
    {
        Check(Preview&&Preview->VisualFor(EFPSAttachmentCategory::Optic).IsNone()&&S->Count(S->GetView().Warehouse,TEXT("Optic"))==1,TEXT("optic removal returns inventory and clears mesh"));
        AttachmentAction(TEXT("Install"),TEXT("Optic"));
    }
    if(Stage==27)
    {
        Escape();
        Check(UI->IsWeaponEditing()&&!UI->IsAttachmentDrawerOpen(),TEXT("Escape closes drawer before leaving editor"));
    }
    if(Stage==28)
    {
        if(Preview)
        {
            const auto G=Preview->GetCachedGeometry();const auto P=G.LocalToAbsolute(G.GetLocalSize()*.5);const TSet<FKey> Keys={EKeys::RightMouseButton};
            const FPointerEvent Down(0,P,P,Keys,EKeys::RightMouseButton,0,FModifierKeysState());
            const FPointerEvent Move(0,P+FVector2D(60,12),P,Keys,EKeys::Invalid,0,FModifierKeysState());
            const FPointerEvent Wheel(0,P,P,{},EKeys::Invalid,1,FModifierKeysState());
            Check(Preview->NativeOnMouseButtonDown(G,Down).IsEventHandled()&&Preview->NativeOnMouseMove(G,Move).IsEventHandled()&&Preview->NativeOnMouseButtonUp(G,Down).IsEventHandled()&&Preview->NativeOnMouseWheel(G,Wheel).IsEventHandled(),TEXT("orbit and zoom input handlers"));
        }
    }
    if(Stage==29)Capture(TEXT("ModificationOrbit"));
    if(Stage==30)Navigate(TEXT("ResetWorkbenchView"));
    if(Stage==31)
    {
        Escape();Check(!UI->IsWeaponEditing()&&Preview&&!Preview->AreSlotsVisible(),TEXT("Escape returns to weapon overview"));
    }
    if(Stage==32)Escape();
    if(Stage==33)
    {
        auto* Panel=UI->WidgetTree->FindWidget(TEXT("ModificationPanel"));
        auto* Overview=UI->WidgetTree->FindWidget(TEXT("Overview"));
        Check(Panel&&!Panel->IsVisible()&&Overview&&Overview->IsVisible(),TEXT("leaving workbench restores factory navigation"));
    }
    if(Stage==34)
    {
        if(!Failed){UE_LOG(LogTemp,Display,TEXT("FPS_EXTRACTION_UI COMPLETE"));}
        FPlatformMisc::RequestExit(false);Stage=50;return;
    }
    ++Stage;At=Now;
#endif
}
