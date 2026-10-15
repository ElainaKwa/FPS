#include "FPSExtractionUI.h"
#include "FPSExtractionLobby.h"
#include "FPSFactoryBase.h"
#include "FPSSystemSettings.h"
#include "FPSDemoPlayerController.h"
#include "FPSExtractionSubsystem.h"
#include "FPSExtractionGridUI.h"
#include "FPSExtractionWeaponViewport.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "FPSInterface.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

namespace ExtractionUI
{
const TCHAR* Root=TEXT("/Game/FPSDemo/Art/UI/ExtractionTerminal/");
template<class T>T* Find(UUserWidget* UI,FName Name){return UI&&UI->WidgetTree?Cast<T>(UI->WidgetTree->FindWidget(Name)):nullptr;}
void Text(UUserWidget* UI,FName Name,const FString& Value)
{if(auto* T=Find<UTextBlock>(UI,Name))if(T->GetText().ToString()!=Value)T->SetText(FText::FromString(Value));}
FString AssetClass(const TCHAR* Name){return FString(Root)+Name+TEXT(".")+Name+TEXT("_C");}
const TCHAR* CategoryName(EFPSAttachmentCategory Category)
{return FPSAttachments::Label(Category);}
bool ResolveVariant(const UFPSWeaponCatalog* Catalog,const FFPSWeaponInstance& Gun,FName AttachmentId,bool Remove,FFPSResolvedWeaponStats& Stats)
{
    if(!Catalog)return false;const auto* Part=Catalog->FindAttachment(AttachmentId);if(!Part)return false;
    auto Ids=Gun.AttachmentIds;
    if(Remove)Ids.Remove(AttachmentId);
    else
    {Ids.RemoveAll([&](FName Id){const auto* Old=Catalog->FindAttachment(Id);return Old&&Old->Category==Part->Category;});Ids.Add(AttachmentId);}
    TArray<FString> Errors;return Catalog->Resolve(Gun.WeaponId,Ids,Stats,Errors);
}
FString CurrentStats(const FFPSResolvedWeaponStats& S)
{return FString::Printf(TEXT("容量  %d 发\n换弹  %.2f 秒\n开镜  %.2f 秒\n后坐力倍率  纵 %.2f / 横 %.2f"),S.MagazineSize,S.ReloadDuration,S.AimTransitionTime,S.VerticalRecoilMultiplier,S.HorizontalRecoilMultiplier)+FString::Printf(TEXT("\n射程  %.0f 米"),S.Range/100.f);}
FString CandidateStats(const FFPSResolvedWeaponStats& S,const FFPSResolvedWeaponStats& C)
{return FString::Printf(TEXT("容量  %d  (%+d)\n换弹  %.2f 秒  (%+.2f)\n开镜  %.2f 秒  (%+.2f)\n后坐力  纵 %+.2f / 横 %+.2f"),S.MagazineSize,S.MagazineSize-C.MagazineSize,S.ReloadDuration,S.ReloadDuration-C.ReloadDuration,S.AimTransitionTime,S.AimTransitionTime-C.AimTransitionTime,S.VerticalRecoilMultiplier-C.VerticalRecoilMultiplier,S.HorizontalRecoilMultiplier-C.HorizontalRecoilMultiplier)+FString::Printf(TEXT("\n射程  %.0f 米  (%+.0f)"),S.Range/100.f,(S.Range-C.Range)/100.f);}
}
void UFPSExtractionButton::Bind(UFPSExtractionUI* Target)
{OwnerUI=Target;OnClicked.AddUniqueDynamic(this,&UFPSExtractionButton::Activate);OnHovered.AddUniqueDynamic(this,&UFPSExtractionButton::HoverCandidate);OnUnhovered.AddUniqueDynamic(this,&UFPSExtractionButton::LeaveCandidate);}
void UFPSExtractionButton::Activate(){if(OwnerUI.IsValid())OwnerUI->Dispatch(Action,ItemId,Quantity);}
void UFPSExtractionButton::HoverCandidate(){if(OwnerUI.IsValid()&&(Action==TEXT("Install")||Action==TEXT("Remove")))OwnerUI->PreviewAttachmentStats(Action,ItemId);}
void UFPSExtractionButton::LeaveCandidate(){if(OwnerUI.IsValid())OwnerUI->ClearAttachmentStats(Action,ItemId);}
UFPSExtractionUI* UFPSExtractionUI::Open(APlayerController* Owner,bool bOverlay)
{
    if(!Owner)return nullptr;
    auto* Class=LoadClass<UFPSExtractionUI>(nullptr,*ExtractionUI::AssetClass(TEXT("WBP_ExtractionTerminal")));
    if(!Class){UE_LOG(LogTemp,Error,TEXT("Extraction UI asset missing; run build_extraction_ui.py after compiling."));return nullptr;}
    auto* UI=CreateWidget<UFPSExtractionUI>(Owner,Class);
    if(UI)UI->bBagOpen=false; // Base/Result are always full panels; Raid starts with the light HUD.
    return UI;
}
UFPSExtractionSubsystem* UFPSExtractionUI::Model() const
{return GetGameInstance()?GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>():nullptr;}
void UFPSExtractionUI::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);
    WidgetTree->ForEachWidget([this](UWidget* W){if(auto* B=Cast<UFPSExtractionButton>(W))B->Bind(this);});
    if(auto* Preview=ExtractionUI::Find<UFPSExtractionWeaponViewport>(this,TEXT("WeaponViewport")))
    {Preview->OnSlotSelected.RemoveAll(this);Preview->OnSlotSelected.AddUObject(this,&UFPSExtractionUI::SelectAttachmentCategory);}
    Refresh();
}
void UFPSExtractionUI::NativeDestruct()
{
    if(LobbyScene)LobbyScene->Release();
    if(auto* Preview=ExtractionUI::Find<UFPSExtractionWeaponViewport>(this,TEXT("WeaponViewport")))
    {Preview->OnSlotSelected.RemoveAll(this);Preview->SetPreviewActive(false);}
    Super::NativeDestruct();
}
void UFPSExtractionUI::NativeTick(const FGeometry& Geometry,float DeltaTime)
{Super::NativeTick(Geometry,DeltaTime);RefreshClock+=DeltaTime;if(RefreshClock>=.1f){RefreshClock=0;Refresh();}}
FReply UFPSExtractionUI::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    // Tunnel before a focused button turns Tab into navigation; do not double-toggle in the controller.
    if(Phase==EFPSExtractionPhase::Raid&&bBagOpen&&(Event.GetKey()==EKeys::Escape || ([&](){const auto* S=UFPSSystemSettings::Get(this);const auto* B=S?S->Binding("Inventory"):nullptr;return B&&(Event.GetKey()==B->Primary||Event.GetKey()==B->Secondary);})()))
    {SetPanelOpen(false);return FReply::Handled();}
    if(Event.GetKey()==EKeys::R&&IsPanelOpen()&&!(Phase==EFPSExtractionPhase::Base&&Tab==3))
    {Dispatch(TEXT("SelectedRotate"));return FReply::Handled();}
    if(Event.GetKey()==EKeys::Escape&&Phase==EFPSExtractionPhase::Base)
    {if(Tab==3){if(!Event.IsRepeat())WorkbenchBack();}else if(ActiveDrag){UWidgetBlueprintLibrary::CancelDragDrop();EndInventoryDrag();}else if(auto* PC=Cast<AFPSDemoPlayerController>(GetOwningPlayer()))PC->OpenSettings(this);return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
void UFPSExtractionUI::SetText(FName Name,const FString& Value){ExtractionUI::Text(this,Name,Value);}
void UFPSExtractionUI::SetShown(FName Name,bool bShow)
{if(auto* W=WidgetTree->FindWidget(Name))W->SetVisibility(bShow?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed);}
void UFPSExtractionUI::SetEnabled(FName Name,bool bEnabled){if(auto* W=WidgetTree->FindWidget(Name))W->SetIsEnabled(bEnabled);}
bool UFPSExtractionUI::IsPanelOpen() const{return Phase!=EFPSExtractionPhase::Raid||bBagOpen;}
void UFPSExtractionUI::SetPanelOpen(bool bOpen)
{if(!bOpen&&ActiveDrag){UWidgetBlueprintLibrary::CancelDragDrop();EndInventoryDrag();}bBagOpen=bOpen;bConfirmAbort=false;Refresh();}
void UFPSExtractionUI::SelectInventoryStack(EFPSExtractionContainer C,FGuid Id)
{SelectedContainer=C;SelectedStack=Id;LocalFeedback.Empty();Refresh();}
bool UFPSExtractionUI::IsInventorySelected(EFPSExtractionContainer C,FGuid Id) const
{return SelectedContainer==C&&SelectedStack==Id&&Id.IsValid();}
bool UFPSExtractionUI::CanInventoryMove(EFPSExtractionContainer From,EFPSExtractionContainer To) const
{
    auto* S=Model();if(!S)return false;const auto V=S->GetView();
    return V.bStorageHealthy&&(V.Phase==EFPSExtractionPhase::Base||
        (V.Phase==EFPSExtractionPhase::Raid&&From==EFPSExtractionContainer::Backpack&&To==EFPSExtractionContainer::Backpack));
}
void UFPSExtractionUI::BeginInventoryDrag(UFPSExtractionGridDrag* Drag){ActiveDrag=Drag;}
void UFPSExtractionUI::EndInventoryDrag(){ActiveDrag=nullptr;LocalFeedback.Empty();}
void UFPSExtractionUI::CommitInventoryDrag(UFPSExtractionGridDrag* Drag,EFPSExtractionContainer To,int32 X,int32 Y)
{
    if(!Drag||Drag->OwnerUI!=this)return;
    auto* S=Model();if(S&&S->MoveStack(Drag->From,Drag->StackId,To,X,Y,Drag->bRotated))
    {
        const auto View=S->GetView();const auto& Source=Drag->From==EFPSExtractionContainer::Warehouse?View.Warehouse:View.Backpack;
        const bool Remainder=Source.ContainsByPredicate([Drag](const auto& Stack){return Stack.StackId==Drag->StackId;});
        SelectedContainer=Remainder?Drag->From:To;SelectedStack=Drag->StackId;
    }
    EndInventoryDrag();Refresh();
}
void UFPSExtractionUI::InventoryFeedback(const FString& Text)
{LocalFeedback=Text;SetText(TEXT("InventoryFeedback"),Text);SetText(TEXT("BagFeedback"),Text);}
void UFPSExtractionUI::SelectAttachmentCategory(EFPSAttachmentCategory Category)
{
    if(Phase!=EFPSExtractionPhase::Base||Tab!=3||!bWeaponEditing||bViewSecondary)return;
    if(!FPSAttachments::IsCategory(Category))return;
    if(SelectedCategory==Category&&bAttachmentDrawer)return;
    SelectedCategory=Category;bAttachmentDrawer=true;ComparedAction=NAME_None;ComparedId=NAME_None;LastRows.Empty();Refresh();
}
void UFPSExtractionUI::WorkbenchBack()
{
    if(Tab!=3||Phase!=EFPSExtractionPhase::Base)return;
    if(bAttachmentDrawer)bAttachmentDrawer=false;
    else if(bWeaponEditing)bWeaponEditing=false;
    else Tab=0;
    ComparedAction=NAME_None;ComparedId=NAME_None;LastRows.Empty();Refresh();
}
bool UFPSExtractionUI::CanModifyViewedWeapon(const FFPSExtractionView& V) const
{
    const auto* Catalog=GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    const auto* Definition=Catalog?Catalog->Find(V.Loadout.Primary.WeaponId):nullptr;
    // The extraction backend exposes installation for the actual primary instance only.
    return !bViewSecondary&&V.Loadout.Primary.IsValid()&&Definition&&Definition->MountIds.Num()>0&&Definition->AvailableAttachments.Num()>0;
}
void UFPSExtractionUI::RefreshWeaponOverview(const FFPSExtractionView& V)
{
    const auto* Catalog=GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    const auto& Gun=bViewSecondary?V.Loadout.Secondary:V.Loadout.Primary;
    const auto* Definition=Catalog?Catalog->Find(Gun.WeaponId):nullptr;
    const FString Name=Definition?Definition->DisplayName.ToString():Gun.WeaponId.ToString();
    SetText(TEXT("WorkbenchWeaponTitle"),Name);
    if(auto* Title=ExtractionUI::Find<UTextBlock>(this,TEXT("WorkbenchWeaponTitle")))if(auto* TitleSlot=Cast<UCanvasPanelSlot>(Title->Slot))TitleSlot->SetPosition(FVector2D(bWeaponEditing?40:480,106));
    SetText(TEXT("OverviewWeaponName"),Name);
    SetText(TEXT("WorkbenchMode"),bWeaponEditing?TEXT("武器改装"):TEXT("持有武器 / 概览"));
    SetText(TEXT("WorkbenchBackLabel"),bAttachmentDrawer?TEXT("收起配件 / Esc"):bWeaponEditing?TEXT("返回概览 / Esc"):TEXT("返回工厂 / Esc"));
    SetText(TEXT("WeaponInstanceNote"),FString::Printf(TEXT("%s  ·  弹药 %d / %d  ·  配件 %d 件"),bViewSecondary?TEXT("副武器"):TEXT("主武器"),Gun.Magazine,Gun.Reserve,Gun.AttachmentIds.Num()));
    SetEnabled(TEXT("EnterWeaponModification"),CanModifyViewedWeapon(V));
    SetText(TEXT("WeaponModificationNote"),CanModifyViewedWeapon(V)?TEXT("查看当前装配，使用仓库中的配件改装。"):TEXT("该武器暂无可用改装接口，可查看模型与参数。"));
    const FName Buttons[]={TEXT("ViewPrimaryWeapon"),TEXT("ViewSecondaryWeapon")};
    for(int32 I=0;I<2;++I)
    {
        const bool Selected=(I==1)==bViewSecondary;
        if(auto* B=ExtractionUI::Find<UFPSExtractionButton>(this,Buttons[I]))if(Theme)B->SetStyle(Selected?Theme->LightButton:Theme->DarkButton);
        const TCHAR* Prefix=I==0?TEXT("Primary"):TEXT("Secondary");
        for(const TCHAR* Suffix:{TEXT("WeaponName"),TEXT("WeaponRole")})
            if(auto* T=ExtractionUI::Find<UTextBlock>(this,FName(*(FString(Prefix)+Suffix))))if(Theme)T->SetColorAndOpacity(Selected?Theme->Ink:Theme->Paper);
    }
    SetEnabled(TEXT("ViewPrimaryWeapon"),V.Loadout.Primary.IsValid());SetEnabled(TEXT("ViewSecondaryWeapon"),V.Loadout.Secondary.IsValid());
    SetText(TEXT("PrimaryWeaponRole"),V.Loadout.Primary.IsValid()?TEXT("主武器 · 已持有"):TEXT("主武器 · 未配置"));
    SetText(TEXT("SecondaryWeaponRole"),V.Loadout.Secondary.IsValid()?TEXT("副武器 · 已持有"):TEXT("副武器 · 未配置"));
    FFPSResolvedWeaponStats Stats;TArray<FString> Errors;
    if(!Definition||!Catalog->Resolve(Gun.WeaponId,Gun.AttachmentIds,Stats,Errors))
    {SetText(TEXT("OverviewWeaponStats"),TEXT("武器参数尚未就绪"));return;}
    const TCHAR* FireMode=Definition->FireMode==EFPSFireMode::Single?TEXT("单发"):Definition->FireMode==EFPSFireMode::Burst?TEXT("点射"):TEXT("自动");
    SetText(TEXT("OverviewWeaponStats"),FString::Printf(TEXT("%.0f\n%.0f 米\n%.0f 发/分\n%d 发\n%.2f 秒\n%.2f 秒\n%s\n纵 %.2f / 横 %.2f"),
        Definition->Damage,Stats.Range/100.f,Definition->FireInterval>0?60.f/Definition->FireInterval:0.f,Stats.MagazineSize,Stats.ReloadDuration,Stats.AimTransitionTime,FireMode,Stats.VerticalRecoilMultiplier,Stats.HorizontalRecoilMultiplier));
}
void UFPSExtractionUI::PreviewAttachmentStats(FName Action,FName Id)
{if(Phase==EFPSExtractionPhase::Base&&Tab==3&&bWeaponEditing&&bAttachmentDrawer){ComparedAction=Action;ComparedId=Id;if(auto* S=Model())RefreshAttachmentStats(S->GetView());}}
void UFPSExtractionUI::ClearAttachmentStats(FName Action,FName Id)
{if(ComparedAction==Action&&ComparedId==Id){ComparedAction=NAME_None;ComparedId=NAME_None;if(auto* S=Model())RefreshAttachmentStats(S->GetView());}}
void UFPSExtractionUI::RefreshAttachmentStats(const FFPSExtractionView& V)
{
    auto* S=Model();const auto* Catalog=GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    SetText(TEXT("AttachmentCategory"),FString(ExtractionUI::CategoryName(SelectedCategory))+TEXT(" / 兼容库存"));
    const FName Names[]={TEXT("SlotOptic"),TEXT("SlotMuzzle"),TEXT("SlotMagazine")};
    for(int32 I=0;I<3;++I)if(auto* B=ExtractionUI::Find<UFPSExtractionButton>(this,Names[I]))if(Theme)
    {const bool Selected=I==int32(SelectedCategory);B->SetStyle(Selected?Theme->AccentButton:Theme->DarkButton);
    if(auto* T=ExtractionUI::Find<UTextBlock>(this,FName(*(Names[I].ToString()+TEXT("Label")))))T->SetColorAndOpacity(Selected?Theme->Ink:Theme->Paper);}
    FFPSResolvedWeaponStats Current;TArray<FString> Errors;
    if(!Catalog||!Catalog->Resolve(V.Loadout.Primary.WeaponId,V.Loadout.Primary.AttachmentIds,Current,Errors))
    {SetText(TEXT("CurrentWeaponStats"),TEXT("武器参数尚未就绪"));SetText(TEXT("CandidateWeaponStats"),TEXT(""));SetText(TEXT("CandidateStatsTitle"),TEXT("候选差异"));return;}
    SetText(TEXT("CurrentWeaponStats"),ExtractionUI::CurrentStats(Current));
    SetText(TEXT("CandidateStatsTitle"),TEXT("候选差异"));
    SetText(TEXT("CandidateWeaponStats"),TEXT("悬停安装或卸下按钮\n查看参数变化\n点击后立即装配"));
    FName PartId;FString Name;
    if(ComparedAction==TEXT("Install")&&S)
    {if(const auto* Item=S->FindItem(ComparedId);Item&&S->Count(V.Warehouse,ComparedId)>0){PartId=Item->AttachmentId;Name=Item->Name.ToString();}}
    else if(ComparedAction==TEXT("Remove")&&V.Loadout.Primary.AttachmentIds.Contains(ComparedId))
    {PartId=ComparedId;if(const auto* Part=Catalog->FindAttachment(PartId))Name=Part->DisplayName.ToString();
    if(S)for(const auto& Item:S->GetItems())if(Item.AttachmentId==PartId){Name=Item.Name.ToString();break;}}
    const auto* Part=Catalog->FindAttachment(PartId);FFPSResolvedWeaponStats Candidate;
    if(Part&&Part->Category==SelectedCategory&&ExtractionUI::ResolveVariant(Catalog,V.Loadout.Primary,PartId,ComparedAction==TEXT("Remove"),Candidate))
    {SetText(TEXT("CandidateStatsTitle"),(ComparedAction==TEXT("Remove")?TEXT("卸下后 / "):TEXT("安装后 / "))+Name);SetText(TEXT("CandidateWeaponStats"),ExtractionUI::CandidateStats(Candidate,Current));}
}
void UFPSExtractionUI::RebuildAttachmentRows(const FFPSExtractionView& V)
{
    auto* List=ExtractionUI::Find<UScrollBox>(this,TEXT("AttachmentRows"));auto* S=Model();
    const auto* Catalog=GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();if(!List||!S)return;
    const float Offset=List->GetScrollOffset();List->ClearChildren();
    auto* Class=LoadClass<UUserWidget>(nullptr,*ExtractionUI::AssetClass(TEXT("WBP_ExtractionAttachmentRow")));if(!Class)return;
    auto Add=[&](FName Action,FName Id,const FString& Name,const FString& Detail,const FString& Count,const FString& Label,bool Enabled)
    {
        auto* W=CreateWidget<UUserWidget>(GetOwningPlayer(),Class);if(!W)return;
        ExtractionUI::Text(W,TEXT("RowTitle"),Name);ExtractionUI::Text(W,TEXT("RowDetail"),Detail);ExtractionUI::Text(W,TEXT("RowCount"),Count);
        ExtractionUI::Text(W,TEXT("RowActionLabel"),Label);ExtractionUI::Text(W,TEXT("RowReason"),V.bStorageHealthy?TEXT(""):TEXT("存档不可用"));
        if(auto* B=ExtractionUI::Find<UFPSExtractionButton>(W,TEXT("RowAction")))
        {B->Action=Action;B->ItemId=Id;B->Bind(this);B->SetIsEnabled(Enabled);B->SetVisibility(Action.IsNone()?ESlateVisibility::Collapsed:ESlateVisibility::Visible);}
        if(auto* Slot=Cast<UScrollBoxSlot>(List->AddChild(W)))Slot->SetPadding(FMargin(0,0,0,10));
    };
    if(Catalog)
    {
        const auto Items=S->GetItems();
        for(FName Id:V.Loadout.Primary.AttachmentIds)
            if(const auto* Part=Catalog->FindAttachment(Id);Part&&Part->Category==SelectedCategory)
            {FFPSResolvedWeaponStats Stats;const bool Valid=ExtractionUI::ResolveVariant(Catalog,V.Loadout.Primary,Id,true,Stats);
            const auto* Item=Items.FindByPredicate([&](const auto& Entry){return Entry.AttachmentId==Id;});
            Add(TEXT("Remove"),Id,Item?Item->Name.ToString():Part->DisplayName.ToString(),TEXT("当前已装配\n卸下后退回仓库"),TEXT("已安装"),TEXT("卸下"),V.bStorageHealthy&&Valid);}
        // Iterate the item catalog rather than stacks: repeated ItemIds aggregate their available count.
        for(const auto& Item:S->GetItems())
        {
            const int32 Count=S->Count(V.Warehouse,Item.ItemId);const auto* Part=Catalog->FindAttachment(Item.AttachmentId);
            if(Count<=0||!Part||Part->Category!=SelectedCategory||V.Loadout.Primary.AttachmentIds.Contains(Part->AttachmentId))continue;
            FFPSResolvedWeaponStats Stats;if(!ExtractionUI::ResolveVariant(Catalog,V.Loadout.Primary,Part->AttachmentId,false,Stats))continue;
            Add(TEXT("Install"),Item.ItemId,Item.Name.ToString(),Item.Description.ToString(),FString::Printf(TEXT("仓库 ×%d"),Count),TEXT("安装"),V.bStorageHealthy);
        }
    }
    if(List->GetChildrenCount()==0)Add(NAME_None,NAME_None,TEXT("没有兼容库存配件"),TEXT("通过搜获或加工取得此类配件。"),TEXT(""),TEXT(""),false);
    List->SetScrollOffset(Offset);
}
void UFPSExtractionUI::Dispatch(FName Action,FName ItemId,int32 Quantity)
{
    auto* S=Model();if(!S)return;
    if(Action==TEXT("LobbySettings")){if(auto* PC=Cast<AFPSDemoPlayerController>(GetOwningPlayer()))PC->OpenSettings(this);return;}
    if(Action==TEXT("LobbyFactory")){if(auto* PC=Cast<AFPSFactoryController>(GetOwningPlayer()))PC->CloseTerminal();return;}
    if(Action==TEXT("WorkbenchBack")){WorkbenchBack();return;}
    if(Action==TEXT("CloseAttachmentDrawer")){if(Tab==3){bAttachmentDrawer=false;ComparedAction=NAME_None;ComparedId=NAME_None;Refresh();}return;}
    if(Action==TEXT("WorkbenchOverview")){if(Tab==3){bWeaponEditing=false;bAttachmentDrawer=false;ComparedAction=NAME_None;ComparedId=NAME_None;Refresh();}return;}
    if(Action==TEXT("ViewPrimaryWeapon")||Action==TEXT("ViewSecondaryWeapon"))
    {
        if(Phase!=EFPSExtractionPhase::Base||Tab!=3||bWeaponEditing)return;
        const auto V=S->GetView();const bool Secondary=Action==TEXT("ViewSecondaryWeapon");if(!(Secondary?V.Loadout.Secondary:V.Loadout.Primary).IsValid())return;
        bViewSecondary=Secondary;bAttachmentDrawer=false;ComparedAction=NAME_None;ComparedId=NAME_None;
        if(auto* Preview=ExtractionUI::Find<UFPSExtractionWeaponViewport>(this,TEXT("WeaponViewport")))Preview->ResetView();Refresh();return;
    }
    if(Action==TEXT("EnterWeaponModification"))
    {if(Phase==EFPSExtractionPhase::Base&&Tab==3&&CanModifyViewedWeapon(S->GetView())){bWeaponEditing=true;bAttachmentDrawer=false;ComparedAction=NAME_None;ComparedId=NAME_None;LastRows.Empty();Refresh();}return;}
    if(Action==TEXT("ResetWorkbenchView"))
    {if(Tab==3)if(auto* Preview=ExtractionUI::Find<UFPSExtractionWeaponViewport>(this,TEXT("WeaponViewport")))Preview->ResetView();return;}
    if((Action==TEXT("Install")||Action==TEXT("Remove"))&&!(Phase==EFPSExtractionPhase::Base&&Tab==3&&bWeaponEditing&&bAttachmentDrawer&&!bViewSecondary))return;
    if(Action==TEXT("SlotOptic")){SelectAttachmentCategory(EFPSAttachmentCategory::Optic);return;}
    if(Action==TEXT("SlotMuzzle")){SelectAttachmentCategory(EFPSAttachmentCategory::Muzzle);return;}
    if(Action==TEXT("SlotMagazine")){SelectAttachmentCategory(EFPSAttachmentCategory::Magazine);return;}
    if(Action==TEXT("Overview"))Tab=0;
    else if(Action==TEXT("Warehouse"))Tab=1;
    else if(Action==TEXT("SellTab"))Tab=1;
    else if(Action==TEXT("Production"))Tab=2;
    else if(Action==TEXT("Modification")){Tab=3;bWeaponEditing=false;bAttachmentDrawer=false;bViewSecondary=false;}
    else if(Action==TEXT("CloseBag")){SetPanelOpen(false);return;}
    else if(Action==TEXT("Abort")){bConfirmAbort=true;Refresh();return;}
    else if(Action==TEXT("CancelAbort")){bConfirmAbort=false;Refresh();return;}
    else if(Action==TEXT("ConfirmAbort")){bConfirmAbort=false;S->AbortExpedition();}
    else if(Action==TEXT("Depart"))S->StartExpedition();
    else if(Action==TEXT("Return"))S->ReturnToBase();
    else if(Action==TEXT("Sell"))S->Sell(ItemId,Quantity);
    else if(Action==TEXT("Craft"))S->StartProduction(ItemId);
    else if(Action==TEXT("Claim"))S->ClaimProduction();
    else if(Action==TEXT("Install"))S->InstallAttachment(ItemId);
    else if(Action==TEXT("Remove"))S->RemoveAttachment(ItemId);
    else if(Action==TEXT("Resupply"))S->Resupply();
    else if(Action==TEXT("SelectedTransfer"))S->AutoTransferStack(SelectedContainer,SelectedStack,SelectedContainer==EFPSExtractionContainer::Warehouse?EFPSExtractionContainer::Backpack:EFPSExtractionContainer::Warehouse);
    else if(Action==TEXT("SelectedSell"))S->SellStack(SelectedStack);
    else if(Action==TEXT("SelectedDrop"))S->DropStack(SelectedStack);
    else if(Action==TEXT("SelectedRotate"))
    {
        if(ActiveDrag){ActiveDrag->Rotate();return;}
        const auto V=S->GetView();const auto& Inventory=SelectedContainer==EFPSExtractionContainer::Warehouse?V.Warehouse:V.Backpack;
        if(const auto* Stack=Inventory.FindByPredicate([this](const auto& I){return I.StackId==SelectedStack;}))
            S->MoveStack(SelectedContainer,SelectedStack,SelectedContainer,Stack->X,Stack->Y,!Stack->bRotated);
    }
    LocalFeedback.Empty();
    ComparedAction=NAME_None;ComparedId=NAME_None;
    // Travel may already be scheduled: do not fabricate a successful inventory change.
    LastRows.Empty();Refresh();
}
FString UFPSExtractionUI::RowSignature(const FFPSExtractionView& V) const
{
    FString S=FString::Printf(TEXT("%d/%d/%d/%d/%d/%s/"),int32(V.Phase),Tab,V.Credits,V.bStorageHealthy,V.bProductionReady,*V.ProductionRecipe.ToString());
    for(const auto* List:{&V.Warehouse,&V.Backpack,&V.LastHaul}){S+=TEXT("|");for(const auto& I:*List)S+=I.ItemId.ToString()+FString::FromInt(I.Quantity)+TEXT(";");}
    for(FName Id:V.Loadout.Primary.AttachmentIds)S+=Id.ToString()+TEXT(";");
    if(Tab==3){const auto* Catalog=GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();S+=FString::Printf(TEXT("/category%d/ready%d/edit%d/drawer%d"),int32(SelectedCategory),Catalog&&Catalog->IsReady(),bWeaponEditing,bAttachmentDrawer);}
    return S;
}
void UFPSExtractionUI::Refresh()
{
    auto* S=Model();if(!S||!WidgetTree)return;
    const auto V=S->GetView();
    if(Phase!=V.Phase){if(ActiveDrag){UWidgetBlueprintLibrary::CancelDragDrop();EndInventoryDrag();}SelectedStack.Invalidate();LocalFeedback.Empty();bWeaponEditing=false;bAttachmentDrawer=false;bViewSecondary=false;}
    Phase=V.Phase;
    const bool Base=Phase==EFPSExtractionPhase::Base,Raid=Phase==EFPSExtractionPhase::Raid,Result=Phase==EFPSExtractionPhase::Result;
    const bool HasLobby=WidgetTree->FindWidget(TEXT("LobbyLayoutVersion"))!=nullptr;
    const bool Lobby=HasLobby&&Base&&Tab==0;
    SetShown(TEXT("BasePanel"),Base&&Tab!=3&&!Lobby);SetShown(TEXT("RaidHUD"),Raid);
    if(!LobbyScene)LobbyScene=NewObject<UFPSExtractionLobbyScene>(this);
    LobbyScene->SetActive(GetOwningPlayer(),LobbyPresentation,Lobby);
    SetShown(TEXT("OverviewPanel"),Base&&Tab==0);SetShown(TEXT("InventoryPanel"),Base&&Tab==1);
    SetShown(TEXT("WorkbenchPanel"),Base&&Tab==2);
    SetShown(TEXT("EquipmentPanel"),Base&&Tab!=3);
    SetShown(TEXT("ModificationPanel"),Base&&Tab==3);
    SetShown(TEXT("WeaponOverviewPanel"),Base&&Tab==3&&!bWeaponEditing);
    SetShown(TEXT("WeaponEnterPanel"),Base&&Tab==3&&!bWeaponEditing);
    SetShown(TEXT("AttachmentDrawer"),Base&&Tab==3&&bWeaponEditing&&bAttachmentDrawer);
    if(auto* Preview=ExtractionUI::Find<UFPSExtractionWeaponViewport>(this,TEXT("WeaponViewport")))
    {
        Preview->SetPreviewActive(Base&&Tab==3);
        if(Base&&Tab==3)
        {
            Preview->SetOverviewMode(!bWeaponEditing);Preview->SetSlotsVisible(bWeaponEditing);
            Preview->Present(bViewSecondary?V.Loadout.Secondary:V.Loadout.Primary);Preview->SetSelectedCategory(SelectedCategory);
            if(auto* ViewSlot=Cast<UCanvasPanelSlot>(Preview->Slot))
            {const FVector2D Position(bWeaponEditing?40:480,150),Size(bWeaponEditing?(bAttachmentDrawer?1150:1520):1080,650);
            if(ViewSlot->GetPosition()!=Position)ViewSlot->SetPosition(Position);if(ViewSlot->GetSize()!=Size)ViewSlot->SetSize(Size);}
        }
    }
    if(Base&&Tab==3){RefreshWeaponOverview(V);if(bWeaponEditing&&bAttachmentDrawer)RefreshAttachmentStats(V);}
    SetShown(TEXT("BagPanel"),Raid&&bBagOpen);SetShown(TEXT("ResultPanel"),Result);
    SetShown(TEXT("FullBackdrop"),!Raid&&!Lobby);SetShown(TEXT("BagBackdrop"),Raid&&bBagOpen);
    SetShown(TEXT("AbortConfirm"),Raid&&bBagOpen&&bConfirmAbort);
    SetEnabled(TEXT("RaidBackpackGrid"),!bConfirmAbort);SetEnabled(TEXT("RaidSelectionPanel"),!bConfirmAbort);SetEnabled(TEXT("Abort"),!bConfirmAbort);SetEnabled(TEXT("CloseBag"),!bConfirmAbort);
    SetText(TEXT("Credits"),FString::Printf(TEXT("%d"),V.Credits));
    SetText(TEXT("LobbyCredits"),FText::AsNumber(V.Credits).ToString());
    SetText(TEXT("LobbyLoadout"),FString::Printf(TEXT("整备背包  %d / %d 格  ·  主武器配件 %d 件"),V.BackpackUsed,V.BackpackSlots,V.Loadout.Primary.AttachmentIds.Num()));
    SetText(TEXT("LobbyFeedback"),V.bStorageHealthy?V.Feedback:TEXT("存档不可写，暂时无法出征。 ")+V.Feedback);
    SetEnabled(TEXT("LobbyDepart"),Base&&V.bStorageHealthy);
    SetEnabled(TEXT("LobbyFactory"),Base&&GetOwningPlayer()&&GetOwningPlayer()->IsA<AFPSFactoryController>());
    SetText(TEXT("Storage"),FString::Printf(TEXT("仓库  %d / %d 格"),V.WarehouseUsed,V.WarehouseSlots));
    SetText(TEXT("Feedback"),V.bStorageHealthy?V.Feedback:TEXT("存档不可写，请检查后重试。\n")+V.Feedback);
    SetText(TEXT("WorkbenchFeedback"),V.bStorageHealthy?V.Feedback:TEXT("存档不可写，请检查后重试。 ")+V.Feedback);
    if(LastBackendFeedback!=V.Feedback){LocalFeedback.Empty();LastBackendFeedback=V.Feedback;}
    const FString Feedback=LocalFeedback.IsEmpty()?V.Feedback:LocalFeedback;
    SetText(TEXT("InventoryFeedback"),Feedback);SetText(TEXT("BagFeedback"),Feedback);SetText(TEXT("ResultFeedback"),V.Feedback);
    SetText(TEXT("BagCapacity"),FString::Printf(TEXT("出征背包  %d / %d 格"),V.BackpackUsed,V.BackpackSlots));
    SetText(TEXT("RaidBagCapacity"),FString::Printf(TEXT("%d / %d 格"),V.BackpackUsed,V.BackpackSlots));
    SetText(TEXT("HomeStorage"),FString::Printf(TEXT("仓库 %d / %d 格\n背包 %d / %d 格"),V.WarehouseUsed,V.WarehouseSlots,V.BackpackUsed,V.BackpackSlots));
    SetText(TEXT("HomeEquipment"),FString::Printf(TEXT("M4  /  M9\n主武器配件 %d 件"),V.Loadout.Primary.AttachmentIds.Num()));
    const auto* Settings=UFPSSystemSettings::Get(this);const FString BagKey=Settings?Settings->BindingText("Inventory"):TEXT("Tab");
    SetText(TEXT("RaidCapacity"),FString::Printf(TEXT("背包  %d / %d  ·  %s"),V.BackpackUsed,V.BackpackSlots,*BagKey));
    SetText(TEXT("CloseBagLabel"),TEXT("关闭背包 / ")+BagKey);
    SetText(TEXT("Interaction"),V.Interaction);SetShown(TEXT("InteractionPanel"),Raid&&!bBagOpen&&!V.Interaction.IsEmpty());
    SetText(TEXT("ExtractionStatus"),V.ExtractionRemaining>=0?FString::Printf(TEXT("撤离中  %.1f 秒"),V.ExtractionRemaining):FString::Printf(TEXT("撤离点  %.0f 米"),FMath::Max(0.f,V.ExtractionDistance)));
    SetText(TEXT("WeaponSummary"),FString::Printf(TEXT("M4\n配件  %d 件"),V.Loadout.Primary.AttachmentIds.Num()));
    SetText(TEXT("AmmoSummary"),FString::Printf(TEXT("弹药  %d / %d"),V.Loadout.Primary.Magazine,V.Loadout.Primary.Reserve));
    const TCHAR* Titles[]={TEXT("个人工厂总览"),TEXT("仓库与整备"),TEXT("加工制造"),TEXT("枪械改装")};
    const TCHAR* Hints[]={TEXT("仅安全带回的搜获进入仓库；容量按物品堆叠计算。"),TEXT("按件出售仓库物资；本次搜获须撤离后才能出售。"),TEXT("单一生产槽。材料与资金由仓库扣除，完成后领取产物。"),TEXT("使用仓库中的配件改装 M4；卸下的配件退回仓库。")};
    SetText(TEXT("SectionTitle"),Titles[Tab]);SetText(TEXT("SectionHint"),Hints[Tab]);
    const FName Tabs[]={TEXT("Overview"),TEXT("Warehouse"),TEXT("Production"),TEXT("Modification")};
    for(int32 I=0;I<4;++I)if(auto* B=ExtractionUI::Find<UFPSExtractionButton>(this,Tabs[I]))if(Theme)
    {B->SetStyle(I==Tab?Theme->LightButton:Theme->DarkButton);if(auto* T=ExtractionUI::Find<UTextBlock>(this,FName(*(Tabs[I].ToString()+TEXT("Label")))))T->SetColorAndOpacity(I==Tab?Theme->Ink:Theme->Paper);}
    FString Production=TEXT("生产槽空闲");
    if(!V.ProductionRecipe.IsNone())
    {const auto* R=S->FindRecipe(V.ProductionRecipe);Production=(R?R->Name.ToString():TEXT("生产任务"))+(V.bProductionReady?TEXT(" · 待领取"):FString::Printf(TEXT(" · %.0f 秒"),FMath::CeilToFloat(V.ProductionRemaining)));}
    SetText(TEXT("HomeProduction"),Production);SetText(TEXT("ProductionStatus"),Production);SetEnabled(TEXT("Claim"),Base&&V.bProductionReady&&V.bStorageHealthy);
    SetText(TEXT("LobbyProductionStatus"),Production);
    SetEnabled(TEXT("Depart"),Base&&V.bStorageHealthy);SetEnabled(TEXT("Resupply"),Base&&V.bStorageHealthy);
    SetText(TEXT("ResultTitle"),V.bLastSuccess?TEXT("撤离成功"):TEXT("行动失败"));
    SetText(TEXT("ResultNote"),V.bLastSuccess?TEXT("当前背包全部入库。出征枪械与已装配件保留。"):TEXT("本次搜获遗失；出征前整备背包已恢复。枪械与已装配件保留，无维修费。"));
    SetText(TEXT("ResultListTitle"),V.bLastSuccess?TEXT("带回物资"):TEXT("遗失物资"));
    if(auto* T=ExtractionUI::Find<UTextBlock>(this,TEXT("ResultTitle")))T->SetColorAndOpacity(V.bLastSuccess?FLinearColor(.94f,.68f,.045f):FLinearColor(.82f,.22f,.18f));
    const auto Items=S->GetItems();
    for(const TCHAR* Name:{TEXT("WarehouseGrid"),TEXT("BaseBackpackGrid"),TEXT("RaidBackpackGrid")})
        if(auto* Grid=ExtractionUI::Find<UFPSExtractionGridUI>(this,Name))Grid->Present(this,V,Items);
    const auto& Inventory=SelectedContainer==EFPSExtractionContainer::Warehouse?V.Warehouse:V.Backpack;
    const auto* Stack=Inventory.FindByPredicate([this](const auto& I){return I.StackId==SelectedStack;});
    const auto* Item=Stack?S->FindItem(Stack->ItemId):nullptr;
    FString Selection=TEXT("选择格中物资，查看详情与操作");
    if(Item)Selection=FString::Printf(TEXT("%s ×%d  ·  %d×%d 格\n%s"),*Item->Name.ToString(),Stack->Quantity,Stack->bRotated?Item->Height:Item->Width,Stack->bRotated?Item->Width:Item->Height,*Item->Description.ToString());
    else if(!ActiveDrag)SelectedStack.Invalidate();
    SetText(TEXT("SelectedInfo"),Selection);SetText(TEXT("RaidSelectedInfo"),Selection);
    const bool Selected=Item&&V.bStorageHealthy;
    SetEnabled(TEXT("SelectedTransfer"),Base&&Selected);SetEnabled(TEXT("SelectedSell"),Base&&Selected&&SelectedContainer==EFPSExtractionContainer::Warehouse);
    SetEnabled(TEXT("SelectedRotate"),Selected&&(Base||SelectedContainer==EFPSExtractionContainer::Backpack));
    SetEnabled(TEXT("RaidRotate"),Selected&&SelectedContainer==EFPSExtractionContainer::Backpack&&!bConfirmAbort);
    SetEnabled(TEXT("SelectedDrop"),Raid&&Selected&&SelectedContainer==EFPSExtractionContainer::Backpack&&!bConfirmAbort);
    if(Stack&&Item)SetText(TEXT("SelectedSellLabel"),FString::Printf(TEXT("出售整堆 · %d"),Item->SellPrice*Stack->Quantity));
    else SetText(TEXT("SelectedSellLabel"),TEXT("出售整堆"));
    SetText(TEXT("SelectedTransferLabel"),SelectedContainer==EFPSExtractionContainer::Warehouse?TEXT("放入背包"):TEXT("放回仓库"));
    const FString Signature=RowSignature(V);if(!ActiveDrag&&Signature!=LastRows){if(Base&&Tab==3&&bWeaponEditing&&bAttachmentDrawer)RebuildAttachmentRows(V);else if(Result||(Base&&Tab==2))RebuildRows(V);LastRows=Signature;}
}
void UFPSExtractionUI::RebuildRows(const FFPSExtractionView& V)
{
    auto* S=Model();if(!S)return;
    const FName ListName=V.Phase==EFPSExtractionPhase::Base?TEXT("BaseRows"):V.Phase==EFPSExtractionPhase::Raid?TEXT("BagRows"):TEXT("ResultRows");
    auto* List=ExtractionUI::Find<UScrollBox>(this,ListName);if(!List)return;
    const float SavedOffset=List->GetScrollOffset();List->ClearChildren();
    auto* RowClass=LoadClass<UUserWidget>(nullptr,*ExtractionUI::AssetClass(TEXT("WBP_ExtractionRow")));if(!RowClass)return;
    auto Row=[&](const FString& Title,const FString& Detail,const FString& Count,FName Action,FName Item,const FString& Label,bool Enabled,const FString& Reason)
    {
        auto* W=CreateWidget<UUserWidget>(GetOwningPlayer(),RowClass);if(!W)return;
        ExtractionUI::Text(W,TEXT("RowTitle"),Title);ExtractionUI::Text(W,TEXT("RowDetail"),Detail);
        ExtractionUI::Text(W,TEXT("RowCount"),Count);ExtractionUI::Text(W,TEXT("RowActionLabel"),Label);
        ExtractionUI::Text(W,TEXT("RowReason"),Reason);
        if(auto* B=ExtractionUI::Find<UFPSExtractionButton>(W,TEXT("RowAction")))
        {B->Action=Action;B->ItemId=Item;B->Quantity=1;B->Bind(this);B->SetIsEnabled(Enabled);B->SetVisibility(Action.IsNone()?ESlateVisibility::Collapsed:ESlateVisibility::Visible);}
        auto* Slot=Cast<UScrollBoxSlot>(List->AddChild(W));if(Slot)Slot->SetPadding(FMargin(0,0,0,10));
    };
    if(V.Phase==EFPSExtractionPhase::Base&&Tab==2)
    {
        for(const auto& R:S->GetRecipes())
        {
            bool Can=V.bStorageHealthy&&V.ProductionRecipe.IsNone()&&V.Credits>=R.CreditCost;
            FString Inputs;for(const auto& I:R.Inputs){Inputs+=FString::Printf(TEXT("%s ×%d  "),*S->ItemName(I.ItemId).ToString(),I.Quantity);Can&=S->Count(V.Warehouse,I.ItemId)>=I.Quantity;}
            FString Reason=!V.ProductionRecipe.IsNone()?TEXT("生产槽占用"):!V.bStorageHealthy?TEXT("存档不可用"):!Can?TEXT("材料或资金不足"):TEXT("");
            Row(R.Name.ToString(),Inputs+FString::Printf(TEXT("\n资金 %d  ·  %.0f 秒  →  %s ×%d"),R.CreditCost,R.Duration,*S->ItemName(R.Output.ItemId).ToString(),R.Output.Quantity),TEXT("配方"),TEXT("Craft"),R.RecipeId,TEXT("开始制作"),Can,Reason);
        }
    }
    else
    {
        if(V.Phase==EFPSExtractionPhase::Base&&Tab==3)
            for(const FName Id:V.Loadout.Primary.AttachmentIds)
            {
                FString Name=TEXT("已装配件");for(const auto& Item:S->GetItems())if(Item.AttachmentId==Id){Name=Item.Name.ToString();break;}
                Row(Name,TEXT("当前装在 M4 上；卸下后退回仓库。"),TEXT("已装"),TEXT("Remove"),Id,TEXT("卸下配件"),V.bStorageHealthy,V.bStorageHealthy?TEXT(""):TEXT("存档不可用"));
            }
        const auto& Inventory=V.Phase==EFPSExtractionPhase::Base?V.Warehouse:V.Phase==EFPSExtractionPhase::Raid?V.Backpack:V.LastHaul;
        for(const auto& Stack:Inventory)
        {
            const auto* Item=S->FindItem(Stack.ItemId);if(!Item)continue;
            if(V.Phase==EFPSExtractionPhase::Base&&Tab==3&&Item->AttachmentId.IsNone())continue;
            FName Action;FString Label,Detail=Item->Description.ToString();
            if(V.Phase==EFPSExtractionPhase::Raid){Action=TEXT("Drop");Label=TEXT("丢弃 1 件");}
            else if(V.Phase==EFPSExtractionPhase::Base&&Tab==1){Action=TEXT("Sell");Label=TEXT("出售 1 件");Detail+=FString::Printf(TEXT("\n单价 %d  ·  库存价值 %d"),Item->SellPrice,Item->SellPrice*Stack.Quantity);}
            else if(V.Phase==EFPSExtractionPhase::Base&&Tab==3){Action=TEXT("Install");Label=TEXT("安装配件");}
            Row(Item->Name.ToString(),Detail,FString::Printf(TEXT("× %d"),Stack.Quantity),Action,Stack.ItemId,Label,V.bStorageHealthy,V.bStorageHealthy?TEXT(""):TEXT("存档不可用"));
        }
    }
    if(List->GetChildrenCount()==0)Row(TEXT("暂无物资"),V.Phase==EFPSExtractionPhase::Base?TEXT("出征搜集物资并成功撤离后，在这里管理与加工。"):TEXT("本次行动尚无记录物资。"),TEXT("—"),NAME_None,NAME_None,TEXT(""),false,TEXT(""));
    List->SetScrollOffset(SavedOffset);
}

#if WITH_EDITOR
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
namespace ExtractionUI
{
struct FLayout
{
    UWidgetBlueprint* BP=nullptr;UWidgetTree* Tree=nullptr;UCanvasPanel* Canvas=nullptr;
    UFPSUITheme* Theme=nullptr;UFont* Font=nullptr;UFont* NumberFont=nullptr;
    // v3 appends its own panel; the existing v2 tree and Designer layout remain intact.
    explicit FLayout(UWidgetBlueprint* Existing)
    {
        BP=Existing;Tree=BP?BP->WidgetTree:nullptr;
        Canvas=Tree?Cast<UCanvasPanel>(Tree->FindWidget(TEXT("Layout"))):nullptr;
        if(BP&&BP->GeneratedClass)if(auto* UI=Cast<UFPSExtractionUI>(BP->GeneratedClass->GetDefaultObject()))Theme=UI->Theme;
        if(!Theme)Theme=LoadObject<UFPSUITheme>(nullptr,TEXT("/Game/FPSDemo/Art/Industrial/UI/DA_FPSUITheme.DA_FPSUITheme"));
        Font=LoadObject<UFont>(nullptr,TEXT("/Game/FPSDemo/Art/Industrial/UI/Fonts/F_UI_Text.F_UI_Text"));
        NumberFont=LoadObject<UFont>(nullptr,TEXT("/Game/FPSDemo/Art/Industrial/UI/Fonts/F_UI_Number.F_UI_Number"));
    }
    FLayout(const TCHAR* Name,bool Row,bool Replace=false)
    {
        Theme=LoadObject<UFPSUITheme>(nullptr,TEXT("/Game/FPSDemo/Art/Industrial/UI/DA_FPSUITheme.DA_FPSUITheme"));
        Font=LoadObject<UFont>(nullptr,TEXT("/Game/FPSDemo/Art/Industrial/UI/Fonts/F_UI_Text.F_UI_Text"));
        NumberFont=LoadObject<UFont>(nullptr,TEXT("/Game/FPSDemo/Art/Industrial/UI/Fonts/F_UI_Number.F_UI_Number"));
        if(!Theme||!Font||!NumberFont)return;
        const FString Path=FString(Root)+Name;
        auto* Factory=NewObject<UWidgetBlueprintFactory>();Factory->ParentClass=Row?UUserWidget::StaticClass():UFPSExtractionUI::StaticClass();
        BP=Replace?LoadObject<UWidgetBlueprint>(nullptr,*(Path+TEXT(".")+Name)):nullptr;
        if(BP)
        {
            BP->Modify();
            if(BP->WidgetTree)BP->WidgetTree->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
            BP->WidgetTree=NewObject<UWidgetTree>(BP,TEXT("WidgetTree"),RF_Transactional);
        }
        else BP=Cast<UWidgetBlueprint>(Factory->FactoryCreateNew(UWidgetBlueprint::StaticClass(),CreatePackage(*Path),Name,RF_Public|RF_Standalone,nullptr,GWarn));
        if(!BP)return;
        Tree=BP->WidgetTree;
        if(!Tree){Tree=NewObject<UWidgetTree>(BP,TEXT("WidgetTree"),RF_Transactional);BP->WidgetTree=Tree;}
        auto* Size=Make<USizeBox>(TEXT("DesignSize"));Size->SetWidthOverride(Row?980:1600);Size->SetHeightOverride(Row?126:900);
        Canvas=Make<UCanvasPanel>(TEXT("Layout"));Size->AddChild(Canvas);
        if(Row)Tree->RootWidget=Size;
        else{auto* Scale=Make<UScaleBox>(TEXT("ViewportFit"));Scale->SetStretch(EStretch::ScaleToFit);Scale->AddChild(Size);Tree->RootWidget=Scale;}
        FAssetRegistryModule::AssetCreated(BP);
    }
    template<class T>T* Make(const TCHAR* Name){return Tree->ConstructWidget<T>(T::StaticClass(),Name);}
    void Place(UCanvasPanel* P,UWidget* W,float X,float Y,float Width,float Height)
    {auto* Slot=P->AddChildToCanvas(W);Slot->SetPosition({X,Y});Slot->SetSize({Width,Height});}
    UCanvasPanel* Group(const TCHAR* Name,float X,float Y,float W,float H,UCanvasPanel* Parent=nullptr)
    {auto* C=Make<UCanvasPanel>(Name);C->SetVisibility(ESlateVisibility::SelfHitTestInvisible);Place(Parent?Parent:Canvas,C,X,Y,W,H);return C;}
    UImage* Surface(const TCHAR* Name,float X,float Y,float W,float H,FLinearColor Color,UCanvasPanel* P=nullptr,bool Rounded=false)
    {auto* I=Make<UImage>(Name);if(Rounded)I->SetBrush(FSlateRoundedBoxBrush(Color,5.f));else I->SetBrush(FSlateColorBrush(Color));I->SetVisibility(ESlateVisibility::HitTestInvisible);Place(P?P:Canvas,I,X,Y,W,H);return I;}
    UTextBlock* Label(const TCHAR* Name,const FString& Text,float X,float Y,float W,float H,int32 Size,UCanvasPanel* P=nullptr,bool Bold=false,FLinearColor Color=FLinearColor(.76f,.78f,.74f,1))
    {auto* T=Make<UTextBlock>(Name);T->SetText(FText::FromString(Text));const bool Numeric=FName(Name)==TEXT("Credits");T->SetFont(FSlateFontInfo(Numeric?NumberFont:Font,Size,Bold?FName("Bold"):FName("Regular")));T->SetColorAndOpacity(Color);T->SetAutoWrapText(true);T->SetVisibility(ESlateVisibility::HitTestInvisible);Place(P?P:Canvas,T,X,Y,W,H);return T;}
    UFPSExtractionButton* Button(const TCHAR* Name,const FString& Text,float X,float Y,float W,float H,UCanvasPanel* P=nullptr,bool Accent=false)
    {
        auto* B=Make<UFPSExtractionButton>(Name);B->Action=Name;B->SetStyle(Accent?Theme->AccentButton:Theme->DarkButton);
        auto* Label=Make<UTextBlock>(*(FString(Name)+TEXT("Label")));Label->SetText(FText::FromString(Text));Label->SetFont(FSlateFontInfo(Font,18,TEXT("Bold")));Label->SetColorAndOpacity(Accent?Theme->Ink:Theme->Paper);Label->SetJustification(ETextJustify::Center);Label->SetVisibility(ESlateVisibility::HitTestInvisible);B->AddChild(Label);
        auto* Slot=CastChecked<UButtonSlot>(Label->Slot);Slot->SetHorizontalAlignment(HAlign_Center);Slot->SetVerticalAlignment(VAlign_Center);Slot->SetPadding(FMargin(12,0,12,4));
        Place(P?P:Canvas,B,X,Y,W,H);return B;
    }
    void Rows(const TCHAR* Name,float X,float Y,float W,float H,UCanvasPanel* P)
    {auto* S=Make<UScrollBox>(Name);S->SetScrollBarVisibility(ESlateVisibility::Visible);Place(P,S,X,Y,W,H);}
    bool Save()
    {
        TSet<FName> Names;Tree->ForEachWidget([&](UWidget* W){Names.Add(W->GetFName());});
        for(auto It=BP->WidgetVariableNameToGuidMap.CreateIterator();It;++It)if(!Names.Contains(It.Key()))It.RemoveCurrent();
        Tree->ForEachWidget([&](UWidget* W){if(!BP->WidgetVariableNameToGuidMap.Contains(W->GetFName()))BP->OnVariableAdded(W->GetFName());});
        FKismetEditorUtilities::CompileBlueprint(BP);if(BP->Status==BS_Error||!BP->GeneratedClass)return false;
        if(auto* UI=Cast<UFPSExtractionUI>(BP->GeneratedClass->GetDefaultObject()))
        {UI->Theme=Theme;UI->LobbyPresentation=LoadObject<UFPSExtractionLobbyPresentation>(nullptr,TEXT("/Game/FPSDemo/Art/UI/ExtractionTerminal/DA_LobbyPresentation.DA_LobbyPresentation"));}
        BP->MarkPackageDirty();FSavePackageArgs A;A.TopLevelFlags=RF_Public|RF_Standalone;A.SaveFlags=SAVE_NoError;
        return UPackage::SavePackage(BP->GetOutermost(),BP,*FPackageName::LongPackageNameToFilename(BP->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),A);
    }
};
void AddWeaponWorkbench(FLayout& P,UCanvasPanel* Base)
{
    const auto Dark=FLinearColor(.013f,.019f,.023f,1),Panel=FLinearColor(.021f,.028f,.032f,1),Muted=FLinearColor(.39f,.46f,.46f,1);
    P.Label(TEXT("WeaponWorkbenchLayoutVersion"),TEXT("3"),0,0,1,1,1)->SetVisibility(ESlateVisibility::Collapsed);
    auto* Work=P.Group(TEXT("ModificationPanel"),0,0,1600,900,Base);
    P.Surface(TEXT("WeaponWorkbenchBG"),286,177,1274,603,Panel,Work,true);
    P.Label(TEXT("WeaponWorkbenchTitle"),TEXT("武器工作台"),308,190,740,48,29,Work,true);
    P.Label(TEXT("WeaponWorkbenchHelp"),TEXT("右键拖动旋转 / 滚轮缩放"),308,240,730,28,15,Work,false,Muted);
    auto* View=P.Make<UFPSExtractionWeaponViewport>(TEXT("WeaponViewport"));
    View->SetVisibility(ESlateVisibility::Visible);P.Place(Work,View,308,275,862,355);
    P.Surface(TEXT("WeaponStatsDivider"),308,639,862,1,Muted,Work);
    P.Label(TEXT("CurrentStatsTitle"),TEXT("当前装配"),308,646,380,28,17,Work,true);
    P.Label(TEXT("CurrentWeaponStats"),TEXT(""),308,677,400,100,16,Work)->SetAutoWrapText(false);
    P.Label(TEXT("CandidateStatsTitle"),TEXT("候选差异"),728,646,442,28,16,Work,true,P.Theme->Accent)->SetAutoWrapText(false);
    P.Label(TEXT("CandidateWeaponStats"),TEXT(""),728,677,442,100,16,Work)->SetAutoWrapText(false);
    P.Surface(TEXT("AttachmentColumnBG"),1186,193,358,570,Dark,Work,true);
    P.Label(TEXT("AttachmentColumnTitle"),TEXT("配件装配"),1200,201,330,35,23,Work,true);
    P.Button(TEXT("SlotOptic"),TEXT("瞄具"),1198,249,104,44,Work,true);
    P.Button(TEXT("SlotMuzzle"),TEXT("枪口"),1310,249,104,44,Work);
    P.Button(TEXT("SlotMagazine"),TEXT("弹匣"),1422,249,104,44,Work);
    P.Label(TEXT("AttachmentCategory"),TEXT("瞄具 / 兼容库存"),1200,307,326,32,17,Work);
    P.Rows(TEXT("AttachmentRows"),1198,347,336,405,Work);
    Work->SetVisibility(ESlateVisibility::Collapsed);
}
bool EnsureAttachmentRow()
{
    if(LoadObject<UWidgetBlueprint>(nullptr,*(FString(Root)+TEXT("WBP_ExtractionAttachmentRow.WBP_ExtractionAttachmentRow"))))return true;
    FLayout P(TEXT("WBP_ExtractionAttachmentRow"),true);if(!P.BP)return false;
    auto* Size=CastChecked<USizeBox>(P.Tree->RootWidget);Size->SetWidthOverride(320);Size->SetHeightOverride(206);
    P.Surface(TEXT("RowBackground"),0,0,320,206,FLinearColor(.027f,.034f,.038f,1),nullptr,true);
    P.Surface(TEXT("RowLine"),0,0,3,206,FLinearColor(.19f,.23f,.24f,1));
    P.Label(TEXT("RowTitle"),TEXT("配件名称"),14,9,292,56,18,nullptr,true);
    P.Label(TEXT("RowDetail"),TEXT("配件说明"),14,69,292,60,14);
    P.Label(TEXT("RowCount"),TEXT("仓库 ×0"),14,142,165,31,16);
    P.Button(TEXT("RowAction"),TEXT("安装"),196,134,110,44);
    P.Label(TEXT("RowReason"),TEXT(""),14,180,292,24,13,nullptr,false,P.Theme->Accent);
    return P.Save();
}
void AddReferenceWorkbench(FLayout& P)
{
    const auto Dark=FLinearColor(.010f,.016f,.020f,1),Panel=FLinearColor(.022f,.031f,.036f,1),Muted=FLinearColor(.47f,.53f,.54f,1),Line=FLinearColor(.16f,.22f,.24f,1);
    P.Label(TEXT("WeaponWorkbenchLayoutVersion"),TEXT("4"),0,0,1,1,1)->SetVisibility(ESlateVisibility::Collapsed);
    // Root sibling: hiding the factory frame cannot hide the weapon workbench itself.
    auto* Work=P.Group(TEXT("ModificationPanel"),0,0,1600,900);
    P.Surface(TEXT("WeaponWorkbenchBG"),0,0,1600,900,Dark,Work);
    auto* View=P.Make<UFPSExtractionWeaponViewport>(TEXT("WeaponViewport"));View->SetVisibility(ESlateVisibility::Visible);
    P.Place(Work,View,40,150,1520,650);
    P.Surface(TEXT("WorkbenchTopRule"),40,94,1520,1,Line,Work);
    P.Button(TEXT("WorkbenchBack"),TEXT("返回工厂 / Esc"),40,32,220,48,Work);
    P.Button(TEXT("WorkbenchOverview"),TEXT("武器概览"),280,32,180,48,Work);
    P.Label(TEXT("WorkbenchMode"),TEXT("持有武器 / 概览"),486,38,600,38,23,Work,true);
    P.Surface(TEXT("WorkbenchModeAccent"),486,83,144,3,P.Theme->Accent,Work);
    P.Label(TEXT("WorkbenchWeaponTitle"),TEXT("M4"),480,106,920,43,30,Work,true);
    P.Label(TEXT("WorkbenchBrand"),TEXT("WEAPON WORKBENCH"),1190,44,366,30,16,Work,false,Muted)->SetJustification(ETextJustify::Right);
    auto* Overview=P.Group(TEXT("WeaponOverviewPanel"),0,0,1600,900,Work);
    P.Surface(TEXT("WeaponOverviewBG"),40,112,390,680,Panel,Overview,true);
    P.Label(TEXT("OwnedWeaponsTitle"),TEXT("持有武器"),58,123,348,34,20,Overview,true);
    const TCHAR* Prefixes[]={TEXT("Primary"),TEXT("Secondary")};
    const TCHAR* Actions[]={TEXT("ViewPrimaryWeapon"),TEXT("ViewSecondaryWeapon")};
    const TCHAR* Ids[]={TEXT("M4"),TEXT("M9")};
    for(int32 I=0;I<2;++I)
    {
        const float Y=166+I*126;const FString Prefix=Prefixes[I];
        P.Button(Actions[I],TEXT(""),52,Y,366,112,Overview);
        P.Label(*(Prefix+TEXT("WeaponName")),Ids[I],68,Y+7,160,37,25,Overview,true);
        P.Label(*(Prefix+TEXT("WeaponRole")),I==0?TEXT("主武器 · 已持有"):TEXT("副武器 · 已持有"),220,Y+13,182,30,16,Overview);
        auto* Icon=P.Make<UImage>(*(Prefix+TEXT("WeaponImage")));
        const FString Texture=FString::Printf(TEXT("/Game/FPSDemo/Art/Industrial/UI/T_UI_%s.T_UI_%s"),Ids[I],Ids[I]);
        Icon->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr,*Texture),true);Icon->SetColorAndOpacity(FLinearColor::White);Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
        P.Place(Overview,Icon,116,Y+40,234,68);

    }
    P.Surface(TEXT("OverviewStatsRule"),58,429,348,1,Line,Overview);
    P.Label(TEXT("OverviewWeaponName"),TEXT("M4"),58,443,348,42,26,Overview,true);
    P.Label(TEXT("OverviewStatsLabel"),TEXT("基础伤害\n最大射程\n射速\n弹匣容量\n换弹时间\n开镜时间\n开火模式\n后坐力倍率"),58,501,144,250,19,Overview,false,Muted)->SetAutoWrapText(false);
    P.Label(TEXT("OverviewWeaponStats"),TEXT(""),198,501,208,250,19,Overview)->SetAutoWrapText(false);
    CastChecked<UTextBlock>(P.Tree->FindWidget(TEXT("OverviewWeaponStats")))->SetJustification(ETextJustify::Right);
    P.Label(TEXT("WeaponInstanceNote"),TEXT(""),480,752,1040,40,18,Overview,false,Muted)->SetAutoWrapText(false);
    auto* Enter=P.Group(TEXT("WeaponEnterPanel"),0,0,1600,900,Work);
    P.Label(TEXT("WeaponModificationNote"),TEXT(""),480,787,1040,28,16,Enter,false,Muted);
    P.Button(TEXT("EnterWeaponModification"),TEXT("进入改装  →"),1320,824,240,52,Enter,true);
    auto* Drawer=P.Group(TEXT("AttachmentDrawer"),1210,150,350,650,Work);
    P.Surface(TEXT("AttachmentDrawerBG"),0,0,350,650,Panel,Drawer,true)->SetVisibility(ESlateVisibility::Visible);
    P.Surface(TEXT("AttachmentDrawerAccent"),0,0,3,650,P.Theme->Accent,Drawer);
    P.Label(TEXT("AttachmentCategory"),TEXT("瞄具 / 兼容库存"),16,14,239,36,19,Drawer,true);
    P.Button(TEXT("CloseAttachmentDrawer"),TEXT("收起"),264,10,70,40,Drawer);
    P.Button(TEXT("SlotOptic"),TEXT("瞄具"),14,67,102,43,Drawer);
    P.Button(TEXT("SlotMuzzle"),TEXT("枪口"),124,67,102,43,Drawer);
    P.Button(TEXT("SlotMagazine"),TEXT("弹匣"),234,67,102,43,Drawer);
    // Categories are selected on the six model cards; retain legacy names for saved UI compatibility.
    for(const TCHAR* Name:{TEXT("SlotOptic"),TEXT("SlotMuzzle"),TEXT("SlotMagazine")})P.Tree->FindWidget(Name)->SetVisibility(ESlateVisibility::Collapsed);
    P.Rows(TEXT("AttachmentRows"),8,70,334,204,Drawer);
    P.Surface(TEXT("DrawerStatsRule"),16,286,318,1,Line,Drawer);
    P.Label(TEXT("CurrentStatsTitle"),TEXT("当前参数"),16,298,318,27,17,Drawer,true);
    P.Label(TEXT("CurrentWeaponStats"),TEXT(""),16,328,318,146,16,Drawer)->SetAutoWrapText(false);
    P.Label(TEXT("CandidateStatsTitle"),TEXT("候选差异"),16,482,318,27,16,Drawer,true,P.Theme->Accent)->SetAutoWrapText(false);
    P.Label(TEXT("CandidateWeaponStats"),TEXT(""),16,514,318,130,15,Drawer)->SetAutoWrapText(false);
    Drawer->SetVisibility(ESlateVisibility::Collapsed);
    P.Label(TEXT("WorkbenchHelp"),TEXT("右键拖动旋转 / 滚轮缩放"),40,815,950,30,17,Work,false,Muted);
    P.Label(TEXT("WorkbenchFeedback"),TEXT(""),40,850,1070,40,16,Work,false,P.Theme->Accent);
    P.Button(TEXT("ResetWorkbenchView"),TEXT("重置视角"),1120,824,180,52,Work);
    Work->SetVisibility(ESlateVisibility::Collapsed);
}
void AddLobby(FLayout& P)
{
    const auto Paper=P.Theme->Paper,Yellow=P.Theme->Accent,Ink=P.Theme->Ink;
    const auto Muted=FLinearColor(.47f,.52f,.53f,1),Line=FLinearColor(.30f,.36f,.37f,.35f),Panel=FLinearColor(.012f,.019f,.022f,.9f);
    P.Label(TEXT("LobbyLayoutVersion"),TEXT("1"),0,0,1,1,1)->SetVisibility(ESlateVisibility::Collapsed);
    auto* Home=P.Group(TEXT("OverviewPanel"),0,0,1600,900);
    auto Image=[&](const TCHAR* Name,const TCHAR* Asset,float X,float Y,float W,float H)
    {auto* I=P.Make<UImage>(Name);I->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr,*(FString(Root)+TEXT("Textures/")+Asset+TEXT(".")+Asset)));I->SetVisibility(ESlateVisibility::HitTestInvisible);P.Place(Home,I,X,Y,W,H);return I;};
    Image(TEXT("LobbyVignette"),TEXT("T_LobbyVignette"),0,0,1600,900);
    P.Surface(TEXT("LobbyHeader"),0,0,1600,108,FLinearColor(.009f,.014f,.017f,.78f),Home);
    P.Surface(TEXT("LobbyTopRule"),40,27,1520,1,Line,Home);
    P.Surface(TEXT("LobbyBottomRule"),40,105,1520,1,Line,Home);
    P.Label(TEXT("LobbyBrand"),TEXT("FOUNDRY"),40,41,186,40,27,Home,true,Paper)->SetAutoWrapText(false);
    P.Label(TEXT("LobbyBrandSub"),TEXT("个人工厂"),42,79,160,20,11,Home,false,Muted);
    const TCHAR* Names[]={TEXT("LobbyOverview"),TEXT("LobbyWarehouse"),TEXT("LobbyFactory"),TEXT("LobbyModification")};
    const TCHAR* Labels[]={TEXT("出征"),TEXT("仓库"),TEXT("工厂"),TEXT("改枪台")};
    const TCHAR* Actions[]={TEXT("Overview"),TEXT("Warehouse"),TEXT("LobbyFactory"),TEXT("Modification")};
    for(int32 I=0;I<4;++I)
    {
        auto* B=P.Button(Names[I],Labels[I],250+I*142,44,134,54,Home,I==0);B->Action=Actions[I];
        FButtonStyle Style=B->GetStyle();Style.Normal=FSlateColorBrush(I==0?FLinearColor(.10f,.12f,.11f,.9f):FLinearColor::Transparent);Style.Hovered=FSlateColorBrush(FLinearColor(.16f,.19f,.19f,.9f));Style.Pressed=FSlateColorBrush(FLinearColor(.23f,.23f,.17f,1));B->SetStyle(Style);
        auto* Label=Cast<UTextBlock>(B->GetChildAt(0));Label->SetColorAndOpacity(I==0?Yellow:Paper);
    }
    P.Surface(TEXT("LobbyNavActive"),250,102,134,3,Yellow,Home);
    P.Label(TEXT("LobbyFundsLabel"),TEXT("可用资金"),1138,55,96,28,14,Home,false,Muted)->SetAutoWrapText(false);
    auto* Credits=P.Label(TEXT("LobbyCredits"),TEXT("0"),1232,44,178,44,28,Home,true,Paper);
    Credits->SetFont(FSlateFontInfo(P.NumberFont,28,TEXT("Bold")));Credits->SetJustification(ETextJustify::Right);Credits->SetAutoWrapText(false);
    P.Surface(TEXT("LobbyFundsRule"),1430,51,1,35,Line,Home);
    P.Button(TEXT("LobbySettings"),TEXT("设置"),1450,44,110,54,Home);
    P.Label(TEXT("LobbyModeTag"),TEXT("单人  /  PVE"),43,143,300,25,14,Home,false,Yellow);
    P.Label(TEXT("LobbyTitle"),TEXT("回收行动"),40,178,390,68,42,Home,true,Paper);
    P.Surface(TEXT("LobbyTitleRule"),42,264,76,3,Yellow,Home);
    P.Label(TEXT("LobbySubtitle"),TEXT("整备、搜寻，安全带回。"),42,287,400,35,17,Home,false,Muted);
    P.Label(TEXT("LobbyRobotLabel"),TEXT("驻地机器人"),42,675,390,37,24,Home,true,Paper);
    P.Label(TEXT("LobbyRobotCode"),TEXT("FACTORY / READY"),43,718,390,25,13,Home,false,Muted);
    P.Surface(TEXT("LobbyProductionRule"),42,754,310,1,Line,Home);
    P.Label(TEXT("LobbyProductionStatus"),TEXT("生产槽空闲"),43,768,300,30,15,Home,false,Muted);
    P.Button(TEXT("LobbyProduction"),TEXT("查看加工  →"),42,808,230,42,Home)->Action=TEXT("Production");
    // The one supported destination is shown honestly; no placeholder map selector.
    P.Surface(TEXT("LobbyMissionBG"),1172,524,388,288,Panel,Home,true);
    Image(TEXT("LobbyMapImage"),TEXT("T_LobbyIndustrialCrossing"),1180,532,372,154);
    P.Surface(TEXT("LobbyMapTagBG"),1192,544,130,27,Panel,Home);
    P.Label(TEXT("LobbyMapTag"),TEXT("行动区域 / 01"),1202,545,140,25,12,Home,false,Paper);
    P.Label(TEXT("LobbyMapName"),TEXT("工业关隘"),1192,700,340,43,28,Home,true,Paper);
    P.Label(TEXT("LobbyMapMode"),TEXT("单人回收  ·  搜索物资并撤离"),1194,751,340,30,14,Home,false,Muted);
    P.Label(TEXT("LobbyLoadout"),TEXT(""),1174,483,386,28,13,Home,false,Paper)->SetAutoWrapText(false);
    P.Button(TEXT("LobbyPrepare"),TEXT("整备"),1172,826,152,52,Home)->Action=TEXT("Warehouse");
    P.Button(TEXT("LobbyDepart"),TEXT("出发  →"),1336,826,224,52,Home,true)->Action=TEXT("Depart");
    P.Label(TEXT("LobbyFeedback"),TEXT(""),400,812,714,65,15,Home,false,Yellow);
    P.Label(TEXT("LobbySceneNote"),TEXT("PERSONAL BASE / FOUNDRY"),1150,152,410,24,11,Home,false,Muted)->SetJustification(ETextJustify::Right);
}
bool BuildInventoryTerminal(bool Replace)
{
    FLayout P(TEXT("WBP_ExtractionTerminal"),false,Replace);if(!P.BP)return false;
    const auto Dark=FLinearColor(.013f,.019f,.023f,1),Panel=FLinearColor(.021f,.028f,.032f,.98f),Muted=FLinearColor(.39f,.46f,.46f,1),Yellow=P.Theme->Accent;
    P.Label(TEXT("InventoryLayoutVersion"),TEXT("2"),0,0,1,1,1)->SetVisibility(ESlateVisibility::Collapsed);
    P.Surface(TEXT("FullBackdrop"),0,0,1600,900,Dark);
    auto* Base=P.Group(TEXT("BasePanel"),0,0,1600,900);
    for(int32 I=0;I<12;++I)P.Surface(*FString::Printf(TEXT("GridV%d"),I),40+I*138,0,1,900,FLinearColor(.09f,.12f,.13f,.28f),Base);
    P.Surface(TEXT("TopLine"),40,43,1520,1,Muted,Base);P.Surface(TEXT("TitleAccent"),40,70,5,53,Yellow,Base);
    P.Label(TEXT("BaseTitle"),TEXT("个人工厂"),60,63,700,64,38,Base,true);
    P.Label(TEXT("BaseSubtitle"),TEXT("机器人驻地  /  行动整备"),62,122,700,35,17,Base,false,Muted);
    P.Label(TEXT("FundsLabel"),TEXT("可用资金"),1360,67,180,25,15,Base,false,Muted);
    P.Label(TEXT("Credits"),TEXT("0"),1360,96,180,45,32,Base,true);
    P.Button(TEXT("Overview"),TEXT("01  行动大厅"),40,184,222,55,Base);
    P.Button(TEXT("Warehouse"),TEXT("02  仓库与整备"),40,252,222,55,Base);
    P.Button(TEXT("Production"),TEXT("03  加工制造"),40,320,222,55,Base);
    P.Button(TEXT("Modification"),TEXT("04  枪械改装"),40,388,222,55,Base);
    P.Label(TEXT("Storage"),TEXT("仓库"),48,477,215,60,18,Base);
    P.Label(TEXT("FactoryRule"),TEXT("成功：背包全部入库\n失败：恢复出征整备\n仅丢本次搜获"),48,573,210,126,16,Base,false,Muted)->SetAutoWrapText(false);
    P.Surface(TEXT("MainSurface"),286,177,1008,615,Panel,Base,true);
    P.Label(TEXT("SectionTitle"),TEXT("个人工厂总览"),308,193,850,44,29,Base,true);
    AddLobby(P);
    auto Grid=[&](const TCHAR* Name,EFPSExtractionContainer Container,float X,float Y,float W,float H,float Cell,UCanvasPanel* Parent)
    {auto* G=P.Make<UFPSExtractionGridUI>(Name);G->Container=Container;G->CellSize=Cell;G->TextFont=P.Font;G->NumberFont=P.NumberFont;G->SetVisibility(ESlateVisibility::Visible);P.Place(Parent,G,X,Y,W,H);};
    auto* Inventory=P.Group(TEXT("InventoryPanel"),0,0,1600,900,Base);
    P.Label(TEXT("WarehouseHeader"),TEXT("仓库  /  8 × 8"),306,243,464,34,18,Inventory,true);
    P.Label(TEXT("BagCapacity"),TEXT("出征背包"),826,243,450,34,18,Inventory,true);
    Grid(TEXT("WarehouseGrid"),EFPSExtractionContainer::Warehouse,306,281,464,464,58,Inventory);
    Grid(TEXT("BaseBackpackGrid"),EFPSExtractionContainer::Backpack,826,281,290,232,58,Inventory);
    P.Label(TEXT("InventoryHelp"),TEXT("左键选择 / 拖动放置\n旋转按钮 / R 旋转\n同类物资可整组合并"),1134,287,140,154,15,Inventory,false,Muted);
    P.Label(TEXT("SelectedInfo"),TEXT("选择格中物资"),826,533,450,104,18,Inventory);
    P.Button(TEXT("SelectedRotate"),TEXT("旋转物品"),826,650,200,44,Inventory);
    P.Button(TEXT("SelectedTransfer"),TEXT("放入背包"),1042,650,230,44,Inventory);
    P.Button(TEXT("SelectedSell"),TEXT("出售整堆"),826,706,446,44,Inventory);
    P.Label(TEXT("InventoryFeedback"),TEXT(""),304,754,980,45,16,Inventory,false,Yellow);
    auto* Work=P.Group(TEXT("WorkbenchPanel"),0,0,1600,900,Base);
    P.Label(TEXT("SectionHint"),TEXT(""),308,242,950,53,17,Work,false,Muted);
    P.Rows(TEXT("BaseRows"),300,306,980,405,Work);
    P.Label(TEXT("ProductionStatus"),TEXT("生产槽空闲"),304,737,700,45,18,Work);
    P.Button(TEXT("Claim"),TEXT("领取产物"),1095,730,190,50,Work);
    auto* Equip=P.Group(TEXT("EquipmentPanel"),0,0,1600,900,Base);
    P.Surface(TEXT("LoadoutSurface"),1316,177,244,603,Panel,Equip,true);
    P.Label(TEXT("LoadoutTitle"),TEXT("出征装备"),1338,199,198,44,24,Equip,true);
    P.Label(TEXT("PrimarySlot"),TEXT("主武器 · 固定槽"),1338,249,198,30,15,Equip,false,Muted);
    P.Label(TEXT("WeaponSummary"),TEXT("M4\n配件 0 件"),1338,283,198,116,27,Equip,true)->SetAutoWrapText(false);
    P.Label(TEXT("AmmoSummary"),TEXT("弹药"),1338,401,198,38,18,Equip)->SetAutoWrapText(false);
    P.Surface(TEXT("EquipmentDivider"),1338,448,200,1,Muted,Equip);
    P.Label(TEXT("SecondarySlot"),TEXT("副武器 · 固定槽\nM9"),1338,464,198,78,22,Equip,true)->SetAutoWrapText(false);
    P.Label(TEXT("EquipmentPolicy"),TEXT("枪械、配件保留\n无维修费\n补给需 10 资金"),1338,560,195,100,16,Equip,false,Muted)->SetAutoWrapText(false);
    P.Button(TEXT("Resupply"),TEXT("补给 / 10 资金"),1338,681,200,51,Equip);
    P.Label(TEXT("Feedback"),TEXT(""),304,817,965,63,17,Base,false,Yellow);
    P.Label(TEXT("DeploymentMap"),TEXT("工业关隘 · 单人行动"),1319,785,241,30,16,Base);
    P.Button(TEXT("Depart"),TEXT("出征  →"),1316,823,244,54,Base,true);
    auto* HUD=P.Group(TEXT("RaidHUD"),0,0,1600,900);
    P.Surface(TEXT("RaidStatusBG"),40,105,380,110,FLinearColor(.01f,.017f,.02f,.8f),HUD,true);
    P.Surface(TEXT("RaidStatusAccent"),40,105,4,110,Yellow,HUD);
    P.Label(TEXT("ExtractionStatus"),TEXT("前往撤离点"),61,119,343,42,23,HUD,true);
    P.Label(TEXT("RaidCapacity"),TEXT("背包"),61,165,343,32,18,HUD);
    auto* Interaction=P.Group(TEXT("InteractionPanel"),490,555,620,62,HUD);
    P.Surface(TEXT("InteractionBG"),0,0,620,62,FLinearColor(.01f,.017f,.02f,.88f),Interaction,true);
    P.Label(TEXT("Interaction"),TEXT(""),18,11,584,45,22,Interaction,true,Yellow);
    P.Surface(TEXT("BagBackdrop"),0,0,1600,900,FLinearColor(0,0,0,.5f));
    auto* Bag=P.Group(TEXT("BagPanel"),0,0,1600,900);
    P.Surface(TEXT("BagSurface"),220,140,1160,700,Dark,Bag,true);
    P.Label(TEXT("BagTitle"),TEXT("行动背包"),250,166,730,64,34,Bag,true);
    P.Label(TEXT("RaidBagCapacity"),TEXT("5 × 4 格"),1040,180,280,45,24,Bag,true);
    P.Label(TEXT("BagRule"),TEXT("成功后全部入库；失败恢复出征前整备，仅丢本次搜获。"),250,237,1040,44,18,Bag,false,Muted);
    Grid(TEXT("RaidBackpackGrid"),EFPSExtractionContainer::Backpack,250,295,360,288,72,Bag);
    auto* Selection=P.Group(TEXT("RaidSelectionPanel"),0,0,1600,900,Bag);
    P.Label(TEXT("RaidSelectedInfo"),TEXT("选择格中物资"),665,301,640,138,21,Selection);
    P.Label(TEXT("RaidEquipment"),TEXT("出征装备  M4 / M9\n行动中不改装，不更换固定枪械槽。"),665,446,640,74,18,Selection,false,Muted);
    P.Button(TEXT("RaidRotate"),TEXT("旋转物品"),665,544,250,55,Selection)->Action=TEXT("SelectedRotate");
    P.Button(TEXT("SelectedDrop"),TEXT("丢弃整堆到地面"),939,544,368,55,Selection);
    P.Label(TEXT("BagFeedback"),TEXT(""),250,629,1060,60,18,Bag,false,Yellow);
    P.Button(TEXT("Abort"),TEXT("放弃本次行动"),250,740,300,55,Bag);
    P.Button(TEXT("CloseBag"),TEXT("关闭背包 / Tab"),1025,740,290,55,Bag);
    auto* Confirm=P.Group(TEXT("AbortConfirm"),430,300,740,290);
    P.Surface(TEXT("ConfirmBG"),0,0,740,290,Dark,Confirm,true);
    P.Label(TEXT("ConfirmTitle"),TEXT("放弃本次行动？"),30,22,680,53,31,Confirm,true);
    P.Label(TEXT("ConfirmRule"),TEXT("仅遗失本次搜获，恢复出征前整备背包。枪械与已装配件保留，无维修费。"),30,94,680,83,20,Confirm);
    P.Button(TEXT("CancelAbort"),TEXT("继续行动"),30,217,320,52,Confirm);
    P.Button(TEXT("ConfirmAbort"),TEXT("确认放弃"),390,217,320,52,Confirm,true);
    auto* Result=P.Group(TEXT("ResultPanel"),0,0,1600,900);
    P.Surface(TEXT("ResultAccent"),170,130,6,87,Yellow,Result);
    P.Label(TEXT("ResultTitle"),TEXT("行动结果"),202,122,1140,80,50,Result,true);
    P.Label(TEXT("ResultNote"),TEXT(""),202,216,1170,90,23,Result);
    P.Label(TEXT("ResultListTitle"),TEXT("本次物资"),310,322,980,45,25,Result,true);
    P.Rows(TEXT("ResultRows"),310,384,980,306,Result);
    P.Label(TEXT("ResultFeedback"),TEXT(""),310,711,980,54,19,Result,false,Yellow);
    P.Button(TEXT("Return"),TEXT("返回基地"),990,786,300,66,Result,true);
    P.Label(TEXT("ResultPolicy"),TEXT("出征枪械与已装配件保留"),310,800,660,43,20,Result,false,Muted);
    for(const TCHAR* Name:{TEXT("InventoryPanel"),TEXT("WorkbenchPanel"),TEXT("RaidHUD"),TEXT("BagBackdrop"),TEXT("BagPanel"),TEXT("AbortConfirm"),TEXT("ResultPanel")})P.Tree->FindWidget(Name)->SetVisibility(ESlateVisibility::Collapsed);
    AddReferenceWorkbench(P);
    return P.Save();
}
}
#endif

