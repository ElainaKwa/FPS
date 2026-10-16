#include "FPSMatchFlow.h"
#include "FPSSystemSettings.h"
#include "FPSUIStyle.h"
#include "FPSInterface.h"
#include "FPSAcceptanceSmoke.h"
#include "FPSLoadoutSmoke.h"
#include "FPSDropSmoke.h"
#include "FPSMovementSmoke.h"
#include "FPSCharacterTravelSmoke.h"
#include "FPSArtCapture.h"

#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "FPSDemoCharacter.h"
#include "FPSBaseMembership.h"
#include "FPSLoadoutMenu.h"
#include "FPSCustomizationMenu.h"
#include "FPSWeaponComponent.h"
#include "FPSFrontGameInstance.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/CommandLine.h"

namespace
{
bool IsLobby(const UWorld* World)
{
    return World && World->GetGameState<AFPSLobbyState>();
}


}

void AFPSFrontController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController()) return;
    MenuAction = NewObject<UInputAction>(this);
    MenuAction->ValueType = EInputActionValueType::Boolean;
    MenuContext = NewObject<UInputMappingContext>(this);
    MenuContext->MapKey(MenuAction, EKeys::Escape);
    LoadoutAction = NewObject<UInputAction>(this);
    LoadoutAction->ValueType = EInputActionValueType::Boolean;
    MenuContext->MapKey(LoadoutAction, EKeys::P);
    CustomizationAction = NewObject<UInputAction>(this);
    CustomizationAction->ValueType = EInputActionValueType::Boolean;
    MenuContext->MapKey(CustomizationAction, EKeys::B);
    if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
        Subsystem->AddMappingContext(MenuContext, 20);
    if (auto* Input = Cast<UEnhancedInputComponent>(InputComponent))
    {
        Input->BindAction(MenuAction, ETriggerEvent::Started, this, &AFPSFrontController::ToggleMenu);
        Input->BindAction(LoadoutAction, ETriggerEvent::Started, this, &AFPSFrontController::ToggleLoadout);
        Input->BindAction(CustomizationAction, ETriggerEvent::Started, this, &AFPSFrontController::ToggleCustomization);
    }
    RefreshMenu();
    RefreshSettingsMappings();
}

void AFPSFrontController::RefreshSettingsMappings()
{
    auto* S=UFPSSystemSettings::Get(this);if(!S||!MenuContext||SettingsMappingRevision==S->BindingRevision())return;
    SettingsMappingRevision=S->BindingRevision();MenuContext->UnmapAll();MenuContext->MapKey(MenuAction,EKeys::Escape);
    for(const auto& Pair:TArray<TPair<FName,UInputAction*>>{{"Loadout",LoadoutAction},{"Customization",CustomizationAction}})
        if(const auto* B=S->Binding(Pair.Key))for(FKey Key:{B->Primary,B->Secondary})if(Key.IsValid())MenuContext->MapKey(Pair.Value,Key);
    if(auto* Sub=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {FModifyContextOptions O;O.bForceImmediately=true;O.bIgnoreAllPressedKeysUntilRelease=true;Sub->RequestRebuildControlMappings(O,EInputMappingRebuildType::RebuildWithFlush);}
}

void AFPSFrontController::SetupInputComponent()
{
    Super::SetupInputComponent();
}

void AFPSFrontController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    FFPSAcceptanceSmoke::Tick(this, DeltaSeconds);
    FFPSLoadoutSmoke::Tick(this);
    FFPSDropSmoke::Tick(this);
    FFPSMovementSmoke::Tick(this, DeltaSeconds);
    FFPSCharacterTravelSmoke::Tick(this);
    FFPSArtCapture::Tick(this);
    if (!IsLocalController()) return;
    RefreshSettingsMappings();
    if (CustomizationMenu.IsValid())
    {
        const auto* Body = Cast<AFPSDemoCharacter>(GetPawn());
        if (!Body || Body != CustomizationBody.Get() || Body->IsEliminated() || !Body->GetWeapons()->IsCustomizationOpen()) HideCustomization();
    }
    if (LoadoutMenu.IsValid())
    {
        LoadoutMenu->SetRenderOpacity(.35f+.65f*FMath::Clamp(float((GetWorld()->GetTimeSeconds()-LoadoutOpenedAt)/.15),0.f,1.f));
        auto* Body = Cast<AFPSDemoCharacter>(GetPawn());
        const auto* Membership = Body ? Body->FindComponentByClass<UFPSBaseMembership>() : nullptr;
        if (Body != LoadoutBody.Get() || !Membership || !Membership->CanSelect()) HideLoadout();
    }
    if(Menu.IsValid()) Menu->SetRenderOpacity(.35f+.65f*FMath::Clamp(float((GetWorld()->GetTimeSeconds()-MenuOpenedAt)/.15),0.f,1.f));
    const auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
#if !UE_BUILD_SHIPPING
    if (GS && FParse::Param(FCommandLine::Get(), TEXT("FPSFlowSmoke")))
    {
        static TMap<TWeakObjectPtr<APlayerController>, FString> PreviousSnapshots;
        FString Snapshot = FString::Printf(TEXT("waiting=%d finished=%d players=%d podium=%d"), GS->bWaiting, GS->bFinished, GS->PlayerArray.Num(), GS->Podium.Num());
        for (const APlayerState* Base : GS->PlayerArray)
            if (const auto* PS = Cast<AFPSDemoPlayerState>(Base))
                Snapshot += FString::Printf(TEXT(" [%s team=%d score=%d deaths=%d]"), *PS->GetPlayerName(), PS->TeamId, PS->GetEliminationCount(), PS->GetDeathCount());
        if (PreviousSnapshots.FindRef(this) != Snapshot)
        {
            UE_LOG(LogTemp, Display, TEXT("FPS_SMOKE SNAPSHOT %s %s"), HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), *Snapshot);
            PreviousSnapshots.Add(this, Snapshot);
        }
    }
