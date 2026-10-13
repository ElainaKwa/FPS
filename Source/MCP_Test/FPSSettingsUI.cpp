#include "FPSSettingsUI.h"
#include "FPSInterface.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"

namespace SettingsUI
{
const TCHAR* Root=TEXT("/Game/FPSDemo/Art/UI/SystemSettings/");
const TCHAR* Pages[]={TEXT("画面"),TEXT("声音"),TEXT("控制"),TEXT("按键"),TEXT("界面与舒适性")};
const TCHAR* PageActions[]={TEXT("PageGraphics"),TEXT("PageSound"),TEXT("PageControls"),TEXT("PageBindings"),TEXT("PageHUD")};
const TCHAR* ModalButtons[]={TEXT("ExitApply"),TEXT("ExitDiscard"),TEXT("ExitContinue"),TEXT("DisplayConfirm"),TEXT("DisplayRevert"),
    TEXT("ResetConfirm"),TEXT("ModalCancel"),TEXT("CaptureClear"),TEXT("CaptureCancel"),TEXT("ConflictReplace"),TEXT("ConflictCancel")};
template<class T>T* Find(const UUserWidget* UI,FName Name){return UI&&UI->WidgetTree?Cast<T>(UI->WidgetTree->FindWidget(Name)):nullptr;}
void Text(UUserWidget* UI,FName Name,const FString& Value)
{if(auto* T=Find<UTextBlock>(UI,Name))if(T->GetText().ToString()!=Value)T->SetText(FText::FromString(Value));}
void Show(UUserWidget* UI,FName Name,bool Visible)
{if(auto* W=Find<UWidget>(UI,Name))W->SetVisibility(Visible?(Cast<UButton>(W)?ESlateVisibility::Visible:ESlateVisibility::SelfHitTestInvisible):ESlateVisibility::Collapsed);}
void Enable(UUserWidget* UI,FName Name,bool Enabled){if(auto* W=Find<UWidget>(UI,Name))W->SetIsEnabled(Enabled);}
FString ClassPath(const TCHAR* Name){return FString(Root)+Name+TEXT(".")+Name+TEXT("_C");}
}

void UFPSSettingsButton::Bind(UFPSSettingsUI* Owner,UFPSSettingsRow* Row)
{OwnerUI=Owner;OwnerRow=Row;OnClicked.AddUniqueDynamic(this,&UFPSSettingsButton::Activate);}
void UFPSSettingsButton::Activate()
{if(OwnerRow.IsValid())OwnerRow->Activate();else if(OwnerUI.IsValid())OwnerUI->Dispatch(Action);}