FString UFPSExtractionUIBuilder::BuildAssets()
{
#if WITH_EDITOR
    using namespace ExtractionUI;
    int32 Created=0;
    if(!EnsureAttachmentRow())return TEXT("FAIL attachment row save");
    if(!LoadObject<UWidgetBlueprint>(nullptr,*(FString(Root)+TEXT("WBP_ExtractionRow.WBP_ExtractionRow"))))
    {
        FLayout P(TEXT("WBP_ExtractionRow"),true);if(!P.BP)return TEXT("FAIL missing theme/font or row factory");
        P.Surface(TEXT("RowBackground"),0,0,980,116,FLinearColor(.027f,.034f,.038f,1),nullptr,true);
        P.Surface(TEXT("RowLine"),0,0,3,116,FLinearColor(.19f,.23f,.24f,1));
        P.Label(TEXT("RowTitle"),TEXT("物资名称"),20,10,520,32,23,nullptr,true);
        P.Label(TEXT("RowDetail"),TEXT("物资说明"),20,47,630,57,16);
        P.Label(TEXT("RowCount"),TEXT("× 0"),655,12,130,30,23,nullptr,true);
        P.Button(TEXT("RowAction"),TEXT("操作"),793,17,167,48);
        P.Label(TEXT("RowReason"),TEXT(""),680,77,280,32,15,nullptr,false,FLinearColor(.65f,.52f,.32f,1));
        if(!P.Save())return TEXT("FAIL row save");++Created;
    }
    if(!LoadObject<UWidgetBlueprint>(nullptr,*(FString(Root)+TEXT("WBP_ExtractionTerminal.WBP_ExtractionTerminal"))))
    {if(!BuildInventoryTerminal(false))return TEXT("FAIL inventory terminal save");++Created;}
    return FString::Printf(TEXT("PASS extraction UI created=%d (existing assets preserved)"),Created);
#else
    return TEXT("FAIL editor required");
#endif
}