#endif
    RefreshMenu();
}

void AFPSFrontController::RefreshMenu()
{
    if (!IsLocalController()) return;
    const auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
    const bool bWaiting = IsLobby(GetWorld()) || (GS && (GS->bWaiting || GS->bFinished));
    if (bWaiting != bWasWaiting)
    {
        HideMenu();
        bWasWaiting = bWaiting;
    }
    // Retry if the replicated GameState or local viewport was not ready at BeginPlay.
    if (bWaiting && !Menu.IsValid()) ShowMenu();
}

void AFPSFrontController::EndPlay(const EEndPlayReason::Type Reason)
{
    HideCustomization();
    HideLoadout();
    HideMenu();
    if (GetLocalPlayer())
        if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
            Subsystem->RemoveMappingContext(MenuContext);
    Super::EndPlay(Reason);
}

void AFPSFrontController::ToggleMenu()
{
    if(IsSettingsOpen()){SettingsBack();return;}
    if (CustomizationMenu.IsValid()) { HideCustomization(); return; }
    if (LoadoutMenu.IsValid()) { HideLoadout(); return; }
    if (IsLobby(GetWorld())) { if (bMultiplayerMenu) SelectMultiplayerMenu(false); return; }
    if (bWasWaiting) return;
    if (Menu.IsValid()) HideMenu(); else ShowMenu();
}

void AFPSFrontController::SelectMultiplayerMenu(bool bShowMultiplayer)
{
    if (!IsLobby(GetWorld())) return;
    HideMenu();
    bMultiplayerMenu = bShowMultiplayer;
    Error.Empty();
    ShowMenu();
    if (bMultiplayerMenu && MenuAsset) MenuAsset->Dispatch(TEXT("LAN"));
}

void AFPSFrontController::HideMenu()
{
    CloseSettingsForTravel();
    if (Menu.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetForPlayer(GetLocalPlayer(), Menu.ToSharedRef());
    Menu.Reset();
    MenuAsset=nullptr;
    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());
    SetIgnoreMoveInput(false);
    SetIgnoreLookInput(false);
}

void AFPSFrontController::Travel(const FString& Options)
{
    if (auto* Instance = GetGameInstance<UFPSFrontGameInstance>()) Instance->ConnectionError.Empty();
    HideMenu();
    const FName Map = Options.Contains(TEXT("Mode=Training"))
        ? TEXT("/Game/FPSDemo/Maps/LV_FPSTraining") : TEXT("/Game/FPSDemo/Maps/LV_FPSFoundry");
    UGameplayStatics::OpenLevel(this, Map, true,
        TEXT("game=/Script/MCP_Test.FPSMatchMode?") + Options);
}

