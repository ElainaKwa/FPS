#include "FPSLoadoutMenu.h"
#include "FPSMatchFlow.h"
#include "FPSModeRules.h"
#include "FPSBaseMembership.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoPlayerState.h"
#include "FPSWeaponComponent.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"

/** Minimal vector weapon category icon; authored weapon art remains future work. */
class SFPSWeaponIcon : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SFPSWeaponIcon) {} SLATE_ARGUMENT(EFPSWeaponCategory, Category) SLATE_END_ARGS()
    void Construct(const FArguments& Args) { Category = Args._Category; }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(90,48); }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&, FSlateWindowElementList& Elements,
        int32 Layer, const FWidgetStyle&, bool) const override
    {
        auto Rect = [&](FVector2D Position, FVector2D Size)
        { FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor(.55f,.8f,1)); };
        const bool Pistol = Category == EFPSWeaponCategory::Pistol;
        Rect(FVector2D(Pistol ? 26 : 8,12), FVector2D(Pistol ? 42 : 72,8));
        Rect(FVector2D(28,20), FVector2D(12,21));
        if (!Pistol) { Rect(FVector2D(8,20), FVector2D(16,9)); Rect(FVector2D(49,20), FVector2D(9,15)); }
        return Layer;
    }
    EFPSWeaponCategory Category;
};
TSharedRef<SWidget> MakeFPSLoadoutMenu(AFPSFrontController* Controller)
{
    const auto* Rules = UFPSModeRules::ForWorld(Controller->GetWorld());
    auto Rows = SNew(SVerticalBox);
    Rows->AddSlot().AutoHeight().Padding(0,0,0,12)[SNew(STextBlock).Text(FText::FromString(TEXT("本局配装 · 点击选择 · P / Esc 关闭"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",24))];
    auto Columns = SNew(SHorizontalBox);
    for (const auto Slot : {EFPSWeaponSlot::Primary, EFPSWeaponSlot::Secondary})
    {
        auto List = SNew(SVerticalBox);
        List->AddSlot().AutoHeight().Padding(6)[SNew(STextBlock).Text(FText::FromString(Slot == EFPSWeaponSlot::Primary ? TEXT("主武器") : TEXT("副武器"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",22))];
        if (Rules) for (const auto& Ref : Rules->SelectableWeapons)
        {
            const auto* Def = Ref.Get();
            if (!Def || Def->Slot != Slot) continue;
            const FName Id = Def->WeaponId;
            List->AddSlot().AutoHeight().Padding(4)[SNew(SButton)
                .OnClicked_Lambda([Controller, Id]() { Controller->RequestSelection(Id); return FReply::Handled(); })
                [SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth()[SNew(SFPSWeaponIcon).Category(Def->Category)]
                    + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(STextBlock).Text(Def->DisplayName).Font(FCoreStyle::GetDefaultFontStyle("Regular",20))]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text_Lambda([Controller, Id, Slot]()
                    {
                        const auto* Body = Cast<AFPSDemoCharacter>(Controller->GetPawn());
                        if (!Body) return FText::GetEmpty();
                        const auto& Loadout = Body->GetWeapons()->GetLoadout();
                        return FText::FromString((Slot == EFPSWeaponSlot::Primary ? Loadout.Primary : Loadout.Secondary).WeaponId == Id ? TEXT(" ✓") : TEXT(""));
                    })]]];
        }
        Columns->AddSlot().FillWidth(1)[List];
    }
    Rows->AddSlot().AutoHeight()[Columns];
    Rows->AddSlot().AutoHeight().Padding(6,14)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",18)).AutoWrapText(true).Text_Lambda([Controller]()
    {
        const auto* Rules = UFPSModeRules::ForWorld(Controller->GetWorld());
        const FString Policy = Rules && Rules->Replacement == EFPSReplacementPolicy::Drop
            ? TEXT("\n靶场旧枪向前轻抛，可在空中按 F 拾取。")
            : TEXT("\n成功选择立即保存；首次离开所属基地后，本次生命不能再次选枪。");
        const FString Cap = Rules ? FString::Printf(TEXT("\n动态掉落最多 %d 把，保留 %.0f 秒；超出时回收最早的未拾取枪。"), Rules->DynamicDropLimit, Rules->DynamicDropLifetime) : FString();
        return FText::FromString(Controller->GetSelectionFeedback() + Policy + Cap);
    })];
    return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).HAlign(HAlign_Center).VAlign(VAlign_Center).BorderBackgroundColor(FLinearColor(.02f,.03f,.05f,.96f))
        [SNew(SBox).WidthOverride(850).Padding(24)[SNew(SScrollBox) + SScrollBox::Slot()[Rows]]];
}
void AFPSFrontController::ToggleLoadout()
{
    if (!IsLocalController()) return;
    if (LoadoutMenu.IsValid()) { HideLoadout(); return; }
    auto* Body = Cast<AFPSDemoCharacter>(GetPawn());
    auto* Membership = Body ? Body->FindComponentByClass<UFPSBaseMembership>() : nullptr;
    if (!Membership || !Membership->CanSelect() || !IsWeaponContentVerified() || !GetLocalPlayer() || !GetWorld()->GetGameViewport()) return;
    HideMenu(); Body->StopCombatInput();
    SelectionFeedback.Empty(); LoadoutBody = Body;
    LoadoutMenu = MakeFPSLoadoutMenu(this);
    GetWorld()->GetGameViewport()->AddViewportWidgetForPlayer(GetLocalPlayer(), LoadoutMenu.ToSharedRef(), 60);
    bShowMouseCursor = true;
    FInputModeGameAndUI Mode; Mode.SetWidgetToFocus(LoadoutMenu); Mode.SetHideCursorDuringCapture(false); SetInputMode(Mode);
    SetIgnoreMoveInput(true); SetIgnoreLookInput(true);
}
void AFPSFrontController::HideLoadout()
{
    if (!LoadoutMenu.IsValid()) return;
    if (GetWorld() && GetWorld()->GetGameViewport()) GetWorld()->GetGameViewport()->RemoveViewportWidgetForPlayer(GetLocalPlayer(), LoadoutMenu.ToSharedRef());
    LoadoutMenu.Reset(); LoadoutBody.Reset(); bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly()); SetIgnoreMoveInput(false); SetIgnoreLookInput(false);
}
void AFPSFrontController::RequestSelection(FName Id)
{
    if (!IsLocalController()) return;
    auto* Body = Cast<AFPSDemoCharacter>(GetPawn());
    const auto* Membership = Body ? Body->FindComponentByClass<UFPSBaseMembership>() : nullptr;
    if (!Membership || !Membership->CanSelect()) { SelectionFeedback = TEXT("当前没有选枪权限。"); HideLoadout(); return; }
    SelectionFeedback = TEXT("正在处理……");
    ServerSelectWeapon(Body, Membership->GetLifeId(), Id, ++NextSelectionRequest);
}
void AFPSFrontController::ServerSelectWeapon_Implementation(AFPSDemoCharacter* Body, FGuid Life, FName Id, uint32 RequestId)
{
    auto* Membership = IsValid(Body) ? Body->FindComponentByClass<UFPSBaseMembership>() : nullptr;
    FString Failure;
    const bool ValidLife = Membership && Body == GetPawn() && Body->GetController() == this && Life.IsValid() && Membership->GetLifeId() == Life;
    bool Success = false;
    if (IsWeaponContentVerified() && ValidLife)
    {
        const auto* Rules = UFPSModeRules::ForWorld(GetWorld());
        const double Now = GetWorld()->GetTimeSeconds();
        if (Now + .0001 < NextSelectionAllowedAt) Failure = TEXT("领取过快，请稍后再试。");
        else
        {
            NextSelectionAllowedAt = Now + (Rules ? Rules->SelectionRequestInterval : .1f);
            Success = Body->GetWeapons()->SelectDefinition(Id, Failure);
        }
    }
    if (!Success && Failure.IsEmpty()) Failure = TEXT("旧生命或未通过准入的请求已拒绝。");
    ClientSelectionResult(Body, Life, RequestId, Success, Success ? TEXT("已装备。") : Failure);
}
void AFPSFrontController::ClientSelectionResult_Implementation(AFPSDemoCharacter* Body, FGuid Life, uint32 RequestId, bool Success, const FString& Reason)
{
    const auto* Membership = IsValid(Body) ? Body->FindComponentByClass<UFPSBaseMembership>() : nullptr;
    if (!Membership || Body != GetPawn() || Membership->GetLifeId() != Life || RequestId < LastSelectionReply) return;
    LastSelectionReply = RequestId; bSelectionSuccess = Success; SelectionFeedback = Reason;
}