FString UFPSExtractionUIBuilder::UpgradeInventoryAssets()
{
#if WITH_EDITOR
    auto* BP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal.WBP_ExtractionTerminal"));
    if(BP&&BP->WidgetTree&&BP->WidgetTree->FindWidget(TEXT("InventoryLayoutVersion")))return TEXT("PASS inventory UI v2 already present; Designer edits preserved");
    const FString Initial=BuildAssets();if(!Initial.StartsWith(TEXT("PASS")))return Initial;
    BP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal.WBP_ExtractionTerminal"));
    if(BP&&BP->WidgetTree&&BP->WidgetTree->FindWidget(TEXT("InventoryLayoutVersion")))return TEXT("PASS inventory UI v2 created");
    return ExtractionUI::BuildInventoryTerminal(true)?TEXT("PASS inventory UI v1 migrated to v2"):TEXT("FAIL inventory UI migration");
#else
    return TEXT("FAIL editor required");
#endif
}

FString UFPSExtractionUIBuilder::UpgradeWeaponWorkbenchAssets()
{
#if WITH_EDITOR
    using namespace ExtractionUI;
    auto* BP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal.WBP_ExtractionTerminal"));
    if(!BP||!BP->WidgetTree||!BP->WidgetTree->FindWidget(TEXT("InventoryLayoutVersion")))
        return TEXT("FAIL workbench migration requires v2 terminal; run inventory upgrade first");
    if(BP->WidgetTree->FindWidget(TEXT("WeaponWorkbenchLayoutVersion")))
        return EnsureAttachmentRow()?TEXT("PASS workbench v3 already present; Designer edits preserved"):TEXT("FAIL attachment row save");
    FLayout P(BP);auto* Base=Cast<UCanvasPanel>(P.Tree->FindWidget(TEXT("BasePanel")));
    if(!Base||!P.Canvas||!P.Theme||!P.Font||!P.NumberFont)return TEXT("FAIL workbench base tree/theme/fonts missing");
    // Refuse partial/custom naming collisions rather than generating duplicate widget names.
    for(const TCHAR* Name:{TEXT("ModificationPanel"),TEXT("WeaponWorkbenchBG"),TEXT("WeaponWorkbenchTitle"),TEXT("WeaponWorkbenchHelp"),
        TEXT("WeaponViewport"),TEXT("WeaponStatsDivider"),TEXT("CurrentStatsTitle"),TEXT("CurrentWeaponStats"),TEXT("CandidateStatsTitle"),
        TEXT("CandidateWeaponStats"),TEXT("AttachmentColumnBG"),TEXT("AttachmentColumnTitle"),TEXT("SlotOptic"),TEXT("SlotOpticLabel"),
        TEXT("SlotMuzzle"),TEXT("SlotMuzzleLabel"),TEXT("SlotMagazine"),TEXT("SlotMagazineLabel"),TEXT("AttachmentCategory"),TEXT("AttachmentRows")})
        if(P.Tree->FindWidget(Name))return FString::Printf(TEXT("FAIL workbench widget name already exists: %s"),Name);
    if(!EnsureAttachmentRow())return TEXT("FAIL attachment row save");
    BP->Modify();P.Tree->Modify();Base->Modify();AddWeaponWorkbench(P,Base);
    return P.Save()?TEXT("PASS workbench v2 migrated to v3; existing panels preserved"):TEXT("FAIL workbench terminal save");
#else
    return TEXT("FAIL editor required");
#endif
}

