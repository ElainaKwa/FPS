#include "FPSMatchFlow.h"

#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "FPSDemoCharacter.h"
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
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/CommandLine.h"

namespace
{
bool IsLobby(const UWorld* World)
{
    return World && World->GetGameState<AFPSLobbyState>();
}

bool IsValidAddress(const FString& Address)
{
    FString Host = Address;
    FString Port;
    if (Address.Split(TEXT(":"), &Host, &Port))
    {
        if (Port.IsEmpty() || !Port.IsNumeric() || FCString::Atoi(*Port) < 1 || FCString::Atoi(*Port) > 65535) return false;
    }
    TArray<FString> Parts;
    Host.ParseIntoArray(Parts, TEXT("."), false);
    if (Parts.Num() != 4) return false;
    for (const FString& Part : Parts)
    {
        if (Part.IsEmpty() || Part.Len() > 3) return false;
        for (TCHAR C : Part) if (C < '0' || C > '9') return false;
        if (FCString::Atoi(*Part) > 255) return false;
    }
    return true;
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
    if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
        Subsystem->AddMappingContext(MenuContext, 20);
    if (auto* Input = Cast<UEnhancedInputComponent>(InputComponent))
        Input->BindAction(MenuAction, ETriggerEvent::Started, this, &AFPSFrontController::ToggleMenu);
    RefreshMenu();
}

void AFPSFrontController::SetupInputComponent()
{
    Super::SetupInputComponent();
}

void AFPSFrontController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!IsLocalController()) return;
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
    HideMenu();
    if (GetLocalPlayer())
        if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
            Subsystem->RemoveMappingContext(MenuContext);
    Super::EndPlay(Reason);
}

void AFPSFrontController::ToggleMenu()
{
    if (bWasWaiting) return;
    if (Menu.IsValid()) HideMenu(); else ShowMenu();
}