FString AFPSFrontController::StatusText() const
{
    if (const auto* Instance = GetGameInstance<UFPSFrontGameInstance>())
        if (!Instance->ConnectionError.IsEmpty()) return Instance->ConnectionError;
    if (!Error.IsEmpty()) return Error;
    const auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
    if (!GS) return TEXT("局域网联机支持 2–10 人，每队最多 5 人。");
    FString Text = GS->bTraining ? TEXT("靶场 · 单人练习\n") : GS->bFinished ? TEXT("对局结束 · 前三名\n") : GS->bWaiting ? GS->RoomName + TEXT(" · 等待房主开始\n") : TEXT("对局进行中（菜单不暂停计时）\n");
    if (GS->bFinished)
    {
        for (int32 I = 0; I < GS->Podium.Num(); ++I)
            Text += FString::Printf(TEXT("%d. %s   %d 分 / %d 死亡\n"), I + 1, *GS->Podium[I].Name, GS->Podium[I].Kills, GS->Podium[I].Deaths);
    }
    else
    {
        for (int32 Team = 0; Team < 2; ++Team)
        {
            Text += Team == 0 ? TEXT("\n蓝队：\n") : TEXT("\n红队：\n");
            for (const APlayerState* Base : GS->PlayerArray)
                if (const auto* PS = Cast<AFPSDemoPlayerState>(Base))
                    if (PS->TeamId == Team)
                        Text += FString::Printf(TEXT("  %s%s   %d 分\n"), *PS->GetPlayerName(), PS == PlayerState ? TEXT("（你）") : TEXT(""), PS->GetEliminationCount());
        }
        if (GS->bWaiting) Text += TEXT("\n双方均有玩家后，房主可以开始。切换阵营在目标队伍满员时不生效。");
    }
    return Text;
}

void AFPSFrontController::ShowMenu()
{
    HideCustomization();
    if (!IsLocalController() || !GetLocalPlayer() || Menu.IsValid() || !GetWorld()->GetGameViewport()) return;
    HideLoadout();
    if (auto* Fighter = Cast<AFPSDemoCharacter>(GetPawn())) Fighter->StopCombatInput();
    const auto* GS=GetWorld()->GetGameState<AFPSDemoGameState>();
    const TCHAR* Page=IsLobby(GetWorld()) ? (bMultiplayerMenu ? TEXT("Lobby") : TEXT("Start")) : GS && GS->bFinished ? TEXT("Report") : GS && GS->bWaiting ? TEXT("Waiting") : TEXT("Pause");
    MenuAsset=UFPSInterface::Open(this,Page);
    if(!MenuAsset)return;
    Menu=MenuAsset->TakeWidget();
    MenuOpenedAt=GetWorld()->GetTimeSeconds();Menu->SetRenderOpacity(.35f);
    GetWorld()->GetGameViewport()->AddViewportWidgetForPlayer(GetLocalPlayer(), Menu.ToSharedRef(), 50);
    bShowMouseCursor = true;
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(Menu);
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("FPSLobbySmoke")))
    {
        const bool bCompleteLobby = IsLobby(GetWorld()) && MenuAsset && (bMultiplayerMenu ? MenuAsset->HasControl(TEXT("BotInput")) && MenuAsset->HasControl(TEXT("RoomInput")) && MenuAsset->HasControl(TEXT("AddressInput")) : MenuAsset->HasControl(TEXT("Multiplayer")) && MenuAsset->HasControl(TEXT("Recovery")));
        UE_LOG(LogTemp, Display, TEXT("FPS_LOBBY_SMOKE %s menu-ready role=%s auth-mode=%d"),
            bCompleteLobby ? TEXT("PASS") : TEXT("FAIL"), HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"),
            GetWorld()->GetAuthGameMode() != nullptr);
    }
#endif
}

void AFPSFrontController::ServerStartRound_Implementation()
{
    if (auto* Mode = GetWorld()->GetAuthGameMode<AFPSMatchMode>()) Mode->StartRound(this);
}

void AFPSFrontController::ServerSwitchTeam_Implementation()
{
    if (auto* Mode = GetWorld()->GetAuthGameMode<AFPSMatchMode>()) Mode->SwitchTeam(this);
}