FString UFPSExtractionUIBuilder::UpgradeWeaponWorkbenchReferenceAssets(bool Rebuild)
{
#if WITH_EDITOR
    using namespace ExtractionUI;
    auto* BP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal.WBP_ExtractionTerminal"));
    if(!BP||!BP->WidgetTree)return TEXT("FAIL missing terminal; create/upgrade inventory first");
    auto* Version=Cast<UTextBlock>(BP->WidgetTree->FindWidget(TEXT("WeaponWorkbenchLayoutVersion")));
    if(Version&&Version->GetText().ToString()==TEXT("4")&&!Rebuild)return TEXT("PASS workbench v4 already present; Designer edits preserved");
    if(!Version||(Version->GetText().ToString()!=TEXT("3")&&!(Rebuild&&Version->GetText().ToString()==TEXT("4"))))return TEXT("FAIL reference workbench migration requires v3 or explicit v4 rebuild");
    auto* Old=BP->WidgetTree->FindWidget(TEXT("ModificationPanel"));if(!Old)return TEXT("FAIL v3 ModificationPanel missing");
    FLayout P(BP);if(!P.Canvas||!P.Theme||!P.Font||!P.NumberFont)return TEXT("FAIL layout/theme/fonts missing");
    for(const TCHAR* Name:{TEXT("WeaponOverviewPanel"),TEXT("AttachmentDrawer"),TEXT("WeaponEnterPanel"),TEXT("ViewPrimaryWeapon"),TEXT("ViewSecondaryWeapon"),TEXT("EnterWeaponModification")})
        if(!Rebuild&&P.Tree->FindWidget(Name))return FString::Printf(TEXT("FAIL reference widget name already exists: %s"),Name);
    if(!EnsureAttachmentRow())return TEXT("FAIL attachment row missing");
    TArray<UWidget*> Retired;UWidgetTree::ForWidgetAndChildren(Old,[&](UWidget* Widget){Retired.Add(Widget);});Retired.Add(Version);
    BP->Modify();P.Tree->Modify();Old->RemoveFromParent();Version->RemoveFromParent();
    for(UWidget* Widget:Retired)
    {
        Widget->Modify();const FName RetiredName=MakeUniqueObjectName(GetTransientPackage(),Widget->GetClass(),FName(*(TEXT("Retired_")+Widget->GetName())));
        if(!Widget->Rename(*RetiredName.ToString(),GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional))return TEXT("FAIL retiring old workbench widget");
    }
    AddReferenceWorkbench(P);
    return P.Save()?TEXT("PASS reference workbench v3 migrated to v4; other panels preserved"):TEXT("FAIL reference workbench save");
#else
    return TEXT("FAIL editor required");
#endif
}

