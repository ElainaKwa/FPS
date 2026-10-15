#include "FPSCustomizationMenu.h"
#include "FPSDemoCharacter.h"
#include "FPSWeaponComponent.h"
#include "Input/DragAndDrop.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

namespace
{
const FTextBlockStyle& EntryText()
{
    static const FTextBlockStyle Style = FTextBlockStyle(FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText")).SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 18));
    return Style;
}
class FMagazineDrag : public FDragDropOperation
{
public:
    DRAG_DROP_OPERATOR_TYPE(FMagazineDrag, FDragDropOperation)
    FMagazineDrag() { bCreateNewWindow = false; }
    FName Part; FGuid Instance; uint32 Revision = 0; FText Label;
    virtual TSharedPtr<SWidget> GetDefaultDecorator() const override
    {
        return SNew(SBorder).Padding(8)[SNew(STextBlock).TextStyle(&EntryText()).Text(Label)];
    }
    virtual FVector2D GetDecoratorPosition() const override { return FSlateApplication::Get().GetCursorPos() + FVector2D(12,12); }
    static TSharedRef<FMagazineDrag> Make(FName Part, const FFPSWeaponInstance& Gun, const FText& Name)
    {
        auto Drag = MakeShared<FMagazineDrag>(); Drag->Part = Part; Drag->Instance = Gun.InstanceId; Drag->Revision = Gun.ConfigurationRevision;
        Drag->Label = Name; Drag->Construct(); return Drag;
    }
};
class SAttachmentEntry : public SBorder
{
public:
    SLATE_BEGIN_ARGS(SAttachmentEntry) {} SLATE_ARGUMENT(UFPSWeaponComponent*, Weapon) SLATE_ARGUMENT(FName, Part) SLATE_ARGUMENT(FText, Label) SLATE_EVENT(FSimpleDelegate, Preview) SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Weapon = Args._Weapon; Part = Args._Part; Label = Args._Label; Preview = Args._Preview;
        SBorder::Construct(SBorder::FArguments().BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.055f,.075f,.09f,1)).Padding(10)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).Text(Label)]);
    }
    virtual void OnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) override { SBorder::OnMouseEnter(Geometry, Event); Preview.ExecuteIfBound(); }
    virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& Event) override
    {
        return Event.GetEffectingButton() == EKeys::LeftMouseButton ? FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton) : FReply::Unhandled();
    }
    virtual FReply OnDragDetected(const FGeometry&, const FPointerEvent&) override
    {
        const auto* W = Weapon.Get();
        if (!W || W->GetLoadout().Action != EFPSWeaponAction::Idle) return FReply::Unhandled();
        return FReply::Handled().BeginDragDrop(FMagazineDrag::Make(Part, W->Equipped(), Label));
    }
