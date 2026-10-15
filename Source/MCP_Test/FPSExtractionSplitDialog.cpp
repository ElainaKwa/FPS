#include "FPSExtractionGridUI.h"
#include "FPSExtractionUI.h"
#include "FPSExtractionSubsystem.h"
#include "FPSInterface.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/SpinBox.h"
#include "Components/SizeBox.h"
#include "Engine/GameInstance.h"
#include "Engine/Font.h"
#include "InputCoreTypes.h"
#include "Brushes/SlateColorBrush.h"

void UFPSExtractionSplitDialog::OpenFor(UFPSExtractionGridUI* Grid,FGuid Id,int32 Quantity)
{
    Source=Grid;StackId=Id;OriginalQuantity=Quantity;
    Phase=GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>()->GetPhase();
    SetIsFocusable(true);AddToPlayerScreen(150);SetKeyboardFocus();
}
TSharedRef<SWidget> UFPSExtractionSplitDialog::RebuildWidget()
{
    auto* Grid=Source.Get();if(!Grid)return Super::RebuildWidget();
    auto* UI=Grid->OwnerUI.Get();const auto* Theme=UI?UI->Theme.Get():nullptr;
    auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("SplitRoot"));WidgetTree->RootWidget=Root;
    auto* Shade=WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("SplitDismiss"));
    Shade->SetBackgroundColor(FLinearColor(0,0,0,.75f));
    if(Theme)Shade->SetStyle(Theme->DarkButton);
    Shade->OnClicked.AddUniqueDynamic(this,&UFPSExtractionSplitDialog::Cancel);
    auto* ShadeSlot=Root->AddChildToCanvas(Shade);ShadeSlot->SetAnchors(FAnchors(0,0,1,1));ShadeSlot->SetOffsets(FMargin(0));
    auto* Panel=WidgetTree->ConstructWidget<UBorder>();Panel->SetBrushColor(FLinearColor(.035f,.047f,.05f,1));Panel->SetPadding(FMargin(24));
    auto* PanelSlot=Root->AddChildToCanvas(Panel);PanelSlot->SetAnchors(FAnchors(.5f,.5f));PanelSlot->SetAlignment(FVector2D(.5f,.5f));PanelSlot->SetPosition(FVector2D::ZeroVector);PanelSlot->SetSize(FVector2D(440,260));
    auto* Content=WidgetTree->ConstructWidget<UVerticalBox>();Panel->AddChild(Content);
    auto Label=[&](const FString& Value,int32 Size)
    {
        auto* Text=WidgetTree->ConstructWidget<UTextBlock>();Text->SetText(FText::FromString(Value));
        Text->SetFont(FSlateFontInfo(Grid->TextFont.Get(),Size));Text->SetColorAndOpacity(FLinearColor(.86f,.89f,.87f));
        Content->AddChildToVerticalBox(Text)->SetPadding(FMargin(0,0,0,12));return Text;
    };
    Label(TEXT("拆分物资"),22);
    Label(FString::Printf(TEXT("拆出数量  1–%d  ·  放入当前容器空位"),OriginalQuantity-1),14);
    QuantityInput=WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(),TEXT("SplitQuantity"));
    auto InputStyle=QuantityInput->GetWidgetStyle();
    const FSlateColorBrush InputBackground(FLinearColor(.012f,.02f,.024f));
    const FSlateColorBrush InputFill(FLinearColor(.07f,.12f,.13f));
    InputStyle.SetBackgroundBrush(InputBackground).SetHoveredBackgroundBrush(InputBackground).SetActiveBackgroundBrush(InputBackground)
        .SetInactiveFillBrush(InputFill).SetHoveredFillBrush(InputFill).SetActiveFillBrush(InputFill).SetTextPadding(FMargin(8,4));
    QuantityInput->SetWidgetStyle(InputStyle);QuantityInput->SetForegroundColor(FLinearColor(.92f,.94f,.9f));
    QuantityInput->SetJustification(ETextJustify::Center);QuantityInput->SetAlwaysUsesDeltaSnap(true);
    QuantityInput->SetMinValue(1);QuantityInput->SetMaxValue(OriginalQuantity-1);
    QuantityInput->SetMinSliderValue(1);QuantityInput->SetMaxSliderValue(OriginalQuantity-1);
    QuantityInput->SetDelta(1);QuantityInput->SetMinFractionalDigits(0);QuantityInput->SetMaxFractionalDigits(0);
    QuantityInput->SetFont(FSlateFontInfo(Grid->NumberFont.Get(),24));QuantityInput->SetValue(FMath::Max(1,OriginalQuantity/2));
    Content->AddChildToVerticalBox(QuantityInput)->SetPadding(FMargin(0,0,0,20));
    auto* Buttons=WidgetTree->ConstructWidget<UHorizontalBox>();Content->AddChildToVerticalBox(Buttons);
    auto Button=[&](FName Name,const TCHAR* Text,bool Accent)
    {
        auto* B=WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),Name);if(Theme)B->SetStyle(Accent?Theme->AccentButton:Theme->DarkButton);
        auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Text));T->SetFont(FSlateFontInfo(Grid->TextFont.Get(),16));
        T->SetColorAndOpacity(Accent?FLinearColor(.04f,.045f,.035f):FLinearColor(.86f,.89f,.87f));B->AddChild(T);
        auto* Box=WidgetTree->ConstructWidget<USizeBox>();Box->SetHeightOverride(44);Box->AddChild(B);
        auto* Slot=Buttons->AddChildToHorizontalBox(Box);Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Slot->SetPadding(FMargin(4,0));return B;
    };
    Button(TEXT("SplitCancel"),TEXT("取消 / Esc"),false)->OnClicked.AddUniqueDynamic(this,&UFPSExtractionSplitDialog::Cancel);
    Button(TEXT("SplitConfirm"),TEXT("确认拆分"),true)->OnClicked.AddUniqueDynamic(this,&UFPSExtractionSplitDialog::Confirm);
    return Super::RebuildWidget();
}
void UFPSExtractionSplitDialog::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);
    auto* Grid=Source.Get();auto* UI=Grid?Grid->OwnerUI.Get():nullptr;
    auto* Model=GetGameInstance()?GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>():nullptr;
    if(!Grid || !UI || !UI->IsPanelOpen() || !Model || Model->GetPhase()!=Phase){Close();return;}
    const auto View=Model->GetView();const auto& Inventory=Grid->Container==EFPSExtractionContainer::Warehouse?View.Warehouse:View.Backpack;
    const auto* Stack=Inventory.FindByPredicate([this](const auto& S){return S.StackId==StackId;});
    if(!Stack || Stack->Quantity!=OriginalQuantity)Close();
}
void UFPSExtractionSplitDialog::Confirm()
{
    auto* Grid=Source.Get();auto* Model=GetGameInstance()?GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>():nullptr;
    if(Grid && Model && QuantityInput)
    {
        const auto View=Model->GetView();const auto& Inventory=Grid->Container==EFPSExtractionContainer::Warehouse?View.Warehouse:View.Backpack;
        const auto* Stack=Inventory.FindByPredicate([this](const auto& S){return S.StackId==StackId;});
        if(Stack && Stack->Quantity==OriginalQuantity && View.Phase==Phase)
            Model->SplitStack(Grid->Container,StackId,FMath::RoundToInt(QuantityInput->GetValue()));
    }
    Close();
}
void UFPSExtractionSplitDialog::Cancel(){Close();}
void UFPSExtractionSplitDialog::Close()
{
    RemoveFromParent();
    if(auto* Grid=Source.Get())
    {
        if(Grid->SplitDialog==this)Grid->SplitDialog=nullptr;
        if(auto* UI=Grid->OwnerUI.Get()){UI->Refresh();if(UI->IsPanelOpen())UI->SetKeyboardFocus();}
    }
    Source.Reset();
}
FReply UFPSExtractionSplitDialog::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(Event.GetKey()==EKeys::Escape){Close();return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