FString UFPSExtractionUIBuilder::UpgradeLobbyAssets(bool Rebuild)
{
#if WITH_EDITOR
    using namespace ExtractionUI;
    auto* BP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal.WBP_ExtractionTerminal"));
    if(!BP||!BP->WidgetTree)return TEXT("FAIL missing terminal");
    auto* Version=Cast<UTextBlock>(BP->WidgetTree->FindWidget(TEXT("LobbyLayoutVersion")));
    if(Version&&!Rebuild)return TEXT("PASS lobby already present; Designer edits preserved");
    auto* Workbench=Cast<UTextBlock>(BP->WidgetTree->FindWidget(TEXT("WeaponWorkbenchLayoutVersion")));
    if(!Workbench||Workbench->GetText().ToString()!=TEXT("4"))return TEXT("FAIL lobby requires v4 workbench");
    auto* Old=BP->WidgetTree->FindWidget(TEXT("OverviewPanel"));if(!Old)return TEXT("FAIL old overview missing");
    FLayout P(BP);if(!P.Canvas||!P.Theme||!P.Font||!P.NumberFont)return TEXT("FAIL layout/theme/fonts missing");
    TArray<UWidget*> Retired;UWidgetTree::ForWidgetAndChildren(Old,[&](UWidget* Widget){Retired.Add(Widget);});if(Version)Retired.Add(Version);
    BP->Modify();P.Tree->Modify();Old->RemoveFromParent();if(Version)Version->RemoveFromParent();
    for(auto* Widget:Retired)
    {
        const FName Name=MakeUniqueObjectName(GetTransientPackage(),Widget->GetClass(),FName(*(TEXT("Retired_")+Widget->GetName())));
        if(!Widget->Rename(*Name.ToString(),GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional))return TEXT("FAIL retiring old lobby widget");
    }
    AddLobby(P);
    if(auto* Label=Cast<UTextBlock>(P.Tree->FindWidget(TEXT("OverviewLabel"))))Label->SetText(FText::FromString(TEXT("01  行动大厅")));
    return P.Save()?TEXT("PASS lobby saved; inventory and workbench preserved"):TEXT("FAIL lobby save");
#else
    return TEXT("FAIL editor required");
#endif
}
