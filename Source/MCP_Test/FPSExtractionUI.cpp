#include "FPSExtractionUI.h"
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
{return Category==EFPSAttachmentCategory::Optic?TEXT("瞄具"):Category==EFPSAttachmentCategory::Muzzle?TEXT("枪口"):TEXT("弹匣");}
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
{return FString::Printf(TEXT("容量  %d 发\n换弹  %.2f 秒\n开镜  %.2f 秒\n后坐力倍率  纵 %.2f / 横 %.2f"),S.MagazineSize,S.ReloadDuration,S.AimTransitionTime,S.VerticalRecoilMultiplier,S.HorizontalRecoilMultiplier);}
FString CandidateStats(const FFPSResolvedWeaponStats& S,const FFPSResolvedWeaponStats& C)
{return FString::Printf(TEXT("容量  %d  (%+d)\n换弹  %.2f 秒  (%+.2f)\n开镜  %.2f 秒  (%+.2f)\n后坐力  纵 %+.2f / 横 %+.2f"),S.MagazineSize,S.MagazineSize-C.MagazineSize,S.ReloadDuration,S.ReloadDuration-C.ReloadDuration,S.AimTransitionTime,S.AimTransitionTime-C.AimTransitionTime,S.VerticalRecoilMultiplier-C.VerticalRecoilMultiplier,S.HorizontalRecoilMultiplier-C.HorizontalRecoilMultiplier);}
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
    if(auto* Preview=ExtractionUI::Find<UFPSExtractionWeaponViewport>(this,TEXT("WeaponViewport")))
    {Preview->OnSlotSelected.RemoveAll(this);Preview->SetPreviewActive(false);}
    Super::NativeDestruct();
}
void UFPSExtractionUI::NativeTick(const FGeometry& Geometry,float DeltaTime)
{Super::NativeTick(Geometry,DeltaTime);RefreshClock+=DeltaTime;if(RefreshClock>=.1f){RefreshClock=0;Refresh();}}
FReply UFPSExtractionUI::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    // Tunnel before a focused button turns Tab into navigation; do not double-toggle in the controller.
    if(Phase==EFPSExtractionPhase::Raid&&bBagOpen&&(Event.GetKey()==EKeys::Tab||Event.GetKey()==EKeys::Escape))
    {SetPanelOpen(false);return FReply::Handled();}
    if(Event.GetKey()==EKeys::R&&IsPanelOpen())
    {Dispatch(TEXT("SelectedRotate"));return FReply::Handled();}
    if(Event.GetKey()==EKeys::Escape&&Phase==EFPSExtractionPhase::Base)
    {if(ActiveDrag){UWidgetBlueprintLibrary::CancelDragDrop();EndInventoryDrag();}else Dispatch(TEXT("Overview"));return FReply::Handled();}
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
    auto* S=Model();if(S&&S->MoveStack(Drag->From,Drag->StackId,To,X,Y,Drag->bRotated)){SelectedContainer=To;SelectedStack=Drag->StackId;}
    EndInventoryDrag();Refresh();
}
void UFPSExtractionUI::InventoryFeedback(const FString& Text)
{LocalFeedback=Text;SetText(TEXT("InventoryFeedback"),Text);SetText(TEXT("BagFeedback"),Text);}
void UFPSExtractionUI::SelectAttachmentCategory(EFPSAttachmentCategory Category)
{
    if(Category!=EFPSAttachmentCategory::Optic&&Category!=EFPSAttachmentCategory::Muzzle&&Category!=EFPSAttachmentCategory::Magazine)return;
    if(SelectedCategory==Category)return;
    SelectedCategory=Category;ComparedAction=NAME_None;ComparedId=NAME_None;LastRows.Empty();Refresh();
}
void UFPSExtractionUI::PreviewAttachmentStats(FName Action,FName Id)
{if(Phase==EFPSExtractionPhase::Base&&Tab==3){ComparedAction=Action;ComparedId=Id;if(auto* S=Model())RefreshAttachmentStats(S->GetView());}}
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
    {PartId=ComparedId;if(const auto* Part=Catalog->FindAttachment(PartId))Name=Part->DisplayName.ToString();}
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
        for(FName Id:V.Loadout.Primary.AttachmentIds)
            if(const auto* Part=Catalog->FindAttachment(Id);Part&&Part->Category==SelectedCategory)
            {FFPSResolvedWeaponStats Stats;const bool Valid=ExtractionUI::ResolveVariant(Catalog,V.Loadout.Primary,Id,true,Stats);
            const auto* Item=S->GetItems().FindByPredicate([&](const auto& Entry){return Entry.AttachmentId==Id;});
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
    if(Action==TEXT("SlotOptic")){SelectAttachmentCategory(EFPSAttachmentCategory::Optic);return;}
    if(Action==TEXT("SlotMuzzle")){SelectAttachmentCategory(EFPSAttachmentCategory::Muzzle);return;}
    if(Action==TEXT("SlotMagazine")){SelectAttachmentCategory(EFPSAttachmentCategory::Magazine);return;}
    if(Action==TEXT("Overview"))Tab=0;
    else if(Action==TEXT("Warehouse"))Tab=1;
    else if(Action==TEXT("SellTab"))Tab=1;
    else if(Action==TEXT("Production"))Tab=2;
    else if(Action==TEXT("Modification"))Tab=3;
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
    if(Tab==3){const auto* Catalog=GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();S+=FString::Printf(TEXT("/category%d/ready%d"),int32(SelectedCategory),Catalog&&Catalog->IsReady());}
    return S;
}
void UFPSExtractionUI::Refresh()
{
    auto* S=Model();if(!S||!WidgetTree)return;
    const auto V=S->GetView();
    if(Phase!=V.Phase){if(ActiveDrag){UWidgetBlueprintLibrary::CancelDragDrop();EndInventoryDrag();}SelectedStack.Invalidate();LocalFeedback.Empty();}
    Phase=V.Phase;
    const bool Base=Phase==EFPSExtractionPhase::Base,Raid=Phase==EFPSExtractionPhase::Raid,Result=Phase==EFPSExtractionPhase::Result;
    SetShown(TEXT("BasePanel"),Base);SetShown(TEXT("RaidHUD"),Raid);
    SetShown(TEXT("OverviewPanel"),Base&&Tab==0);SetShown(TEXT("InventoryPanel"),Base&&Tab==1);
    SetShown(TEXT("WorkbenchPanel"),Base&&Tab==2);
    SetShown(TEXT("EquipmentPanel"),Base&&Tab!=3);
    SetShown(TEXT("ModificationPanel"),Base&&Tab==3);
    if(auto* Preview=ExtractionUI::Find<UFPSExtractionWeaponViewport>(this,TEXT("WeaponViewport")))
    {Preview->SetPreviewActive(Base&&Tab==3);if(Base&&Tab==3){Preview->Present(V.Loadout.Primary);Preview->SetSelectedCategory(SelectedCategory);}}
    if(Base&&Tab==3)RefreshAttachmentStats(V);
    SetShown(TEXT("BagPanel"),Raid&&bBagOpen);SetShown(TEXT("ResultPanel"),Result);
    SetShown(TEXT("FullBackdrop"),!Raid);SetShown(TEXT("BagBackdrop"),Raid&&bBagOpen);
    SetShown(TEXT("AbortConfirm"),Raid&&bBagOpen&&bConfirmAbort);
    SetEnabled(TEXT("RaidBackpackGrid"),!bConfirmAbort);SetEnabled(TEXT("RaidSelectionPanel"),!bConfirmAbort);SetEnabled(TEXT("Abort"),!bConfirmAbort);SetEnabled(TEXT("CloseBag"),!bConfirmAbort);
    SetText(TEXT("Credits"),FString::Printf(TEXT("%d"),V.Credits));
    SetText(TEXT("Storage"),FString::Printf(TEXT("仓库  %d / %d 格"),V.WarehouseUsed,V.WarehouseSlots));
    SetText(TEXT("Feedback"),V.bStorageHealthy?V.Feedback:TEXT("存档不可写，请检查后重试。\n")+V.Feedback);
    if(LastBackendFeedback!=V.Feedback){LocalFeedback.Empty();LastBackendFeedback=V.Feedback;}
    const FString Feedback=LocalFeedback.IsEmpty()?V.Feedback:LocalFeedback;
    SetText(TEXT("InventoryFeedback"),Feedback);SetText(TEXT("BagFeedback"),Feedback);SetText(TEXT("ResultFeedback"),V.Feedback);
    SetText(TEXT("BagCapacity"),FString::Printf(TEXT("出征背包  %d / %d 格"),V.BackpackUsed,V.BackpackSlots));
    SetText(TEXT("RaidBagCapacity"),FString::Printf(TEXT("%d / %d 格"),V.BackpackUsed,V.BackpackSlots));
    SetText(TEXT("HomeStorage"),FString::Printf(TEXT("仓库 %d / %d 格\n背包 %d / %d 格"),V.WarehouseUsed,V.WarehouseSlots,V.BackpackUsed,V.BackpackSlots));
    SetText(TEXT("HomeEquipment"),FString::Printf(TEXT("M4  /  M9\n主武器配件 %d 件"),V.Loadout.Primary.AttachmentIds.Num()));
    SetText(TEXT("RaidCapacity"),FString::Printf(TEXT("背包  %d / %d  ·  Tab"),V.BackpackUsed,V.BackpackSlots));
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
    const FString Signature=RowSignature(V);if(!ActiveDrag&&Signature!=LastRows){if(Base&&Tab==3)RebuildAttachmentRows(V);else if(Result||(Base&&Tab==2))RebuildRows(V);LastRows=Signature;}
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
        if(auto* UI=Cast<UFPSExtractionUI>(BP->GeneratedClass->GetDefaultObject()))UI->Theme=Theme;
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
    P.Button(TEXT("Overview"),TEXT("01  工厂总览"),40,184,222,55,Base);
    P.Button(TEXT("Warehouse"),TEXT("02  仓库与整备"),40,252,222,55,Base);
    P.Button(TEXT("Production"),TEXT("03  加工制造"),40,320,222,55,Base);
    P.Button(TEXT("Modification"),TEXT("04  枪械改装"),40,388,222,55,Base);
    P.Label(TEXT("Storage"),TEXT("仓库"),48,477,215,60,18,Base);
    P.Label(TEXT("FactoryRule"),TEXT("成功：背包全部入库\n失败：恢复出征整备\n仅丢本次搜获"),48,573,210,126,16,Base,false,Muted)->SetAutoWrapText(false);
    P.Surface(TEXT("MainSurface"),286,177,1008,615,Panel,Base,true);
    P.Label(TEXT("SectionTitle"),TEXT("个人工厂总览"),308,193,850,44,29,Base,true);
    auto* Home=P.Group(TEXT("OverviewPanel"),0,0,1600,900,Base);
    P.Label(TEXT("HomeGuide"),TEXT("整理物资，生产配件，准备下一次回收行动。"),310,252,930,46,19,Home,false,Muted);
    const float Xs[]={310,632,954};
    const TCHAR* TileNames[]={TEXT("StoreFacility"),TEXT("ProductionFacility"),TEXT("WorkshopFacility")};
    for(int32 I=0;I<3;++I)
    {
        P.Surface(*(FString(TileNames[I])+TEXT("BG")),Xs[I],320,298,348,Dark,Home,true);
        P.Surface(*(FString(TileNames[I])+TEXT("Rule")),Xs[I]+22,343,254,2,I==0?Yellow:Muted,Home);
        // Simple original facility silhouettes built from saved geometry, no borrowed art.
        for(int32 K=0;K<3;++K)P.Surface(*FString::Printf(TEXT("FacilityShape%d_%d"),I,K),Xs[I]+30+K*78,382+(I==1?K*9:0),60,I==2?26:50,FLinearColor(.17f,.21f,.21f,1),Home,true);
    }
    P.Label(TEXT("HomeStorage"),TEXT("仓库 / 背包"),332,468,254,83,21,Home,true);
    P.Label(TEXT("HomeProduction"),TEXT("生产槽空闲"),654,468,254,91,21,Home,true);
    P.Label(TEXT("HomeEquipment"),TEXT("M4 / M9"),976,468,254,91,21,Home,true);
    P.Button(TEXT("EnterWarehouse"),TEXT("进入仓库与整备"),332,581,254,60,Home)->Action=TEXT("Warehouse");
    P.Button(TEXT("EnterProduction"),TEXT("进入加工制造"),654,581,254,60,Home)->Action=TEXT("Production");
    P.Button(TEXT("EnterModification"),TEXT("进入枪械改装"),976,581,254,60,Home)->Action=TEXT("Modification");
    P.Label(TEXT("HomePolicy"),TEXT("设施状态来自实际仓库与生产任务。整备背包中的物资会随本次出征携带。"),310,703,950,63,18,Home,false,Muted);
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
    AddWeaponWorkbench(P,Base);
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
