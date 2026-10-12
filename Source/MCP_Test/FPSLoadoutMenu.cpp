#include "FPSLoadoutMenu.h"
#include "FPSUIStyle.h"
#include "FPSInterface.h"
#include "Widgets/Images/SImage.h"
#include "Engine/GameInstance.h"
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
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"

void AFPSFrontController::ToggleLoadout()
{
    if (!IsLocalController()) return;
    HideCustomization();
    if (LoadoutMenu.IsValid()) { HideLoadout(); return; }
    auto* Body = Cast<AFPSDemoCharacter>(GetPawn());
    auto* Membership = Body ? Body->FindComponentByClass<UFPSBaseMembership>() : nullptr;
    if (!Membership || !Membership->CanSelect() || !IsWeaponContentVerified() || !GetLocalPlayer() || !GetWorld()->GetGameViewport()) return;
    HideMenu(); Body->StopCombatInput();
    SelectionFeedback.Empty(); LoadoutBody = Body;
    LoadoutOpenedAt=GetWorld()->GetTimeSeconds();
    LoadoutAsset=UFPSInterface::Open(this,TEXT("Loadout"));
    if(!LoadoutAsset)return;
    LoadoutMenu=LoadoutAsset->TakeWidget();
    LoadoutMenu->SetRenderOpacity(.35f);
    GetWorld()->GetGameViewport()->AddViewportWidgetForPlayer(GetLocalPlayer(), LoadoutMenu.ToSharedRef(), 60);
    bShowMouseCursor = true;
    FInputModeGameAndUI Mode; Mode.SetWidgetToFocus(LoadoutMenu); Mode.SetHideCursorDuringCapture(false); SetInputMode(Mode);
    SetIgnoreMoveInput(true); SetIgnoreLookInput(true);
}
void AFPSFrontController::HideLoadout()
{
    if (!LoadoutMenu.IsValid()) return;
    if (GetWorld() && GetWorld()->GetGameViewport()) GetWorld()->GetGameViewport()->RemoveViewportWidgetForPlayer(GetLocalPlayer(), LoadoutMenu.ToSharedRef());
    LoadoutMenu.Reset(); LoadoutAsset=nullptr; LoadoutBody.Reset(); bShowMouseCursor = false;
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