void UFPSSettingsRow::NativeConstruct()
{
    Super::NativeConstruct();
    if(auto* S=SettingsUI::Find<USlider>(this,TEXT("OptionSlider")))
    {S->OnValueChanged.AddUniqueDynamic(this,&UFPSSettingsRow::SliderChanged);S->OnMouseCaptureBegin.AddUniqueDynamic(this,&UFPSSettingsRow::SliderBegin);S->OnMouseCaptureEnd.AddUniqueDynamic(this,&UFPSSettingsRow::SliderEnd);}
    if(auto* C=SettingsUI::Find<UComboBoxString>(this,TEXT("OptionChoice")))
    {C->OnSelectionChanged.AddUniqueDynamic(this,&UFPSSettingsRow::ChoiceChanged);C->OnGenerateWidgetEvent.BindDynamic(this,&UFPSSettingsRow::MakeChoice);}
}
UWidget* UFPSSettingsRow::MakeChoice(FString Choice)
{
    auto* T=NewObject<UTextBlock>(this);T->SetText(FText::FromString(Choice));
    T->SetFont(FSlateFontInfo(TextFont.Get(),20,TEXT("Regular")));T->SetColorAndOpacity(FLinearColor(.84f,.86f,.82f,1));
    T->SetVisibility(ESlateVisibility::HitTestInvisible);return T;
}
bool UFPSSettingsRow::IsInteracting() const
{const auto* C=SettingsUI::Find<UComboBoxString>(this,TEXT("OptionChoice"));return bSliding||(C&&C->IsOpen());}
void UFPSSettingsRow::Present(UFPSSettingsUI* Owner,const FFPSSettingOption& Option)
{
    OwnerUI=Owner;OptionId=Option.Id;Kind=Option.Kind;Minimum=Option.Min;Maximum=Option.Max;Step=Option.Step;bUpdating=true;
    SettingsUI::Text(this,TEXT("OptionLabel"),Option.Label);SettingsUI::Text(this,TEXT("OptionDescription"),Option.Description);
    FString ValueText=Option.ValueText;
    if(ValueText.IsEmpty()&&Kind==EFPSSettingKind::Slider)ValueText=Step>=1?FString::Printf(TEXT("%.0f"),Option.Value):FString::Printf(TEXT("%.2f"),Option.Value);
    if(ValueText.IsEmpty()&&Kind==EFPSSettingKind::Choice&&Option.Choices.IsValidIndex(FMath::RoundToInt32(Option.Value)))ValueText=Option.Choices[FMath::RoundToInt32(Option.Value)];
    SettingsUI::Text(this,TEXT("OptionValue"),ValueText);SetToolTipText(FText::FromString(Option.Description));
    if(auto* W=SettingsUI::Find<UWidget>(this,TEXT("OptionSlider")))W->SetVisibility(Kind==EFPSSettingKind::Slider?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    if(auto* W=SettingsUI::Find<UWidget>(this,TEXT("OptionChoice")))W->SetVisibility(Kind==EFPSSettingKind::Choice?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    if(auto* W=SettingsUI::Find<UWidget>(this,TEXT("OptionAction")))W->SetVisibility(Kind==EFPSSettingKind::Binding||Kind==EFPSSettingKind::Action?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    if(Kind==EFPSSettingKind::Slider)
    {
        if(auto* S=SettingsUI::Find<USlider>(this,TEXT("OptionSlider")))
        {S->SetVisibility(ESlateVisibility::Visible);if(!bSliding){S->SetMinValue(Minimum);S->SetMaxValue(Maximum);S->SetStepSize(Step);S->SetValue(Option.Value);}}
    }
    else if(Kind==EFPSSettingKind::Choice)
    {
        if(auto* C=SettingsUI::Find<UComboBoxString>(this,TEXT("OptionChoice")))
        {
            C->SetVisibility(ESlateVisibility::Visible);
            if(!C->IsOpen())
            {
                if(Choices!=Option.Choices){Choices=Option.Choices;C->ClearOptions();for(const auto& Choice:Choices)C->AddOption(Choice);}
                const int32 Index=FMath::RoundToInt32(Option.Value);
                if(Choices.IsValidIndex(Index)&&C->GetSelectedIndex()!=Index)C->SetSelectedIndex(Index);
                else if(!Choices.IsValidIndex(Index))C->ClearSelection();
            }
        }
    }
    else if(auto* B=SettingsUI::Find<UFPSSettingsButton>(this,TEXT("OptionAction")))
    {
        B->Bind(Owner,this);B->SetVisibility(ESlateVisibility::Visible);
        SettingsUI::Text(this,TEXT("OptionActionLabel"),Kind==EFPSSettingKind::Binding?(Option.ValueText.IsEmpty()?TEXT("未绑定 / 点击设置"):Option.ValueText):TEXT("试听"));
    }
    SettingsUI::Show(this,TEXT("OptionValue"),Kind==EFPSSettingKind::Slider||Kind==EFPSSettingKind::Choice);
    bUpdating=false;
}
void UFPSSettingsRow::SliderBegin(){bSliding=true;}
void UFPSSettingsRow::SliderEnd(){bSliding=false;if(OwnerUI.IsValid())OwnerUI->Refresh();}
void UFPSSettingsRow::SliderChanged(float Value)
{
    if(bUpdating||!OwnerUI.IsValid())return;
    const double Snapped=Step>0?Minimum+FMath::RoundToDouble((Value-Minimum)/Step)*Step:Value;
    OwnerUI->SetOptionValue(OptionId,FMath::Clamp(Snapped,Minimum,Maximum));
}
void UFPSSettingsRow::ChoiceChanged(FString Choice,ESelectInfo::Type Selection)
{if(!bUpdating&&OwnerUI.IsValid()){const int32 Index=Choices.IndexOfByKey(Choice);if(Index!=INDEX_NONE)OwnerUI->SetOptionValue(OptionId,Index);}}
void UFPSSettingsRow::Activate(){if(OwnerUI.IsValid())OwnerUI->ActivateOption(OptionId,Kind);}

UFPSSettingsUI* UFPSSettingsUI::Open(APlayerController* Owner)
{
    if(!Owner)return nullptr;auto* Class=LoadClass<UFPSSettingsUI>(nullptr,*SettingsUI::ClassPath(TEXT("WBP_SystemSettings")));
    if(!Class){UE_LOG(LogTemp,Error,TEXT("Settings UI missing; run build_system_settings_ui.py after compilation."));return nullptr;}
    auto* UI=CreateWidget<UFPSSettingsUI>(Owner,Class);if(UI)UI->AddToPlayerScreen(100);return UI;
}
void UFPSSettingsUI::NativeConstruct()
{
    Super::NativeConstruct();SetIsFocusable(true);
    WidgetTree->ForEachWidget([this](UWidget* W){if(auto* B=Cast<UFPSSettingsButton>(W))B->Bind(this);});
    Refresh();SetKeyboardFocus();
}
void UFPSSettingsUI::NativeDestruct()
{
    if(auto* S=UFPSSystemSettings::Get(this)){S->CancelCapture();if(S->IsConfirmingDisplay())S->RevertDisplay();}
    bReturnAfterDisplay=false;Super::NativeDestruct();
}
void UFPSSettingsUI::NativeTick(const FGeometry& G,float DeltaTime)
{Super::NativeTick(G,DeltaTime);Clock+=DeltaTime;if(Clock>=.1f){Clock=0;Refresh();}}
UFPSSettingsUI::EDialog UFPSSettingsUI::Dialog() const
{
    const auto* S=UFPSSystemSettings::Get(this);if(!S)return EDialog::None;
    if(S->IsConfirmingDisplay())return EDialog::Display;
    if(S->HasBindingConflict())return EDialog::Conflict;
    if(S->IsCapturing())return EDialog::Capture;
    if(bResetPrompt)return EDialog::Reset;
    return bExitPrompt?EDialog::Exit:EDialog::None;
}
void UFPSSettingsUI::Close()
{if(bClosing)return;bClosing=true;RemoveFromParent();OnClosed.ExecuteIfBound();}
void UFPSSettingsUI::RequestBack()
{
    auto* S=UFPSSystemSettings::Get(this);if(!S){Close();return;}
    if(S->IsCapturing()||S->HasBindingConflict()){S->CancelCapture();LocalFeedback.Empty();Refresh();return;}
    if(S->IsConfirmingDisplay()){S->RevertDisplay();bReturnAfterDisplay=false;Refresh();return;}
    if(bResetPrompt||bExitPrompt){bResetPrompt=false;bExitPrompt=false;Refresh();return;}
    if(S->HasPendingGraphics()){bExitPrompt=true;Refresh();return;}Close();
}
void UFPSSettingsUI::Apply(bool ReturnAfter)
{
    auto* S=UFPSSystemSettings::Get(this);if(!S)return;bExitPrompt=false;bReturnAfterDisplay=false;LocalFeedback.Empty();
    if(S->ApplyGraphics())
    {
        if(S->IsConfirmingDisplay())bReturnAfterDisplay=ReturnAfter;
        else if(ReturnAfter&&S->LastSaveSucceeded()){Close();return;}
    }
    Refresh();
}
void UFPSSettingsUI::Dispatch(FName Action)
{
    if(bClosing)return;auto* S=UFPSSystemSettings::Get(this);if(!S)return;
    if(Action==TEXT("Back")){RequestBack();return;}
    const EDialog D=Dialog();LocalFeedback.Empty();
    if(D==EDialog::Exit)
    {
        if(Action==TEXT("ExitApply")){Apply(true);return;}
        if(Action==TEXT("ExitDiscard")){S->DiscardGraphics();bExitPrompt=false;Close();return;}
        if(Action==TEXT("ExitContinue"))bExitPrompt=false;
    }
    else if(D==EDialog::Display)
    {
        if(Action==TEXT("DisplayConfirm"))
        {const bool Return=bReturnAfterDisplay;S->ConfirmDisplay();bReturnAfterDisplay=false;if(Return&&!S->IsConfirmingDisplay()&&S->LastSaveSucceeded()){Close();return;}}
        else if(Action==TEXT("DisplayRevert")){S->RevertDisplay();bReturnAfterDisplay=false;}
    }
    else if(D==EDialog::Reset)
    {if(Action==TEXT("ResetConfirm")){S->ResetPage(Page);bResetPrompt=false;}else if(Action==TEXT("ModalCancel"))bResetPrompt=false;}
    else if(D==EDialog::Capture)
    {if(Action==TEXT("CaptureClear"))S->ClearCapturedBinding();else if(Action==TEXT("CaptureCancel"))S->CancelCapture();}
    else if(D==EDialog::Conflict)
    {if(Action==TEXT("ConflictReplace"))S->ConfirmBindingReplacement();else if(Action==TEXT("ConflictCancel"))S->CancelCapture();}
    else
    {
        for(int32 I=0;I<5;++I)if(Action==SettingsUI::PageActions[I]){Page=I;Refresh();return;}
        if(Action==TEXT("ApplyGraphics")){Apply(false);return;}
        if(Action==TEXT("DiscardGraphics"))S->DiscardGraphics();
        else if(Action==TEXT("ResetPage"))bResetPrompt=true;
    }
    Refresh();
}
void UFPSSettingsUI::SetOptionValue(FName Id,double Value)
{if(Dialog()!=EDialog::None)return;if(auto* S=UFPSSystemSettings::Get(this)){LocalFeedback.Empty();S->SetValue(Id,Value);Refresh();}}
void UFPSSettingsUI::ActivateOption(FName Id,EFPSSettingKind Kind)
{
    if(Dialog()!=EDialog::None)return;if(auto* S=UFPSSystemSettings::Get(this))
    {LocalFeedback.Empty();if(Kind==EFPSSettingKind::Binding){PendingModifier=FKey();bModifierChord=false;S->BeginCapture(Id);SetKeyboardFocus();}else if(Kind==EFPSSettingKind::Action)S->PreviewSound();Refresh();}
}
UFPSSettingsRow* UFPSSettingsUI::FindOptionRow(FName Id) const
{for(const auto& Row:Rows)if(Row&&Row->GetOptionId()==Id)return Row.Get();return nullptr;}
void UFPSSettingsUI::RefreshRows()
{
    auto* S=UFPSSystemSettings::Get(this);auto* List=SettingsUI::Find<UScrollBox>(this,TEXT("SettingsRows"));if(!S||!List)return;
    const auto Options=S->Options(Page);bool Rebuild=RowsPage!=Page||Rows.Num()!=Options.Num();
    for(int32 I=0;!Rebuild&&I<Rows.Num();++I)Rebuild=Rows[I]->GetOptionId()!=Options[I].Id;
    if(Rebuild)
    {
        if(Rows.ContainsByPredicate([](const auto& Row){return Row&&Row->IsInteracting();})||S->IsCapturing())return;
        auto* Class=LoadClass<UFPSSettingsRow>(nullptr,*SettingsUI::ClassPath(TEXT("WBP_SystemSettingsRow")));if(!Class)return;
        List->ClearChildren();Rows.Empty();RowsPage=Page;List->SetScrollOffset(0);
        for(const auto& Option:Options)
        {auto* Row=CreateWidget<UFPSSettingsRow>(GetOwningPlayer(),Class);if(!Row)continue;
        if(auto* RowSlot=Cast<UScrollBoxSlot>(List->AddChild(Row)))RowSlot->SetPadding(FMargin(0,0,0,10));Row->Present(this,Option);Rows.Add(Row);}
    }
    else for(int32 I=0;I<Rows.Num();++I)Rows[I]->Present(this,Options[I]);
}
void UFPSSettingsUI::RefreshDialog()
{
    auto* S=UFPSSystemSettings::Get(this);if(!S)return;const EDialog D=Dialog();
    SettingsUI::Show(this,TEXT("SettingsModal"),D!=EDialog::None);SettingsUI::Enable(this,TEXT("SettingsContent"),D==EDialog::None);
    TSet<FName> VisibleButtons;
    auto Buttons=[&](const TCHAR* A,const TCHAR* B,const TCHAR* C=nullptr)
    {VisibleButtons.Add(A);VisibleButtons.Add(B);if(C)VisibleButtons.Add(C);};
    FString Title,Body;
    switch(D)
    {
    case EDialog::Exit:Title=TEXT("画面设置尚未应用");Body=TEXT("应用后返回来源页面，或仅放弃画面修改。\n声音、控制、按键和界面设置已即时生效。");Buttons(TEXT("ExitApply"),TEXT("ExitDiscard"),TEXT("ExitContinue"));break;
    case EDialog::Reset:Title=TEXT("恢复本页默认？");Body=FString::Printf(TEXT("将恢复「%s」分类的默认值。\n%s"),SettingsUI::Pages[Page],Page==0?TEXT("画面修改仍需点击应用。"):TEXT("此操作会即时生效并保存。"));Buttons(TEXT("ResetConfirm"),TEXT("ModalCancel"));break;
    case EDialog::Display:Title=TEXT("保留当前显示设置？");Body=FString::Printf(TEXT("请确认画面可正常显示。\n%d 秒后自动恢复此前显示设置。"),S->DisplaySecondsLeft());Buttons(TEXT("DisplayConfirm"),TEXT("DisplayRevert"));break;
    case EDialog::Capture:Title=TEXT("设置按键");Body=S->CapturePrompt();Buttons(TEXT("CaptureClear"),TEXT("CaptureCancel"));break;
    case EDialog::Conflict:Title=TEXT("按键已被使用");Body=S->CapturePrompt();Buttons(TEXT("ConflictReplace"),TEXT("ConflictCancel"));break;
    default:break;
    }
    for(const TCHAR* Name:SettingsUI::ModalButtons)SettingsUI::Show(this,Name,VisibleButtons.Contains(Name));
    SettingsUI::Text(this,TEXT("ModalTitle"),Title);SettingsUI::Text(this,TEXT("ModalBody"),Body);
    SettingsUI::Text(this,TEXT("ModalFeedback"),LocalFeedback.IsEmpty()&&(D==EDialog::Capture||D==EDialog::Conflict)?S->Feedback:LocalFeedback);
}
void UFPSSettingsUI::Refresh()
{
    if(bClosing||!WidgetTree)return;auto* S=UFPSSystemSettings::Get(this);if(!S)return;
    if(!S->IsCapturing()){PendingModifier=FKey();bModifierChord=false;}
    const bool Confirming=S->IsConfirmingDisplay();if(bWasConfirming&&!Confirming)bReturnAfterDisplay=false;bWasConfirming=Confirming;
    SettingsUI::Text(this,TEXT("SettingsPageTitle"),SettingsUI::Pages[Page]);
    SettingsUI::Text(this,TEXT("SettingsPageNote"),Page==0?TEXT("修改后点击应用；切换分类保留未应用内容。"):TEXT("修改即时生效并自动保存。"));
    SettingsUI::Text(this,TEXT("SettingsFeedback"),LocalFeedback.IsEmpty()?S->Feedback:LocalFeedback);
    if(auto* T=SettingsUI::Find<UTextBlock>(this,TEXT("SettingsFeedback")))T->SetColorAndOpacity(Theme?(S->LastSaveSucceeded()?Theme->Paper:Theme->Red):FLinearColor::White);
    SettingsUI::Text(this,TEXT("GraphicsStatus"),S->HasPendingGraphics()?TEXT("画面 · 有未应用修改"):TEXT("画面 · 无待应用修改"));
    SettingsUI::Enable(this,TEXT("ApplyGraphics"),S->HasPendingGraphics());SettingsUI::Enable(this,TEXT("DiscardGraphics"),S->HasPendingGraphics());
    for(int32 I=0;I<5;++I)if(auto* B=SettingsUI::Find<UFPSSettingsButton>(this,SettingsUI::PageActions[I]))if(Theme)
    {B->SetStyle(Page==I?Theme->LightButton:Theme->DarkButton);if(auto* T=SettingsUI::Find<UTextBlock>(this,FName(*(FString(SettingsUI::PageActions[I])+TEXT("Label")))))T->SetColorAndOpacity(Page==I?Theme->Ink:Theme->Paper);}
    SettingsUI::Show(this,TEXT("HUDPreview"),Page==4);
    if(auto* List=SettingsUI::Find<UScrollBox>(this,TEXT("SettingsRows")))if(auto* ListSlot=Cast<UCanvasPanelSlot>(List->Slot))ListSlot->SetSize(FVector2D(1198,Page==4?290:525));
    if(auto* W=SettingsUI::Find<UWidget>(this,TEXT("HUDPreviewContent")))W->SetRenderScale(FVector2D(S->HUDScale,S->HUDScale));
    SettingsUI::Text(this,TEXT("HUDPreviewCaption"),FString::Printf(TEXT("HUD 缩放示意  %.0f%%"),S->HUDScale*100));
    RefreshRows();RefreshDialog();
}
void UFPSSettingsUI::Capture(FKey Key,bool Modified)
{
    auto* S=UFPSSystemSettings::Get(this);if(!S||!S->IsCapturing()||S->HasBindingConflict())return;
    ConsumedCaptureKey=Key;
    if(Modified||Key.IsGamepadKey()){bModifierChord|=PendingModifier.IsValid();LocalFeedback=TEXT("仅支持单个键盘或鼠标按键，不支持组合键和手柄。");}
    else{LocalFeedback.Empty();S->CaptureKey(Key);}Refresh();
}
FReply UFPSSettingsUI::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E)
{
    if(E.GetKey()==EKeys::Escape){if(!E.IsRepeat())RequestBack();return FReply::Handled();}
    if(Dialog()==EDialog::Capture)
    {
        if(!E.IsRepeat())
        {
            const FKey Key=E.GetKey();
            if(Key.IsModifierKey())
            {
                if(PendingModifier.IsValid())bModifierChord=true;
                else
                {
                    PendingModifier=Key;
                    bModifierChord=(E.IsControlDown()&&Key!=EKeys::LeftControl&&Key!=EKeys::RightControl)||
                        (E.IsShiftDown()&&Key!=EKeys::LeftShift&&Key!=EKeys::RightShift)||
                        (E.IsAltDown()&&Key!=EKeys::LeftAlt&&Key!=EKeys::RightAlt)||E.IsCommandDown();
                }
                LocalFeedback=TEXT("单独松开此键可完成绑定；不支持与其他键组合。");Refresh();
            }
            else Capture(Key,E.IsControlDown()||E.IsAltDown()||E.IsShiftDown()||E.IsCommandDown()||PendingModifier.IsValid());
        }
        return FReply::Handled();
    }
    if(Dialog()!=EDialog::None&&E.GetKey().IsGamepadKey())return FReply::Handled();
    return Super::NativeOnPreviewKeyDown(G,E);
}
FReply UFPSSettingsUI::NativeOnKeyUp(const FGeometry& G,const FKeyEvent& E)
{
    if(Dialog()==EDialog::Capture&&E.GetKey()==PendingModifier)
    {
        const FKey Key=PendingModifier;const bool Chord=bModifierChord;PendingModifier=FKey();bModifierChord=false;
        if(!Chord)Capture(Key,false);else{LocalFeedback=TEXT("未设置组合键。请松开所有按键后重新输入。");Refresh();}
        ConsumedCaptureKey=FKey();return FReply::Handled();
    }
    if(E.GetKey()==ConsumedCaptureKey){ConsumedCaptureKey=FKey();return FReply::Handled();}
    if(Dialog()==EDialog::Capture)return FReply::Handled();return Super::NativeOnKeyUp(G,E);
}
bool UFPSSettingsUI::IsDialogButtonAt(const FVector2D& Position) const
{
    for(const TCHAR* Name:SettingsUI::ModalButtons)if(auto* B=SettingsUI::Find<UButton>(this,Name))
        if(B->GetVisibility()!=ESlateVisibility::Collapsed&&B->GetCachedGeometry().IsUnderLocation(Position))return true;
    return false;
}
FReply UFPSSettingsUI::NativeOnPreviewMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{
    if(Dialog()==EDialog::Capture&&!IsDialogButtonAt(E.GetScreenSpacePosition()))
    {Capture(E.GetEffectingButton(),E.IsControlDown()||E.IsAltDown()||E.IsShiftDown()||E.IsCommandDown());return FReply::Handled();}
    return Super::NativeOnPreviewMouseButtonDown(G,E);
}
FReply UFPSSettingsUI::NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E)
{if(E.GetEffectingButton()==ConsumedCaptureKey){ConsumedCaptureKey=FKey();return FReply::Handled();}return Super::NativeOnMouseButtonUp(G,E);}
FReply UFPSSettingsUI::NativeOnMouseWheel(const FGeometry& G,const FPointerEvent& E)
{
    if(Dialog()==EDialog::Capture)
    {Capture(E.GetWheelDelta()>0?EKeys::MouseScrollUp:EKeys::MouseScrollDown,E.IsControlDown()||E.IsAltDown()||E.IsShiftDown()||E.IsCommandDown());return FReply::Handled();}
    if(Dialog()!=EDialog::None)return FReply::Handled();return Super::NativeOnMouseWheel(G,E);
}

#if WITH_EDITOR
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace SettingsUI
{
struct FBuilder
{
    UWidgetBlueprint* BP=nullptr;UWidgetTree* Tree=nullptr;UCanvasPanel* Canvas=nullptr;
    UFPSUITheme* Theme=nullptr;UFont* Font=nullptr;UFont* NumberFont=nullptr;
    FBuilder(const TCHAR* Name,bool Row)
    {
        Theme=LoadObject<UFPSUITheme>(nullptr,TEXT("/Game/FPSDemo/Art/Industrial/UI/DA_FPSUITheme.DA_FPSUITheme"));
        Font=LoadObject<UFont>(nullptr,TEXT("/Game/FPSDemo/Art/Industrial/UI/Fonts/F_UI_Text.F_UI_Text"));
        NumberFont=LoadObject<UFont>(nullptr,TEXT("/Game/FPSDemo/Art/Industrial/UI/Fonts/F_UI_Number.F_UI_Number"));
        if(!Theme||!Font||!NumberFont)return;
        const FString Path=FString(Root)+Name;
        auto* Factory=NewObject<UWidgetBlueprintFactory>();Factory->ParentClass=Row?UFPSSettingsRow::StaticClass():UFPSSettingsUI::StaticClass();
        BP=Cast<UWidgetBlueprint>(Factory->FactoryCreateNew(UWidgetBlueprint::StaticClass(),CreatePackage(*Path),Name,RF_Public|RF_Standalone,nullptr,GWarn));
        if(!BP)return;Tree=BP->WidgetTree;
        if(!Tree){Tree=NewObject<UWidgetTree>(BP,TEXT("WidgetTree"),RF_Transactional);BP->WidgetTree=Tree;}
        auto* Size=Make<USizeBox>(TEXT("DesignSize"));Size->SetWidthOverride(Row?1176:1600);Size->SetHeightOverride(Row?142:900);
        Canvas=Make<UCanvasPanel>(TEXT("SettingsLayout"));Size->AddChild(Canvas);
        if(Row)Tree->RootWidget=Size;
        else{auto* Fit=Make<UScaleBox>(TEXT("ViewportFit"));Fit->SetStretch(EStretch::ScaleToFit);Fit->AddChild(Size);Tree->RootWidget=Fit;}
        FAssetRegistryModule::AssetCreated(BP);
    }
    template<class T>T* Make(const TCHAR* Name){return Tree->ConstructWidget<T>(T::StaticClass(),Name);}
    void Place(UWidget* W,float X,float Y,float Width,float Height,UCanvasPanel* Parent=nullptr)
    {auto* S=(Parent?Parent:Canvas)->AddChildToCanvas(W);S->SetPosition(FVector2D(X,Y));S->SetSize(FVector2D(Width,Height));}
    UCanvasPanel* Group(const TCHAR* Name,float X,float Y,float Width,float Height,UCanvasPanel* Parent=nullptr)
    {auto* G=Make<UCanvasPanel>(Name);G->SetVisibility(ESlateVisibility::SelfHitTestInvisible);Place(G,X,Y,Width,Height,Parent);return G;}
    UImage* Surface(const TCHAR* Name,float X,float Y,float Width,float Height,FLinearColor Color,UCanvasPanel* Parent=nullptr,bool Rounded=false)
    {auto* I=Make<UImage>(Name);if(Rounded)I->SetBrush(FSlateRoundedBoxBrush(Color,5.f));else I->SetBrush(FSlateColorBrush(Color));I->SetVisibility(ESlateVisibility::HitTestInvisible);Place(I,X,Y,Width,Height,Parent);return I;}
    UTextBlock* Label(const TCHAR* Name,const FString& Value,float X,float Y,float Width,float Height,int32 Size,UCanvasPanel* Parent=nullptr,bool Bold=false,bool Numeric=false)
    {auto* T=Make<UTextBlock>(Name);T->SetText(FText::FromString(Value));T->SetFont(FSlateFontInfo(Numeric?NumberFont:Font,Size,Bold?FName(TEXT("Bold")):FName(TEXT("Regular"))));
    T->SetColorAndOpacity(Theme->Paper);T->SetAutoWrapText(true);T->SetVisibility(ESlateVisibility::HitTestInvisible);Place(T,X,Y,Width,Height,Parent);return T;}
    UFPSSettingsButton* Button(const TCHAR* Name,const FString& Value,float X,float Y,float Width,float Height,UCanvasPanel* Parent=nullptr,bool Accent=false)
    {
        auto* B=Make<UFPSSettingsButton>(Name);B->Action=Name;B->SetStyle(Accent?Theme->AccentButton:Theme->DarkButton);
        auto* T=Make<UTextBlock>(*(FString(Name)+TEXT("Label")));T->SetText(FText::FromString(Value));T->SetFont(FSlateFontInfo(Font,21,TEXT("Bold")));
        T->SetColorAndOpacity(Accent?Theme->Ink:Theme->Paper);T->SetJustification(ETextJustify::Center);T->SetVisibility(ESlateVisibility::HitTestInvisible);B->AddChild(T);
        auto* S=CastChecked<UButtonSlot>(T->Slot);S->SetHorizontalAlignment(HAlign_Center);S->SetVerticalAlignment(VAlign_Center);S->SetPadding(FMargin(10,0,10,4));
        Place(B,X,Y,Width,Height,Parent);return B;
    }
    bool Save()
    {
        Tree->ForEachWidget([&](UWidget* W){if(!BP->WidgetVariableNameToGuidMap.Contains(W->GetFName()))BP->OnVariableAdded(W->GetFName());});
        FKismetEditorUtilities::CompileBlueprint(BP);if(BP->Status==BS_Error||!BP->GeneratedClass)return false;
        if(auto* UI=Cast<UFPSSettingsUI>(BP->GeneratedClass->GetDefaultObject()))UI->Theme=Theme;
        if(auto* Row=Cast<UFPSSettingsRow>(BP->GeneratedClass->GetDefaultObject()))Row->TextFont=Font;
        BP->MarkPackageDirty();FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
        return UPackage::SavePackage(BP->GetOutermost(),BP,*FPackageName::LongPackageNameToFilename(BP->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),Args);
    }
};
bool BuildRow()
{
    FBuilder P(TEXT("WBP_SystemSettingsRow"),true);if(!P.BP)return false;
    P.Surface(TEXT("OptionBackground"),0,0,1176,142,FLinearColor(.025f,.032f,.036f,1),nullptr,true);
    P.Surface(TEXT("OptionRule"),20,141,1136,1,FLinearColor(.14f,.17f,.18f,1));
    P.Label(TEXT("OptionLabel"),TEXT("选项"),22,14,606,36,23,nullptr,true);
    P.Label(TEXT("OptionDescription"),TEXT("说明"),22,57,606,74,18)->SetColorAndOpacity(FLinearColor(.56f,.61f,.60f,1));
    P.Label(TEXT("OptionValue"),TEXT(""),670,13,478,33,22,nullptr,false)->SetJustification(ETextJustify::Right);
    auto* Slider=P.Make<USlider>(TEXT("OptionSlider"));auto Style=Slider->GetWidgetStyle();
    Style.NormalBarImage=FSlateColorBrush(FLinearColor(.22f,.27f,.28f,1));Style.HoveredBarImage=FSlateColorBrush(FLinearColor(.32f,.37f,.38f,1));
    Style.NormalThumbImage=FSlateRoundedBoxBrush(P.Theme->Paper,3.f);Style.NormalThumbImage.ImageSize=FVector2D(14,26);
    Style.HoveredThumbImage=FSlateRoundedBoxBrush(P.Theme->Accent,3.f);Style.HoveredThumbImage.ImageSize=FVector2D(14,26);Style.BarThickness=4;
    Slider->SetWidgetStyle(Style);Slider->SetSliderBarColor(FLinearColor::White);Slider->SetSliderHandleColor(FLinearColor::White);
    Slider->MouseUsesStep=true;P.Place(Slider,670,62,478,47);
    auto* Choice=P.Make<UComboBoxString>(TEXT("OptionChoice"));auto ComboStyle=Choice->GetWidgetStyle();ComboStyle.ComboButtonStyle.ButtonStyle=P.Theme->DarkButton;
    Choice->SetWidgetStyle(ComboStyle);Choice->SetContentPadding(FMargin(14,7,14,7));
    auto ItemStyle=Choice->GetItemStyle();ItemStyle.EvenRowBackgroundBrush=FSlateColorBrush(FLinearColor(.023f,.03f,.035f,1));ItemStyle.OddRowBackgroundBrush=ItemStyle.EvenRowBackgroundBrush;
    ItemStyle.EvenRowBackgroundHoveredBrush=FSlateColorBrush(FLinearColor(.18f,.22f,.24f,1));ItemStyle.OddRowBackgroundHoveredBrush=ItemStyle.EvenRowBackgroundHoveredBrush;
    Choice->SetItemStyle(ItemStyle);P.Place(Choice,670,59,478,52);
    P.Button(TEXT("OptionAction"),TEXT("设置按键"),670,48,478,58);
    for(const TCHAR* Name:{TEXT("OptionSlider"),TEXT("OptionChoice"),TEXT("OptionAction")})P.Tree->FindWidget(Name)->SetVisibility(ESlateVisibility::Collapsed);
    return P.Save();
}
bool BuildPanel()
{
    FBuilder P(TEXT("WBP_SystemSettings"),false);if(!P.BP)return false;
    const auto Dark=FLinearColor(.012f,.018f,.022f,1),Panel=FLinearColor(.019f,.026f,.030f,1),Line=FLinearColor(.14f,.18f,.19f,1);
    P.Label(TEXT("SettingsLayoutVersion"),TEXT("1"),0,0,1,1,1)->SetVisibility(ESlateVisibility::Collapsed);
    auto* Backdrop=P.Make<UBorder>(TEXT("SettingsBackdrop"));Backdrop->SetBrushColor(Dark);P.Place(Backdrop,0,0,1600,900);
    for(int32 I=0;I<9;++I)P.Surface(*FString::Printf(TEXT("SettingsGridLine%d"),I),40+I*184,0,1,900,FLinearColor(.06f,.09f,.10f,.4f));
    auto* Content=P.Group(TEXT("SettingsContent"),0,0,1600,900);
    P.Surface(TEXT("SettingsTopRule"),40,37,1520,1,Line,Content);
    P.Surface(TEXT("SettingsTitleAccent"),40,68,5,52,P.Theme->Accent,Content);
    P.Label(TEXT("SettingsTitle"),TEXT("系统设置"),61,57,770,68,39,Content,true);
    P.Label(TEXT("SettingsSubtitle"),TEXT("SYSTEM SETTINGS"),62,129,580,34,20,Content)->SetColorAndOpacity(P.Theme->Muted);
    for(int32 I=0;I<5;++I)P.Button(PageActions[I],FString::Printf(TEXT("%02d  %s"),I+1,Pages[I]),40,194+I*70,274,56,Content);
    P.Surface(TEXT("SettingsNavigationRule"),40,571,274,1,Line,Content);
    P.Label(TEXT("GraphicsStatus"),TEXT("画面 · 无待应用修改"),44,592,270,80,20,Content);
    P.Label(TEXT("SettingsHelp"),TEXT("Esc 返回上一级\n设置保存到本机"),44,691,266,75,19,Content)->SetColorAndOpacity(P.Theme->Muted);
    P.Button(TEXT("Back"),TEXT("返回 / Esc"),40,818,274,54,Content);
    P.Label(TEXT("SettingsPageTitle"),TEXT("画面"),352,83,1130,54,30,Content,true);
    P.Label(TEXT("SettingsPageNote"),TEXT("修改后点击应用；切换分类保留未应用内容。"),352,144,1150,40,19,Content)->SetColorAndOpacity(FLinearColor(.56f,.61f,.60f,1));
    P.Surface(TEXT("SettingsOptionsBG"),340,190,1220,554,Panel,Content,true);
    auto* List=P.Make<UScrollBox>(TEXT("SettingsRows"));List->SetScrollBarVisibility(ESlateVisibility::Visible);P.Place(List,352,203,1198,525,Content);
    auto* Preview=P.Group(TEXT("HUDPreview"),352,505,1198,228,Content);Preview->SetClipping(EWidgetClipping::ClipToBounds);
    P.Surface(TEXT("HUDPreviewBG"),0,0,1198,228,Dark,Preview,true);
    P.Label(TEXT("HUDPreviewCaption"),TEXT("HUD 缩放示意"),22,13,1125,34,19,Preview);
    P.Label(TEXT("HUDPreviewNote"),TEXT("示意比例；实际位置随游戏视口适配。"),22,48,1125,29,18,Preview)->SetColorAndOpacity(P.Theme->Muted);
    auto* Mini=P.Group(TEXT("HUDPreviewContent"),189,99,820,91,Preview);Mini->SetRenderTransformPivot(FVector2D(.5f,.5f));
    P.Surface(TEXT("HUDHealthBG"),0,0,230,91,Panel,Mini,true);
    P.Label(TEXT("HUDHealthLabel"),TEXT("生命"),14,8,130,25,16,Mini);
    P.Label(TEXT("HUDHealthValue"),TEXT("100"),14,29,160,43,31,Mini,true,true);
    P.Surface(TEXT("HUDHealthBar"),14,77,202,4,P.Theme->Paper,Mini);
    P.Surface(TEXT("HUDAmmoBG"),590,0,230,91,Panel,Mini,true);
    P.Label(TEXT("HUDWeaponLabel"),TEXT("M4"),606,8,195,25,17,Mini,false,true);
    P.Label(TEXT("HUDAmmoValue"),TEXT("30 / 120"),606,33,195,47,31,Mini,true,true);
    Preview->SetVisibility(ESlateVisibility::Collapsed);
    P.Label(TEXT("SettingsFeedback"),TEXT(""),352,752,1198,58,19,Content);
    P.Button(TEXT("ResetPage"),TEXT("恢复本页默认"),352,818,236,54,Content);
    P.Button(TEXT("ApplyGraphics"),TEXT("应用画面修改"),1046,818,244,54,Content,true);
    P.Button(TEXT("DiscardGraphics"),TEXT("放弃画面修改"),1306,818,244,54,Content);
    auto* Modal=P.Group(TEXT("SettingsModal"),0,0,1600,900);
    auto* Shield=P.Make<UBorder>(TEXT("ModalShield"));Shield->SetBrushColor(FLinearColor(0,0,0,.76f));P.Place(Shield,0,0,1600,900,Modal);
    P.Surface(TEXT("ModalSurface"),350,244,900,418,Panel,Modal,true);
    P.Surface(TEXT("ModalAccent"),350,244,5,418,P.Theme->Accent,Modal);
    P.Label(TEXT("ModalTitle"),TEXT("确认"),386,272,826,57,31,Modal,true);
    P.Label(TEXT("ModalBody"),TEXT(""),386,342,826,150,22,Modal);
    P.Label(TEXT("ModalFeedback"),TEXT(""),386,502,826,58,18,Modal)->SetColorAndOpacity(P.Theme->Accent);
    auto ModalButton=[&](const TCHAR* Name,const TCHAR* Label,int32 Column,bool Accent=false)
    {P.Button(Name,Label,386+Column*282,582,250,56,Modal,Accent)->SetVisibility(ESlateVisibility::Collapsed);};
    ModalButton(TEXT("ExitApply"),TEXT("应用并返回"),0,true);ModalButton(TEXT("ExitDiscard"),TEXT("放弃修改"),1);ModalButton(TEXT("ExitContinue"),TEXT("继续设置"),2);
    ModalButton(TEXT("DisplayConfirm"),TEXT("保留显示设置"),0,true);ModalButton(TEXT("DisplayRevert"),TEXT("恢复此前设置"),2);
    ModalButton(TEXT("ResetConfirm"),TEXT("确认恢复默认"),0,true);ModalButton(TEXT("ModalCancel"),TEXT("取消"),2);
    ModalButton(TEXT("CaptureClear"),TEXT("清空此绑定"),0);ModalButton(TEXT("CaptureCancel"),TEXT("取消 / Esc"),2);
    ModalButton(TEXT("ConflictReplace"),TEXT("替换原绑定"),0,true);ModalButton(TEXT("ConflictCancel"),TEXT("取消"),2);
    Modal->SetVisibility(ESlateVisibility::Collapsed);return P.Save();
}
}
#endif

FString UFPSSettingsUIBuilder::BuildAssets()
{
#if WITH_EDITOR
    using namespace SettingsUI;int32 Created=0;
    if(!LoadObject<UWidgetBlueprint>(nullptr,*(FString(Root)+TEXT("WBP_SystemSettingsRow.WBP_SystemSettingsRow"))))
    {if(!BuildRow())return TEXT("FAIL settings row build/save");++Created;}
    if(!LoadObject<UWidgetBlueprint>(nullptr,*(FString(Root)+TEXT("WBP_SystemSettings.WBP_SystemSettings"))))
    {if(!BuildPanel())return TEXT("FAIL settings panel build/save");++Created;}
    return FString::Printf(TEXT("PASS system settings created=%d; existing assets preserved"),Created);
#else
    return TEXT("FAIL editor required");
#endif
}