private:
    TWeakObjectPtr<UFPSWeaponComponent> Weapon; FName Part; FText Label; FSimpleDelegate Preview;
};
class SWeaponDropTarget : public SBorder
{
public:
    SLATE_BEGIN_ARGS(SWeaponDropTarget) {} SLATE_ARGUMENT(UFPSWeaponComponent*, Weapon) SLATE_DEFAULT_SLOT(FArguments, Content) SLATE_END_ARGS()
    void Construct(const FArguments& Args) { Weapon = Args._Weapon; SBorder::Construct(SBorder::FArguments().Padding(12)[Args._Content.Widget]); }
    virtual FReply OnDrop(const FGeometry&, const FDragDropEvent& Event) override
    {
        const auto Drag = Event.GetOperationAs<FMagazineDrag>(); auto* W = Weapon.Get();
        if (!Drag || !W || Drag->Instance != W->Equipped().InstanceId || Drag->Revision != W->Equipped().ConfigurationRevision) return FReply::Unhandled();
        W->ModifyAttachment(EFPSAttachmentCategory::Magazine, Drag->Part); return FReply::Handled();
    }
private: TWeakObjectPtr<UFPSWeaponComponent> Weapon;
};
class SFPSCustomizationMenu : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFPSCustomizationMenu) {} SLATE_ARGUMENT(AFPSDemoCharacter*, Character) SLATE_ARGUMENT(TFunction<void()>, Close) SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Weapon = Args._Character->GetWeapons();
        const auto Close = Args._Close;
        ChildSlot.HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            SNew(SBox).WidthOverride(1050).HeightOverride(760)
            [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.015f,.022f,.03f,1)).Padding(18)
                [SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(4)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).Text(FText::FromString(TEXT("训练改装  |  B / Esc 关闭")))]
                    + SVerticalBox::Slot().AutoHeight().Padding(4)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).AutoWrapText(true).Text(FText::FromString(TEXT("普通配件点击应用。把弹匣拖到下方武器区开始换弹；点击“换弹中”取消。关闭界面会继续换弹。")))]
                    + SVerticalBox::Slot().AutoHeight().Padding(4)
                    [SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(1)[SlotButton(EFPSWeaponSlot::Primary)]
                        + SHorizontalBox::Slot().FillWidth(1)[SlotButton(EFPSWeaponSlot::Secondary)]]
                    + SVerticalBox::Slot().AutoHeight().Padding(4)
                    [SAssignNew(DropTarget, SWeaponDropTarget).Weapon(Weapon.Get())
                        [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).AutoWrapText(true).Text_Lambda([this]() { return Summary(); })]]
                    + SVerticalBox::Slot().AutoHeight().Padding(4)
                    [SNew(SBox).HeightOverride(68)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).AutoWrapText(true).Text_Lambda([this]() { return FText::FromString(PreviewText); })]]
                    + SVerticalBox::Slot().FillHeight(1).Padding(4)
                    [SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(Parts, SVerticalBox)]]
                    + SVerticalBox::Slot().AutoHeight().Padding(4)
                    [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).AutoWrapText(true).Text_Lambda([this]() { const auto* W = Weapon.Get(); return FText::FromString(W ? W->CustomizationFeedback() : TEXT("角色已失效。")); })]
                    + SVerticalBox::Slot().AutoHeight().Padding(4)
                    [SNew(SButton).TextStyle(&EntryText()).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button")).Text(FText::FromString(TEXT("关闭"))).OnClicked_Lambda([Close]() { Close(); return FReply::Handled(); })]
                ]
            ]
        ];
        Refresh();
    }
    virtual void Tick(const FGeometry& Geometry, double Time, float Delta) override
    {
        SCompoundWidget::Tick(Geometry, Time, Delta);
        const auto* W = Weapon.Get();
        if (W && (ViewedInstance != W->Equipped().InstanceId || ViewedRevision != W->Equipped().ConfigurationRevision)) Refresh();
    }
#if !UE_BUILD_SHIPPING
    bool SmokePointer(UFPSWeaponComponent* W, FName Part, int32 Phase)
    {
        if (Weapon.Get() != W) return false;
        TSharedPtr<SWidget> Source = Phase < 3 ? MagazineEntries.FindRef(Part) : PrimaryButton;
        if (!Source || !DropTarget || !FSlateApplication::IsInitialized()) return false;
        auto& App = FSlateApplication::Get();
        const FVector2D Start = Source->GetCachedGeometry().GetAbsolutePositionAtCoordinates(FVector2D(.5,.5));
        const FVector2D End = DropTarget->GetCachedGeometry().GetAbsolutePositionAtCoordinates(FVector2D(.5,.5));
        const auto Window = App.FindWidgetWindow(Source.ToSharedRef());
        if (!Window || Source->GetCachedGeometry().GetLocalSize().X <= 0) return false;
        const TSet<FKey> Pressed{EKeys::LeftMouseButton};
        if (Phase == 0 || Phase == 3)
        {
            App.ProcessMouseMoveEvent(FPointerEvent(0, Start, Start, {}, EKeys::Invalid, 0, FModifierKeysState()), false);
            return App.ProcessMouseButtonDownEvent(Window->GetNativeWindow(), FPointerEvent(0, Start, Start, Pressed, EKeys::LeftMouseButton, 0, FModifierKeysState()));
        }
        if (Phase == 1)
        {
            App.ProcessMouseMoveEvent(FPointerEvent(0, Start + FVector2D(25,0), Start, Pressed, EKeys::Invalid, 0, FModifierKeysState()), false);
            return App.IsDragDropping();
        }
        const FVector2D Target = Phase == 2 ? End : Start;
        if (Phase == 2) App.ProcessMouseMoveEvent(FPointerEvent(0, Target, Start + FVector2D(25,0), Pressed, EKeys::Invalid, 0, FModifierKeysState()), false);
        return App.ProcessMouseButtonUpEvent(FPointerEvent(0, Target, Target, {}, EKeys::LeftMouseButton, 0, FModifierKeysState()));
    }