void AFPSFrontController::HideMenu()
{
    if (Menu.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetForPlayer(GetLocalPlayer(), Menu.ToSharedRef());
    Menu.Reset();
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
    if (!GS) return TEXT("局域网房间支持 2–10 人，每队最多 5 人。\n操作：WASD 移动 / 鼠标射击 / Space 跳跃 / Esc 菜单");
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
    if (!IsLocalController() || !GetLocalPlayer() || Menu.IsValid() || !GetWorld()->GetGameViewport()) return;
    if (auto* Fighter = Cast<AFPSDemoCharacter>(GetPawn())) Fighter->StopCombatInput();
    auto Rows = SNew(SVerticalBox);
    Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 18)
        [SNew(STextBlock).Text(FText::FromString(TEXT("FPS DEMO  /  作战大厅"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 34))];
    auto AddLabel = [&Rows](const FString& Label)
    {
        Rows->AddSlot().AutoHeight().Padding(0, 10, 0, 6)
            [SNew(STextBlock).Text(FText::FromString(Label)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 20)).AutoWrapText(true)];
    };
    auto AddButton = [&Rows](const FString& Label, TFunction<FReply()> Action)
    {
        Rows->AddSlot().AutoHeight().Padding(0, 5)
            [SNew(SButton).ContentPadding(FMargin(14, 10)).OnClicked_Lambda(MoveTemp(Action))
                [SNew(STextBlock).Text(FText::FromString(Label)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 22))]];
    };
    if (IsLobby(GetWorld()))
    {
        AddLabel(TEXT("单机模式 · 输入人机数量（0–9）"));
        Rows->AddSlot().AutoHeight()[SAssignNew(BotInput, SEditableTextBox).Font(FCoreStyle::GetDefaultFontStyle("Regular", 20)).Text(FText::FromString(TEXT("3")))];
        Rows->AddSlot().AutoHeight().Padding(0, 8)
            [SNew(SButton).OnClicked_Lambda([this]() { SelectedDifficulty = EFPSDifficulty((int32(SelectedDifficulty) + 1) % 3); return FReply::Handled(); })
                [SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(SelectedDifficulty == EFPSDifficulty::Easy ? TEXT("单机难度：简单（点击切换）") : SelectedDifficulty == EFPSDifficulty::Hard ? TEXT("单机难度：困难（点击切换）") : TEXT("单机难度：普通（点击切换）")); }).Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))]];
        AddButton(TEXT("开始单机对局 · 5 分钟"), [this]()
        {
            const FString Value = BotInput->GetText().ToString().TrimStartAndEnd();
            bool bValid = !Value.IsEmpty() && Value.Len() <= 1 && Value[0] >= '0' && Value[0] <= '9';
            if (!bValid) { Error = TEXT("请输入 0–9 的整数人机数量。"); return FReply::Handled(); }
            Travel(FString::Printf(TEXT("Mode=Solo?Bots=%d?Difficulty=%s"), FCString::Atoi(*Value), SelectedDifficulty == EFPSDifficulty::Easy ? TEXT("Easy") : SelectedDifficulty == EFPSDifficulty::Hard ? TEXT("Hard") : TEXT("Normal")));
            return FReply::Handled();
        });
        AddButton(TEXT("进入靶场 · 单人"), [this]() { Travel(TEXT("Mode=Training")); return FReply::Handled(); });
        AddLabel(TEXT("联机模式 · 房间名称（最多 24 字符）"));
        Rows->AddSlot().AutoHeight()[SAssignNew(RoomInput, SEditableTextBox).Font(FCoreStyle::GetDefaultFontStyle("Regular", 20)).Text(FText::FromString(TEXT("FPS Room")))];
        AddButton(TEXT("创建局域网房间"), [this]()
        {
            FString Name = RoomInput->GetText().ToString().TrimStartAndEnd().Left(24);
            for (TCHAR& C : Name) if (C == '?' || C == '#' || C == '/' || C == '\\' || C == ':' || C < 32) C = '_';
            Travel(TEXT("listen?Mode=LAN?Room=") + Name);
            return FReply::Handled();
        });
        AddLabel(TEXT("加入房间 · 房主 IPv4 地址，可带端口（默认 7777）"));
        Rows->AddSlot().AutoHeight()[SAssignNew(AddressInput, SEditableTextBox).Font(FCoreStyle::GetDefaultFontStyle("Regular", 20)).Text(FText::FromString(TEXT("127.0.0.1")))];
        AddButton(TEXT("加入房间"), [this]()
        {
            const FString Address = AddressInput->GetText().ToString().TrimStartAndEnd();
            if (!IsValidAddress(Address)) { Error = TEXT("请输入有效 IPv4 地址，如 192.168.1.10:7777。"); return FReply::Handled(); }
            Error = TEXT("正在连接房间……");
            if (auto* Instance = GetGameInstance<UFPSFrontGameInstance>()) Instance->ConnectionError.Empty();
            ClientTravel(Address, TRAVEL_Absolute);
            return FReply::Handled();
        });
    }
    else
    {
        const auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
        if (GS && GS->bWaiting)
        {
            AddButton(TEXT("切换阵营"), [this]() { ServerSwitchTeam(); return FReply::Handled(); });
            if (HasAuthority()) AddButton(TEXT("开始对局 · 5 分钟"), [this]() { ServerStartRound(); return FReply::Handled(); });
        }
        if (GS && !GS->bWaiting && !GS->bFinished)
            AddButton(TEXT("继续游戏"), [this]() { HideMenu(); return FReply::Handled(); });
        AddButton(HasAuthority() && GetNetMode() == NM_ListenServer ? TEXT("关闭房间并返回大厅") : TEXT("返回大厅"), [this]()
        {
            HideMenu();
            UGameplayStatics::OpenLevel(this, TEXT("/Engine/Maps/Entry"), true, TEXT("game=/Script/MCP_Test.FPSLobbyMode"));
            return FReply::Handled();
        });
    }
    Rows->AddSlot().AutoHeight().Padding(0, 18)
        [SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(StatusText()); }).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).AutoWrapText(true)];
    Menu = SNew(SBorder).BorderBackgroundColor(FLinearColor(.015f, .025f, .045f, .97f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SBox).WidthOverride(900).Padding(24)
            [SNew(SScrollBox) + SScrollBox::Slot()[Rows]]];
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
        const bool bCompleteLobby = IsLobby(GetWorld()) && BotInput.IsValid() && RoomInput.IsValid() && AddressInput.IsValid();
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
