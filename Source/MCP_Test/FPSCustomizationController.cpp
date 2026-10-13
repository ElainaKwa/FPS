#include "FPSMatchFlow.h"
#include "FPSCustomizationMenu.h"
#include "FPSDemoCharacter.h"
#include "FPSWeaponComponent.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWidget.h"

void AFPSFrontController::ToggleCustomization()
{
    if(IsSettingsOpen())return;
    if (CustomizationMenu.IsValid()) { HideCustomization(); return; }
    auto* Body = Cast<AFPSDemoCharacter>(GetPawn());
    if (!IsLocalController() || !GetLocalPlayer() || !Body || !GetWorld()->GetGameViewport()) return;
    HideLoadout(); HideMenu();
    if (!Body->GetWeapons()->SetCustomizationOpen(true)) return;
    Body->StopCombatInput();
    CustomizationBody = Body;
    const TWeakObjectPtr<AFPSFrontController> Weak(this);
    CustomizationMenu = MakeFPSCustomizationMenu(Body, [Weak]() { if (auto* PC = Weak.Get()) PC->HideCustomization(); });
    GetWorld()->GetGameViewport()->AddViewportWidgetForPlayer(GetLocalPlayer(), CustomizationMenu.ToSharedRef(), 55);
    SetIgnoreMoveInput(true); SetIgnoreLookInput(true); bShowMouseCursor = true;
    FInputModeGameAndUI Mode; Mode.SetWidgetToFocus(CustomizationMenu); Mode.SetHideCursorDuringCapture(false); SetInputMode(Mode);
}

void AFPSFrontController::HideCustomization()
{
    if (!CustomizationMenu.IsValid()) return;
    if (auto* Body = CustomizationBody.Get()) Body->GetWeapons()->SetCustomizationOpen(false);
    if (GetWorld() && GetWorld()->GetGameViewport() && GetLocalPlayer())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetForPlayer(GetLocalPlayer(), CustomizationMenu.ToSharedRef());
    CustomizationMenu.Reset(); CustomizationBody.Reset();
    SetIgnoreMoveInput(false); SetIgnoreLookInput(false); bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());
}