#endif
private:
    TWeakObjectPtr<UFPSWeaponComponent> Weapon;
    TSharedPtr<SVerticalBox> Parts;
    TSharedPtr<SWeaponDropTarget> DropTarget;
    TSharedPtr<SWidget> PrimaryButton;
    TMap<FName, TSharedPtr<SWidget>> MagazineEntries;
    FGuid ViewedInstance; uint32 ViewedRevision = 0;
    FString PreviewText;
    void PreviewPart(EFPSAttachmentCategory Category, FName Id)
    {
        const auto* W = Weapon.Get(); if (!W) return;
        TArray<FName> Ids = W->Equipped().AttachmentIds;
        Ids.RemoveAll([&](FName PartId) { const auto* Part = W->Catalog()->FindAttachment(PartId); return Part && Part->Category == Category; });
        if (!Id.IsNone()) Ids.Add(Id);
        TArray<FString> Errors; FFPSResolvedWeaponStats Stats;
        if (!W->Catalog()->Resolve(W->Equipped().WeaponId, Ids, Stats, Errors)) { PreviewText = FString::Join(Errors, TEXT("\n")); return; }
        const auto Current = W->EffectiveStats();
        PreviewText = FString::Printf(TEXT("预览（尚未应用）  容量 %d → %d   换弹 %.2f → %.2f 秒   FOV %.1f → %.1f\nADS %.2f → %.2f 秒   垂直 / 水平后坐力 %.2f / %.2f → %.2f / %.2f\n最大射程 %.1f → %.1f 米"), Current.MagazineSize, Stats.MagazineSize,
            Current.ReloadDuration, Stats.ReloadDuration, Current.AimFOV, Stats.AimFOV, Current.AimTransitionTime, Stats.AimTransitionTime,
            Current.VerticalRecoilMultiplier, Current.HorizontalRecoilMultiplier, Stats.VerticalRecoilMultiplier, Stats.HorizontalRecoilMultiplier,
            Current.Range / 100.f, Stats.Range / 100.f);
    }
    TSharedRef<SWidget> SlotButton(EFPSWeaponSlot Slot)
    {
        auto Button = SNew(SButton).TextStyle(&EntryText()).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button")).Text_Lambda([this, Slot]()
        {
            const auto* W = Weapon.Get(); if (!W) return FText();
            const auto& State = W->GetLoadout(); const auto& Gun = Slot == EFPSWeaponSlot::Primary ? State.Primary : State.Secondary;
            const auto* Def = W->Catalog()->Find(Gun.WeaponId);
            FString Label = Def ? Def->DisplayName.ToString() : TEXT("空槽位");
            if (State.ActiveSlot == Slot) Label += State.Action == EFPSWeaponAction::Reloading ? TEXT("  换弹中（点击取消）") : TEXT("  当前武器");
            else Label += TEXT("  点击切换");
            return FText::FromString(Label);
        }).OnClicked_Lambda([this, Slot]()
        {
            if (auto* W = Weapon.Get())
            {
                if (W->GetLoadout().ActiveSlot == Slot) W->CancelCustomizationReload();
                else W->Request(Slot == EFPSWeaponSlot::Primary ? EFPSWeaponRequest::Primary : EFPSWeaponRequest::Secondary);
            }
            return FReply::Handled();
        });
        if (Slot == EFPSWeaponSlot::Primary) PrimaryButton = Button;
        return Button;
    }
    FText Summary() const
    {
        const auto* W = Weapon.Get(); if (!W || !W->Definition()) return FText::FromString(TEXT("当前无武器。"));
        const auto Stats = W->EffectiveStats();
        return FText::FromString(FString::Printf(TEXT("%s  |  %d / %d 发，备弹 %d\n换弹 %.2f 秒   ADS %.2f 秒   FOV %.1f 度   后坐力倍率 %.2f / %.2f\n最大射程 %.1f 米\n此处显示已提交装配；拖入弹匣可更换，悬停配件可比较参数。"),
            *W->Definition()->DisplayName.ToString(), W->Equipped().Magazine, Stats.MagazineSize, W->Equipped().Reserve,
            Stats.ReloadDuration, Stats.AimTransitionTime, Stats.AimFOV, Stats.VerticalRecoilMultiplier, Stats.HorizontalRecoilMultiplier, Stats.Range / 100.f));
    }
    void Refresh()
    {
        auto* W = Weapon.Get(); if (!W) return;
        ViewedInstance = W->Equipped().InstanceId; ViewedRevision = W->Equipped().ConfigurationRevision;
        PreviewText = TEXT("悬停配件以对比参数；查看预览不会修改武器。");
        Parts->ClearChildren(); MagazineEntries.Empty(); const auto* Def = W->Definition(); if (!Def) return;

        for (int32 Index = 0; Index < FPSAttachments::CategoryCount; ++Index)
        {
            const auto Category = EFPSAttachmentCategory(Index);
            Parts->AddSlot().AutoHeight().Padding(4)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).Text(FText::FromString(FPSAttachments::Label(Category)))];
            auto Row = SNew(SHorizontalBox);
            auto Add = [&](FName Id, FText Label)
            {
                if (Category == EFPSAttachmentCategory::Magazine)
                {
                    auto Entry = SNew(SAttachmentEntry).Weapon(W).Part(Id).Label(Label).Preview_Lambda([this, Category, Id]() { PreviewPart(Category, Id); });
                    MagazineEntries.Add(Id, Entry); Row->AddSlot().FillWidth(1).Padding(3)[Entry];
                }
                else Row->AddSlot().FillWidth(1).Padding(3)[SNew(SButton).TextStyle(&EntryText()).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button")).Text(Label).OnHovered_Lambda([this, Category, Id]() { PreviewPart(Category, Id); }).OnClicked_Lambda([Weak = Weapon, Category, Id]() { if (auto* WeaponComponent = Weak.Get()) WeaponComponent->ModifyAttachment(Category, Id); return FReply::Handled(); })];
            };
            Add(NAME_None, FText::FromString(TEXT("原枪配置")));
            for (const auto& Ref : Def->AvailableAttachments)
                if (const auto* Part = Ref.Get(); Part && Part->Category == Category)
                    Add(Part->AttachmentId, FText::FromString(Part->DisplayName.ToString() + (W->Equipped().AttachmentIds.Contains(Part->AttachmentId) ? TEXT(" ✓") : TEXT(""))));
            Parts->AddSlot().AutoHeight()[Row];
        }
    }
};
#if !UE_BUILD_SHIPPING
TWeakPtr<SFPSCustomizationMenu> SmokeMenu;
#endif
}
TSharedRef<SWidget> MakeFPSCustomizationMenu(AFPSDemoCharacter* Character, TFunction<void()> Close)
{
    auto Menu = SNew(SFPSCustomizationMenu).Character(Character).Close(MoveTemp(Close));
#if !UE_BUILD_SHIPPING
    SmokeMenu = Menu;
#endif
    return Menu;
}
#if !UE_BUILD_SHIPPING
bool FPSCustomizationSmokePointer(UFPSWeaponComponent* Weapon, FName Part, int32 Phase)
{
    const auto Menu = SmokeMenu.Pin(); return Menu && Menu->SmokePointer(Weapon, Part, Phase);
}
#endif
