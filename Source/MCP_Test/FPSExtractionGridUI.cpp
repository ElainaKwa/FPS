#include "FPSExtractionGridUI.h"
#include "FPSExtractionUI.h"
#include "FPSExtractionSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Font.h"

void UFPSExtractionGridDrag::Rotate()
{
    // Clockwise rotation keeps the originally grabbed cell under the pointer.
    const int32 OldX=GrabX;
    if(!bRotated){GrabX=Height-1-GrabY;GrabY=OldX;}
    else {GrabX=GrabY;GrabY=Width-1-OldX;}
    Swap(Width,Height);bRotated=!bRotated;
}
void UFPSExtractionGridDrag::DragCancelled_Implementation(const FPointerEvent& Event)
{Super::DragCancelled_Implementation(Event);if(OwnerUI)OwnerUI->EndInventoryDrag();}
void UFPSExtractionGridUI::Present(UFPSExtractionUI* Owner,const FFPSExtractionView& V,const TArray<FFPSExtractionItem>& Items)
{
    OwnerUI=Owner;Definitions=Items;
    const bool Warehouse=Container==EFPSExtractionContainer::Warehouse;
    Stacks=Warehouse?V.Warehouse:V.Backpack;
    Columns=Warehouse?V.WarehouseWidth:V.BackpackWidth;Rows=Warehouse?V.WarehouseHeight:V.BackpackHeight;
    if(Owner&&Owner->WidgetTree)if(auto* Help=Cast<UTextBlock>(Owner->WidgetTree->FindWidget(TEXT("InventoryHelp"))))
        Help->SetText(FText::FromString(TEXT("拖放 / R 旋转\nCtrl+点击互转\n右键拆分数量\n同类物资补满")));
    if(!Owner||!Owner->IsInventoryDragging())bPreview=false;
}
const FFPSExtractionItem* UFPSExtractionGridUI::Definition(FName Id) const
{return Definitions.FindByPredicate([Id](const auto& I){return I.ItemId==Id;});}
const FFPSExtractionStack* UFPSExtractionGridUI::Hit(int32 X,int32 Y) const
{
    for(const auto& S:Stacks)if(const auto* D=Definition(S.ItemId))
    {const int32 W=S.bRotated?D->Height:D->Width,H=S.bRotated?D->Width:D->Height;if(X>=S.X&&X<S.X+W&&Y>=S.Y&&Y<S.Y+H)return &S;}
    return nullptr;
}
FIntPoint UFPSExtractionGridUI::CellAt(const FGeometry& G,FVector2D Absolute) const
{const FVector2D P=G.AbsoluteToLocal(Absolute);return {int32(FMath::FloorToInt(P.X/CellSize)),int32(FMath::FloorToInt(P.Y/CellSize))};}
FReply UFPSExtractionGridUI::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{
    const bool RightClick=E.GetEffectingButton()==EKeys::RightMouseButton;
    if((E.GetEffectingButton()!=EKeys::LeftMouseButton&&!RightClick)||!OwnerUI.IsValid())return Super::NativeOnMouseButtonDown(G,E);
    PressedCell=CellAt(G,E.GetScreenSpacePosition());
    if(const auto* S=Hit(PressedCell.X,PressedCell.Y))
    {
        // SelectInventoryStack refreshes the snapshot, invalidating S.
        const FGuid Id=S->StackId;const int32 Quantity=S->Quantity;
        PressedStack=Id;OwnerUI->SelectInventoryStack(Container,Id);
        if(RightClick)
        {
            if(Quantity<=1)OwnerUI->InventoryFeedback(TEXT("该堆叠只有 1 件，无法拆分。"));
            else if(OwnerUI->CanInventoryMove(Container,Container))
            {
                if(SplitDialog)SplitDialog->Close();
                SplitDialog=CreateWidget<UFPSExtractionSplitDialog>(GetOwningPlayer());
                if(SplitDialog)SplitDialog->OpenFor(this,Id,Quantity);
            }
            return FReply::Handled();
        }
        if(E.IsControlDown())
        {
            OwnerUI->Dispatch(TEXT("SelectedTransfer"));
            const auto To=Container==EFPSExtractionContainer::Warehouse?EFPSExtractionContainer::Backpack:EFPSExtractionContainer::Warehouse;
            if(auto* Model=GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>())
            {
                const auto View=Model->GetView();const auto& Target=To==EFPSExtractionContainer::Backpack?View.Backpack:View.Warehouse;
                if(Target.ContainsByPredicate([Id](const auto& Stack){return Stack.StackId==Id;}))OwnerUI->SelectInventoryStack(To,Id);
            }
            return FReply::Handled();
        }
        OwnerUI->InventoryFeedback(TEXT("拖动移动 · R 旋转 · Ctrl+点击快速转移 · 右键拆分"));
        return FReply::Handled().DetectDrag(TakeWidget(),EKeys::LeftMouseButton);
    }
    PressedStack.Invalidate();OwnerUI->SelectInventoryStack(Container,FGuid());return FReply::Handled();
}
void UFPSExtractionGridUI::NativeOnDragDetected(const FGeometry& G,const FPointerEvent& E,UDragDropOperation*& Operation)
{
    if(!OwnerUI.IsValid())return;
    const auto* S=Stacks.FindByPredicate([this](const auto& I){return I.StackId==PressedStack;});
    const auto* D=S?Definition(S->ItemId):nullptr;if(!S||!D)return;
    auto* Drag=NewObject<UFPSExtractionGridDrag>();Drag->OwnerUI=OwnerUI.Get();Drag->StackId=S->StackId;Drag->From=Container;
    Drag->ItemId=S->ItemId;Drag->Quantity=S->Quantity;
    Drag->bRotated=S->bRotated;Drag->Width=S->bRotated?D->Height:D->Width;Drag->Height=S->bRotated?D->Width:D->Height;
    Drag->GrabX=PressedCell.X-S->X;Drag->GrabY=PressedCell.Y-S->Y;Drag->Pivot=EDragPivot::MouseDown;
    // A compact label follows the mouse; the full occupied rectangle previews in the target grid.
    auto* Visual=NewObject<UTextBlock>(Drag);Visual->SetText(FText::FromString(D->Name.ToString()+FString::Printf(TEXT(" ×%d"),S->Quantity)));
    Visual->SetFont(FSlateFontInfo(TextFont.Get(),15,TEXT("Bold")));Visual->SetColorAndOpacity(FLinearColor(.95f,.73f,.18f,1));
    Drag->DefaultDragVisual=Visual;Operation=Drag;OwnerUI->BeginInventoryDrag(Drag);
}
bool UFPSExtractionGridUI::PreviewFits(const UFPSExtractionGridDrag& D,int32 X,int32 Y,FString& Reason) const
{
    if(!OwnerUI.IsValid()||!OwnerUI->CanInventoryMove(D.From,Container)){Reason=TEXT("当前阶段不能移入此容器");return false;}
    // Match MoveStack: fill the target stack and leave any excess at the source.
    for(const auto& S:Stacks)
    {
        if(D.From==Container&&S.StackId==D.StackId)continue;
        if(const auto* I=Definition(S.ItemId))
        {const int32 W=S.bRotated?I->Height:I->Width,H=S.bRotated?I->Width:I->Height;
        if(X>=S.X&&X<S.X+W&&Y>=S.Y&&Y<S.Y+H&&S.ItemId==D.ItemId)
        {const int32 Room=I->StackLimit-S.Quantity;
        if(Room>0){Reason=FString::Printf(TEXT("松开合并 %d 件 · 余量保留原位"),FMath::Min(D.Quantity,Room));return true;}
        Reason=TEXT("目标堆叠已满");return false;}}
    }
    if(X<0||Y<0||X+D.Width>Columns||Y+D.Height>Rows){Reason=TEXT("放置超出格子边界");return false;}
    for(const auto& S:Stacks)
    {
        if(D.From==Container&&S.StackId==D.StackId)continue;
        if(const auto* I=Definition(S.ItemId))
        {const int32 W=S.bRotated?I->Height:I->Width,H=S.bRotated?I->Width:I->Height;
        if(X<S.X+W&&X+D.Width>S.X&&Y<S.Y+H&&Y+D.Height>S.Y){Reason=TEXT("目标格子被物资占用");return false;}}
    }
    Reason=TEXT("松开放置 · R 旋转；也可选中后使用旋转按钮");return true;
}
bool UFPSExtractionGridUI::NativeOnDragOver(const FGeometry& G,const FDragDropEvent& E,UDragDropOperation* Operation)
{
    auto* D=Cast<UFPSExtractionGridDrag>(Operation);if(!D||D->OwnerUI!=OwnerUI.Get())return false;
    const auto C=CellAt(G,E.GetScreenSpacePosition());PreviewX=C.X-D->GrabX;PreviewY=C.Y-D->GrabY;PreviewW=D->Width;PreviewH=D->Height;
    FString Reason;bPreview=true;bFits=PreviewFits(*D,PreviewX,PreviewY,Reason);
    if(bFits)if(const auto* Under=Hit(PreviewX,PreviewY);Under&&Under->StackId!=D->StackId&&Under->ItemId==D->ItemId)
        if(const auto* I=Definition(Under->ItemId)){PreviewX=Under->X;PreviewY=Under->Y;PreviewW=Under->bRotated?I->Height:I->Width;PreviewH=Under->bRotated?I->Width:I->Height;}
    OwnerUI->InventoryFeedback(Reason);return true;
}
void UFPSExtractionGridUI::NativeOnDragLeave(const FDragDropEvent& E,UDragDropOperation* O)
{Super::NativeOnDragLeave(E,O);bPreview=false;}
bool UFPSExtractionGridUI::NativeOnDrop(const FGeometry& G,const FDragDropEvent& E,UDragDropOperation* O)
{
    auto* D=Cast<UFPSExtractionGridDrag>(O);if(!D||D->OwnerUI!=OwnerUI.Get())return false;
    const auto C=CellAt(G,E.GetScreenSpacePosition());bPreview=false;
    OwnerUI->CommitInventoryDrag(D,Container,C.X-D->GrabX,C.Y-D->GrabY);return true;
}
int32 UFPSExtractionGridUI::NativePaint(const FPaintArgs& A,const FGeometry& G,const FSlateRect& C,FSlateWindowElementList& E,int32 L,const FWidgetStyle& Style,bool Enabled) const
{
    L=Super::NativePaint(A,G,C,E,L,Style,Enabled);
    const auto* Brush=FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    auto Box=[&](float X,float Y,float W,float H,FLinearColor Color,int32 Layer)
    {FSlateDrawElement::MakeBox(E,Layer,G.ToPaintGeometry(FVector2D(W,H),FSlateLayoutTransform(FVector2D(X,Y))),Brush,ESlateDrawEffect::None,Color);};
    auto Text=[&](float X,float Y,const FString& Value,int32 Size,bool Numeric,FLinearColor Color)
    {FSlateDrawElement::MakeText(E,L+4,G.ToPaintGeometry(FVector2D(Columns*CellSize,Rows*CellSize),FSlateLayoutTransform(FVector2D(X,Y))),Value,FSlateFontInfo(Numeric?NumberFont.Get():TextFont.Get(),Size,TEXT("Regular")),ESlateDrawEffect::None,Color);};
    for(int32 Y=0;Y<Rows;++Y)for(int32 X=0;X<Columns;++X)
    {Box(X*CellSize,Y*CellSize,CellSize-1,CellSize-1,GridColor,L+1);Box(X*CellSize+1,Y*CellSize+1,CellSize-3,CellSize-3,FLinearColor(.019f,.026f,.029f,1),L+2);}
    for(const auto& S:Stacks)if(const auto* D=Definition(S.ItemId))
    {
        const int32 W=S.bRotated?D->Height:D->Width,H=S.bRotated?D->Width:D->Height;
        const bool Selected=OwnerUI.IsValid()&&OwnerUI->IsInventorySelected(Container,S.StackId);
        const float X=S.X*CellSize,Y=S.Y*CellSize;
        Box(X+2,Y+2,W*CellSize-5,H*CellSize-5,Selected?FLinearColor(.26f,.24f,.12f,1):ItemColor,L+3);
        Box(X+2,Y+2,3,H*CellSize-5,Selected?FLinearColor(.96f,.73f,.16f,1):FLinearColor(.32f,.42f,.43f,1),L+4);
        // Tiny cells must distinguish attachments; truncating "M4 ..." hides their type.
        const FString Label=S.ItemId==TEXT("Optic")?TEXT("瞄具"):S.ItemId==TEXT("Magazine")?TEXT("弹匣"):
            S.ItemId==TEXT("Brake")?TEXT("制退"):D->Name.ToString().Left(W*2);
        Text(X+8,Y+6,Label,14,false,FLinearColor(.84f,.87f,.83f,1));
        Text(X+8,Y+H*CellSize-27,FString::Printf(TEXT("%d"),S.Quantity),15,true,FLinearColor(.84f,.87f,.83f,1));
        if(W>1)Text(X+W*CellSize-42,Y+H*CellSize-24,FString::Printf(TEXT("%dx%d"),W,H),11,true,FLinearColor(.5f,.59f,.58f,1));
    }
    if(bPreview)
    {
        const int32 X=FMath::Clamp(PreviewX,0,Columns),Y=FMath::Clamp(PreviewY,0,Rows);
        const int32 EndX=FMath::Clamp(PreviewX+PreviewW,0,Columns),EndY=FMath::Clamp(PreviewY+PreviewH,0,Rows);
        if(EndX>X&&EndY>Y)Box(X*CellSize,Y*CellSize,(EndX-X)*CellSize-1,(EndY-Y)*CellSize-1,bFits?FLinearColor(.4f,.66f,.29f,.38f):FLinearColor(.8f,.12f,.08f,.5f),L+5);
    }
    return L+5;
}
void UFPSExtractionGridUI::NativeDestruct()
{
    OwnerUI.Reset();
    if(SplitDialog)SplitDialog->Close();SplitDialog=nullptr;
    Super::NativeDestruct();
}
