#include "FPSInterface.h"
#include "FPSExtractionMode.h"
#include "FPSMatchFlow.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "FPSDemoCharacter.h"
#include "FPSMovementComponent.h"
#include "FPSInputSettings.h"
#include "FPSSystemSettings.h"
#include "FPSFrontGameInstance.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponPickup.h"
#include "FPSBaseMembership.h"
#include "FPSModeRules.h"
#include "FPSUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Kismet/GameplayStatics.h"

void UFPSActionButton::Connect()
{
    OnClicked.AddUniqueDynamic(this, &UFPSActionButton::Activate);
    OnHovered.AddUniqueDynamic(this, &UFPSActionButton::Preview);
}
void UFPSActionButton::Activate()
{
    if(auto* UI=GetTypedOuter<UFPSInterface>()) UI->Dispatch(Action, WeaponId);
}
void UFPSActionButton::Preview()
{
    if(!WeaponId.IsNone()) if(auto* UI=GetTypedOuter<UFPSInterface>()) UI->FocusWeapon(WeaponId);
}
UFPSInterface* UFPSInterface::Open(APlayerController* Owner, const TCHAR* Page)
{
    const FString Path=FPSUI::AssetRoot()+FString::Printf(TEXT("Widgets/WBP_FPS%s.WBP_FPS%s_C"),Page,Page);
    auto* Class=LoadClass<UFPSInterface>(nullptr,*Path);
    if(!Class){UE_LOG(LogTemp,Error,TEXT("FPS_UI missing widget asset %s"),*Path);return nullptr;}
    return CreateWidget<UFPSInterface>(Owner,Class);
}
void UFPSInterface::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);
    WidgetTree->ForEachWidget([](UWidget* W){if(auto* B=Cast<UFPSActionButton>(W))B->Connect();});
    if(const auto* Body=Cast<AFPSDemoCharacter>(GetOwningPlayerPawn())) ViewedWeapon=Body->GetWeapons()->GetLoadout().Primary.WeaponId;
    if(ViewedWeapon.IsNone()) ViewedWeapon=TEXT("Weapon.M4");
    UE_LOG(LogTemp,Display,TEXT("FPS_UI asset-ready screen=%s class=%s"),*Screen.ToString(),*GetClass()->GetPathName());
}
bool UFPSInterface::HasControl(FName Name) const {return WidgetTree && WidgetTree->FindWidget(Name)!=nullptr;}
FFPSStaminaHUDState UFPSInterface::ReadStaminaHUD(const AFPSDemoCharacter* Body)
{
    FFPSStaminaHUDState View;
    const auto* Movement=Body?Cast<UFPSMovementComponent>(Body->GetCharacterMovement()):nullptr;
    if(!Body||Body->IsEliminated()||!Movement||!Movement->UsesStamina())return View;
    View.Value=Movement->GetStamina();View.Ratio=FMath::Clamp(View.Value/FMath::Max(1.f,Movement->GetMaxStamina()),0.f,1.f);
    const bool WeakJump=Movement->IsFalling()&&Movement->IsWeakJump();
    const bool SprintBlocked=Movement->IsSprintStartBlocked();
    const bool Low=View.Ratio<=.25f;
    View.Exhausted=View.Value<=KINDA_SMALL_NUMBER;View.Alert=WeakJump||SprintBlocked||Low;
    View.Visible=View.Ratio<1.f-KINDA_SMALL_NUMBER||WeakJump||SprintBlocked;
    View.Status=WeakJump?(View.Exhausted?TEXT("弱跳 · 体力耗尽"):TEXT("弱跳")):View.Exhausted?TEXT("体力耗尽"):Low?TEXT("体力偏低"):TEXT("");
    const bool Recovering=Movement->IsRecovering();
    if(SprintBlocked&&!View.Exhausted)
        View.Status=WeakJump?(Recovering?TEXT("弱跳 · 恢复 · 疾跑受限"):TEXT("弱跳 · 疾跑启动受限"))
            :Recovering?TEXT("恢复中 · 疾跑启动受限"):TEXT("体力偏低 · 疾跑启动受限");
    else if(Recovering)View.Status+=View.Status.IsEmpty()?TEXT("恢复中"):TEXT(" · 恢复中");
    return View;
}
void UFPSInterface::Text(FName Name,const FString& Value)
{
    if(auto* T=Cast<UTextBlock>(WidgetTree->FindWidget(Name))) if(T->GetText().ToString()!=Value)T->SetText(FText::FromString(Value));
}
void UFPSInterface::Visible(FName Name,bool Show)
{
    if(auto* W=WidgetTree->FindWidget(Name))W->SetVisibility(Show ? (W->IsA<UButton>() ? ESlateVisibility::Visible : ESlateVisibility::SelfHitTestInvisible) : ESlateVisibility::Collapsed);
}
void UFPSInterface::Tint(FName Name,FLinearColor Color)
{
    if(auto* T=Cast<UTextBlock>(WidgetTree->FindWidget(Name)))T->SetColorAndOpacity(Color);
    else if(auto* I=Cast<UImage>(WidgetTree->FindWidget(Name)))I->SetColorAndOpacity(Color);
}
FString UFPSInterface::Input(FName Name) const
{
    if(auto* E=Cast<UEditableTextBox>(WidgetTree->FindWidget(Name)))return E->GetText().ToString().TrimStartAndEnd();
    return FString();
}
void UFPSInterface::NativeTick(const FGeometry& Geometry,float DeltaTime)
{
    Super::NativeTick(Geometry,DeltaTime);
    if(!Theme || !GetWorld())return;
    if(Screen==TEXT("Loadout"))RefreshLoadout();
    else if(Screen==TEXT("HUD"))
    {
        RefreshHUD(DeltaTime);
        if(const auto* S=UFPSSystemSettings::Get(this))if(auto* Canvas=Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("HUDCanvas"))))
        {
            for(auto* Child:Canvas->GetAllChildren())if(auto* CanvasSlot=Cast<UCanvasPanelSlot>(Child->Slot))
            {
                const auto P=CanvasSlot->GetPosition();
                const FVector2D Anchor=P.Y>=700?FVector2D(P.X<800?38:1562,858):P.Y<180?FVector2D(P.X>1200?1562:800,23):FVector2D(800,450);
                Child->SetRenderTransformPivot(FVector2D::ZeroVector);Child->SetRenderScale(FVector2D(S->HUDScale));Child->SetRenderTranslation((P-Anchor)*(S->HUDScale-1));
            }
            Text(TEXT("BaseHint"),S->BindingText("Loadout")+TEXT("  配装 / 基地内"));
            Visible(TEXT("PickupKey"),false);
            Text(TEXT("KeyR"),TEXT(""));Visible(TEXT("RKeycap"),false);
            if(const auto* Body=Cast<AFPSDemoCharacter>(GetOwningPlayerPawn());Body&&Body->GetWeapons()->Definition())Text(TEXT("GunName"),Body->GetWeapons()->Definition()->DisplayName.ToString()+TEXT(" · ")+S->BindingText("Reload"));
        }
    }
    else RefreshMenu();
}
void UFPSInterface::Dispatch(FName Action,FName Id)
{
    auto* PC=Cast<AFPSFrontController>(GetOwningPlayer());if(!PC)return;
    if(Action==TEXT("Equip")){ViewedWeapon=Id;PC->RequestSelection(Id);return;}
    if(Action==TEXT("Close")){PC->HideLoadout();return;}
    if(Action==TEXT("Solo")){Tab=0;RefreshMenu();return;}
    if(Action==TEXT("LAN")){Tab=1;RefreshMenu();return;}
    if(Action==TEXT("Range")){Tab=2;RefreshMenu();return;}
    if(Action==TEXT("Help")){bHelp=!bHelp;bInputSettings=false;RefreshMenu();return;}
    if(Action==TEXT("InputSettings")){PC->OpenSettings(this);return;}
    if(Action==TEXT("SprintMode"))
    {
        auto* Settings=UFPSInputSettings::Get();Settings->SetSprintToggle(!Settings->bSprintToggle);RefreshMenu();return;
    }
    if(Action==TEXT("CrouchMode"))
    {
        auto* Settings=UFPSInputSettings::Get();Settings->SetCrouchToggle(!Settings->bCrouchToggle);RefreshMenu();return;
    }
    if(Action==TEXT("Difficulty")){PC->SelectedDifficulty=EFPSDifficulty((int32(PC->SelectedDifficulty)+1)%3);return;}
    if(Action==TEXT("Minus") || Action==TEXT("Plus"))
    {if(auto* E=Cast<UEditableTextBox>(WidgetTree->FindWidget(TEXT("BotInput"))))E->SetText(FText::AsNumber(FMath::Clamp(FCString::Atoi(*Input(TEXT("BotInput")))+(Action==TEXT("Plus")?1:-1),0,9)));return;}
    if(Action==TEXT("Start"))
    {
        if(Tab==2){PC->Travel(TEXT("Mode=Training"));return;}
        const FString Value=Input(TEXT("BotInput"));
        if(Value.Len()!=1 || Value[0]<'0' || Value[0]>'9'){PC->Error=TEXT("请输入 0–9 的整数人机数量。");return;}
        PC->Travel(FString::Printf(TEXT("Mode=Solo?Bots=%d?Difficulty=%s"),FCString::Atoi(*Value),PC->SelectedDifficulty==EFPSDifficulty::Easy?TEXT("Easy"):PC->SelectedDifficulty==EFPSDifficulty::Hard?TEXT("Hard"):TEXT("Normal")));return;
    }
    if(Action==TEXT("Host"))
    {
        FString Name=Input(TEXT("RoomInput")).Left(24);
        for(TCHAR& C:Name)if(C=='?'||C=='#'||C=='/'||C=='\\'||C==':'||C<32)C='_';
        PC->Travel(TEXT("listen?Mode=LAN?Room=")+Name);return;
    }
    if(Action==TEXT("Join"))
    {
        const FString Address=Input(TEXT("AddressInput"));FString Host=Address,Port;bool Valid=true;
        if(Address.Split(TEXT(":"),&Host,&Port))Valid=!Port.IsEmpty()&&Port.IsNumeric()&&FCString::Atoi(*Port)>0&&FCString::Atoi(*Port)<=65535;
        TArray<FString> Parts;Host.ParseIntoArray(Parts,TEXT("."),false);Valid&=Parts.Num()==4;
        for(const auto& P:Parts){Valid&=!P.IsEmpty()&&P.Len()<=3&&FCString::Atoi(*P)<=255;for(TCHAR C:P)Valid&=C>='0'&&C<='9';}
        if(!Valid){PC->Error=TEXT("请输入有效 IPv4 地址，如 192.168.1.10:7777。");return;}
        PC->Error=TEXT("正在连接房间……");if(auto* GI=PC->GetGameInstance<UFPSFrontGameInstance>())GI->ConnectionError.Empty();
        PC->ClientTravel(Address,TRAVEL_Absolute);return;
    }
    if(Action==TEXT("Resume")){PC->HideMenu();return;}
    if(Action==TEXT("SwitchTeam")){PC->ServerSwitchTeam();return;}
    if(Action==TEXT("StartRound")){PC->ServerStartRound();return;}
    if(Action==TEXT("Return")){PC->HideMenu();UGameplayStatics::OpenLevel(PC,TEXT("/Engine/Maps/Entry"),true,TEXT("game=/Script/MCP_Test.FPSLobbyMode"));return;}
    if(Action==TEXT("Quit"))PC->ConsoleCommand(TEXT("quit"));
}
void UFPSInterface::RefreshMenu()
{
    auto* PC=Cast<AFPSFrontController>(GetOwningPlayer());if(!PC)return;
    const auto* GS=GetWorld()->GetGameState<AFPSDemoGameState>();
    FString Error=PC->Error;if(const auto* GI=PC->GetGameInstance<UFPSFrontGameInstance>();GI&&!GI->ConnectionError.IsEmpty())Error=GI->ConnectionError;
    Text(TEXT("Error"),Error);Visible(TEXT("HelpPanel"),bHelp);
    if(const auto* S=UFPSSystemSettings::Get(this);S&&bHelp)
    {
        FString Help;int32 Count=0;for(const auto& B:S->Bindings())
        {Help+=B.Label+TEXT(" ")+S->BindingText(B.Id)+(++Count%3?TEXT("    "):TEXT("\n"));}
        Text(TEXT("HelpText"),Help+TEXT("\nEsc 菜单 / 返回；设置中可修改主副绑定。"));
        if(auto* HelpText=Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("HelpText")))){auto Font=HelpText->GetFont();Font.Size=18;HelpText->SetFont(Font);}
    }
    Visible(TEXT("InputSettingsPanel"),bInputSettings);
    Text(TEXT("InputSettingsLabel"),TEXT("设置"));
    const auto* InputSettings=UFPSInputSettings::Get();
    Text(TEXT("SprintModeLabel"),InputSettings->bSprintToggle?TEXT("切换：再次按下停止"):TEXT("按住：松开停止"));
    Text(TEXT("CrouchModeLabel"),InputSettings->bCrouchToggle?TEXT("切换：再次按下站起"):TEXT("按住：松开站起"));
    Text(TEXT("InputSettingsNote"),TEXT("更改后自动保存到本机。疾跑中断后须重新触发，菜单不会恢复旧动作。"));
    if(Screen==TEXT("Lobby"))
    {
        Visible(TEXT("SoloPanel"),Tab==0);Visible(TEXT("LANPanel"),Tab==1);Visible(TEXT("RangePanel"),Tab==2);
        Visible(TEXT("Start"),Tab!=1);
        Text(TEXT("SectionTitle"),Tab==0?TEXT("单机训练"):Tab==1?TEXT("联机对局"):TEXT("靶场练习"));
        Text(TEXT("SectionNote"),Tab==0?TEXT("本地对局"):Tab==1?TEXT("局域网  /  2–10 人  /  每队最多 5 人"):TEXT("单人场景  /  自由练习  /  随时更换装备"));
        Text(TEXT("StartLabel"),Tab==2?TEXT("进入靶场"):TEXT("开始训练"));
        Text(TEXT("DifficultyLabel"),PC->SelectedDifficulty==EFPSDifficulty::Easy?TEXT("简单"):PC->SelectedDifficulty==EFPSDifficulty::Hard?TEXT("困难"):TEXT("普通"));
        Visible(TEXT("DurationPanel"),Tab!=1);Text(TEXT("DurationValue"),Tab==2?TEXT("--:--"):TEXT("05:00"));
        for(int32 I=0;I<3;++I)
        {
            Visible(FName(*FString::Printf(TEXT("NavAccent%d"),I)),Tab==I);
            const auto Color=Tab==I?Theme->Paper:Theme->Muted;
            Tint(FName(*FString::Printf(TEXT("NavNumber%d"),I)),Color);Tint(FName(*FString::Printf(TEXT("Nav%dTitle"),I)),Color);
        }
        return;
    }
    if(!GS)return;
    const bool Host=PC->HasAuthority();Visible(TEXT("StartRound"),GS->bWaiting&&Host);
    if(auto* B=Cast<UButton>(WidgetTree->FindWidget(TEXT("StartRound"))))
    {int32 N[2]={0,0};for(const APlayerState* P:GS->PlayerArray)if(const auto* PS=Cast<AFPSDemoPlayerState>(P);PS&&PS->TeamId>=0&&PS->TeamId<2)++N[PS->TeamId];B->SetIsEnabled(N[0]>0&&N[1]>0);}
    Text(TEXT("RoomTitle"),GS->RoomName.IsEmpty()?TEXT("铸造场 / FOUNDRY"):GS->RoomName);
    Text(TEXT("SessionNote"),GS->bFinished?TEXT("行动结束 · 本次成绩已结算"):GS->bWaiting?TEXT("双方均有玩家后，房主可以开始对局。"):TEXT("对局仍在进行 · 打开菜单不会暂停计时"));
    Text(TEXT("LeaveNote"),Host&&PC->GetNetMode()==NM_ListenServer?TEXT("房主离开将关闭房间，其他玩家会断开连接。"):TEXT("返回行动终端"));
    if(GS->bFinished)
    {
        if(const auto* PS=PC->GetPlayerState<AFPSDemoPlayerState>())
        {Text(TEXT("PlayerName"),PS->GetPlayerName());Text(TEXT("Kills"),FString::Printf(TEXT("%02d"),PS->GetEliminationCount()));Text(TEXT("Deaths"),FString::Printf(TEXT("%02d"),PS->GetDeathCount()));}
        for(int32 I=0;I<3;++I){Text(FName(*FString::Printf(TEXT("RankName%d"),I)),GS->Podium.IsValidIndex(I)?GS->Podium[I].Name:TEXT("—"));Text(FName(*FString::Printf(TEXT("RankScore%d"),I)),GS->Podium.IsValidIndex(I)?FString::Printf(TEXT("%d 击倒  /  %d 死亡"),GS->Podium[I].Kills,GS->Podium[I].Deaths):TEXT("暂无成绩"));}return;
    }
    for(int32 Team=0;Team<2;++Team)
    {
        int32 Index=0;
        for(const APlayerState* P:GS->PlayerArray)if(const auto* PS=Cast<AFPSDemoPlayerState>(P);PS&&PS->TeamId==Team&&Index<5)
        {
            Text(FName(*FString::Printf(TEXT("Team%dName%d"),Team,Index)),PS->GetPlayerName());
            Text(FName(*FString::Printf(TEXT("Team%dState%d"),Team,Index)),(PS==PC->PlayerState?TEXT("你  "):TEXT(""))+FString(PS->GetPlayerId()==GS->HostPlayerId?TEXT("房主  "):TEXT(""))+FString::Printf(TEXT("%02d 击倒"),PS->GetEliminationCount()));++Index;
        }
        Text(FName(*FString::Printf(TEXT("Team%dCount"),Team)),FString::Printf(TEXT("%d / 5"),Index));
        for(;Index<5;++Index){Text(FName(*FString::Printf(TEXT("Team%dName%d"),Team,Index)),TEXT("等待加入"));Text(FName(*FString::Printf(TEXT("Team%dState%d"),Team,Index)),TEXT("—"));}
    }
}
void UFPSInterface::RefreshLoadout()
{
    auto* PC=Cast<AFPSFrontController>(GetOwningPlayer());const auto* Body=Cast<AFPSDemoCharacter>(GetOwningPlayerPawn());if(!PC||!Body)return;
    const auto* Rules=UFPSModeRules::ForWorld(GetWorld());const auto* Catalog=GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    const auto& L=Body->GetWeapons()->GetLoadout();
    WidgetTree->ForEachWidget([&](UWidget* W)
    {
        auto* B=Cast<UFPSActionButton>(W);if(!B||B->WeaponId.IsNone())return;
        const auto* D=Catalog->Find(B->WeaponId);bool Allowed=false;if(Rules)for(const auto& R:Rules->SelectableWeapons)if(R.Get()==D)Allowed=true;
        B->SetVisibility(Allowed?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        if(B->HasAnyUserFocus())ViewedWeapon=B->WeaponId;
        const bool Equipped=L.Primary.WeaponId==B->WeaponId||L.Secondary.WeaponId==B->WeaponId;
        const FString Key=B->GetName();
        Visible(FName(*(Key+TEXT("Accent"))),Equipped&&D&&D->Slot==EFPSWeaponSlot::Primary);Visible(FName(*(Key+TEXT("Badge"))),Equipped);
        B->SetBackgroundColor(ViewedWeapon==B->WeaponId?FLinearColor(1,1,1):FLinearColor(.94f,.95f,.94f));
    });
    if(const auto* D=Catalog->Find(ViewedWeapon))
    {
        Text(TEXT("WeaponName"),D->WeaponId==TEXT("Weapon.HeavyPistol")?TEXT("重型手枪"):D->DisplayName.ToString());
        Text(TEXT("WeaponType"),D->Slot==EFPSWeaponSlot::Primary?(D->WeaponId==TEXT("Weapon.MP5")||D->WeaponId==TEXT("Weapon.UMP45")?TEXT("主武器  /  冲锋枪"):TEXT("主武器  /  步枪")):TEXT("副武器  /  手枪"));
        Text(TEXT("Magazine"),FString::FromInt(D->MagazineSize));Text(TEXT("Reload"),FString::Printf(TEXT("%.1f s"),D->ReloadDuration));
        const bool Equipped=L.Primary.WeaponId==ViewedWeapon||L.Secondary.WeaponId==ViewedWeapon;
        Text(TEXT("EquippedState"),Equipped?TEXT("已装备"):TEXT("未装备"));Visible(TEXT("DetailCheckFit"),Equipped);Visible(TEXT("DetailBadgeIconBG"),Equipped);
        Tint(TEXT("EquippedState"),Equipped?Theme->Accent:Theme->Muted);
        for(const TCHAR* Edge:{TEXT("Top"),TEXT("Bottom"),TEXT("Left"),TEXT("Right")})Tint(FName(*(FString(TEXT("DetailBadgeFrame"))+Edge)),Equipped?Theme->Accent:Theme->Muted);
        if(auto* I=Cast<UImage>(WidgetTree->FindWidget(TEXT("WeaponPreview"))))I->SetBrushFromTexture(FPSUI::WeaponPreviewTexture(ViewedWeapon));
    }
    const FString Feedback=PC->GetSelectionFeedback();Text(TEXT("Feedback"),Feedback.IsEmpty()?TEXT("//   成功选择后，本局自动保存"):TEXT("//   ")+Feedback);
    Tint(TEXT("Feedback"),Feedback==TEXT("正在处理……")||Feedback.IsEmpty()||PC->LastSelectionSucceeded()?Theme->Paper:Theme->Red);
    const bool Drop=Rules&&Rules->Replacement==EFPSReplacementPolicy::Drop;Visible(TEXT("RuleTail"),!Drop);
    const auto* Settings=UFPSSystemSettings::Get(this);
    Text(TEXT("CloseKeys"),TEXT("Esc"));
    Text(TEXT("LoadoutRule"),Drop?TEXT("旧枪向前轻抛 · ")+(Settings?Settings->BindingText("Interact"):TEXT("F"))+TEXT(" 拾取 · 超出掉落上限时回收最早的枪"):TEXT("首次离开基地后，本次生命不能再次选枪"));
}
void UFPSInterface::RefreshHUD(float DeltaTime)
{
    const auto* PC=Cast<AFPSDemoPlayerController>(GetOwningPlayer());const auto* Body=Cast<AFPSDemoCharacter>(GetOwningPlayerPawn());
    const auto* GS=GetWorld()->GetGameState<AFPSDemoGameState>();if(!GS||!PC)return;
    const auto* Front=Cast<AFPSFrontController>(PC);
    const bool Show=!GS->bWaiting&&!GS->bFinished&&!(Front&&Front->IsLoadoutOpen());
    Visible(TEXT("HUDCanvas"),Show);if(!Show){StaminaPresentation.Initialized=false;return;}
    const auto* PS=PC->GetPlayerState<AFPSDemoPlayerState>();const bool Blue=PS&&PS->TeamId==0;
    const float HP=Body?Body->GetHealth():0,Ratio=Body?FMath::Clamp(HP/FMath::Max(1.f,Body->GetMaxHealth()),0.f,1.f):0;
    Text(TEXT("Health"),FString::Printf(TEXT("%.0f"),HP));Tint(TEXT("Health"),Ratio<=.25f?Theme->Red:Theme->Paper);
    if(auto* Bar=Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("HealthBar")))){Bar->SetPercent(Ratio);Bar->SetFillColorAndOpacity(Ratio<=.25f?Theme->Red:Theme->Paper);}
    Text(TEXT("Team"),Blue?TEXT("蓝队"):TEXT("红队"));Visible(TEXT("TeamBlue"),Blue);Visible(TEXT("TeamRedFit"),!Blue);
    const bool Extraction = GetWorld()->GetAuthGameMode<AFPSExtractionMode>() != nullptr;
    Visible(TEXT("ScoreGroup"),!GS->bTraining&&!Extraction);Visible(TEXT("TrainingLabel"),GS->bTraining);
    if(Extraction){Text(TEXT("Team"),TEXT("回收"));Visible(TEXT("TeamBlue"),false);Visible(TEXT("TeamRedFit"),false);}
    const int32 Sec=FMath::Max(0,FMath::CeilToInt(GS->RoundEndsAt-GS->GetServerWorldTimeSeconds()));Text(TEXT("Timer"),FString::Printf(TEXT("%02d:%02d"),Sec/60,Sec%60));Tint(TEXT("Timer"),Sec<=30?Theme->Accent:Theme->Paper);
    int32 Scores[2]={0,0};for(const APlayerState* P:GS->PlayerArray)if(const auto* State=Cast<AFPSDemoPlayerState>(P);State&&State->TeamId>=0&&State->TeamId<2)Scores[State->TeamId]+=State->GetEliminationCount();
    Text(TEXT("BlueScore"),FString::Printf(TEXT("%02d"),Scores[0]));Text(TEXT("RedScore"),FString::Printf(TEXT("%02d"),Scores[1]));
    const bool Alive=Body&&!Body->IsEliminated();Visible(TEXT("DeathPanel"),Body&&Body->IsEliminated());
    const auto Stamina=ReadStaminaHUD(Body);
    const auto* Movement=Body?Cast<UFPSMovementComponent>(Body->GetCharacterMovement()):nullptr;
    const bool HasStamina=Alive&&Movement&&Movement->UsesStamina();
    if(StaminaOwner.Get()!=Body||!HasStamina){StaminaPresentation.Reset(Stamina.Ratio);StaminaOwner=Body;}
    StaminaPresentation.Update(Stamina.Ratio,DeltaTime);
    if(!HasStamina)StaminaPresentation.Initialized=false;
    const bool ShowStamina=HasStamina&&(Stamina.Visible||!StaminaPresentation.IsSettled());
    Visible(TEXT("StaminaPanel"),ShowStamina);
    if(ShowStamina)
    {
        if(auto* Bar=Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("StaminaBar")))){Bar->SetPercent(StaminaPresentation.White());Bar->SetFillColorAndOpacity(FLinearColor::White);}
        if(auto* Bar=Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("StaminaDelayBar"))))Bar->SetPercent(StaminaPresentation.Ghost());
    }
    if(Body){Text(TEXT("Killer"),TEXT("击倒者  ")+Body->GetKillerName());Text(TEXT("Respawn"),FString::Printf(TEXT("等待重生  %.1f 秒"),Body->GetRespawnSeconds()));}
    Visible(TEXT("KillPanel"),GetWorld()->GetTimeSeconds()<PC->GetKillMarkerUntil());
    Visible(TEXT("DropPanel"),GS->DropNoticeUntil>GS->GetServerWorldTimeSeconds());
    Visible(TEXT("AmmoPanel"),Alive);Visible(TEXT("PickupPanel"),false);Visible(TEXT("BaseHint"),false);
    if(!Alive)return;
    const auto* W=Body->GetWeapons();const auto* D=W->Definition();const auto& Gun=W->Equipped();const auto& State=W->GetLoadout();
    Text(TEXT("GunName"),D?D->DisplayName.ToString():TEXT("未持枪"));Text(TEXT("Ammo"),FString::FromInt(Gun.Magazine));Text(TEXT("Reserve"),FString::Printf(TEXT("/%d"),Gun.Reserve));
    if(auto* I=Cast<UImage>(WidgetTree->FindWidget(TEXT("GunIcon"))))I->SetBrushFromTexture(D?FPSUI::WeaponTexture(D->WeaponId):nullptr);
    const bool Low=D&&Gun.Magazine<=FMath::Max(1,W->EffectiveStats().MagazineSize/5);Tint(TEXT("Ammo"),Low?Theme->Accent:Theme->Paper);
    const FString GunStatus=State.Action==EFPSWeaponAction::Reloading?TEXT("换弹中"):State.Action==EFPSWeaponAction::Switching?TEXT("切换中"):State.Action==EFPSWeaponAction::Inspecting?TEXT("检视中"):Low?TEXT("弹药不足"):TEXT("");
    const bool HasGunStatus=!GunStatus.IsEmpty();Text(TEXT("GunStatus"),GunStatus);
    Visible(TEXT("GunStatus"),HasGunStatus);Visible(TEXT("GunAction"),!HasGunStatus);Visible(TEXT("RKeycap"),!HasGunStatus);Visible(TEXT("KeyR"),!HasGunStatus);
    Visible(TEXT("ReloadBar"),State.Action==EFPSWeaponAction::Reloading);
    if(auto* Bar=Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("ReloadBar")))){Bar->SetPercent(D&&State.Action==EFPSWeaponAction::Reloading?FMath::Clamp(1.0-(State.ActionEndsAt-GS->GetServerWorldTimeSeconds())/FMath::Max(.01,State.ActionEndsAt-State.ActionStartedAt),0.,1.):0);}
    if(const auto* Pickup=W->FindPickup())if(const auto* PD=W->Catalog()->Find(Pickup->GetInstance().WeaponId))
    {const auto* Settings=UFPSSystemSettings::Get(this);Visible(TEXT("PickupPanel"),true);Text(TEXT("PickupText"),(Settings?Settings->BindingText("Interact"):TEXT("F"))+TEXT("  拾取 ")+PD->DisplayName.ToString());}
    if(const auto* Base=Body->FindComponentByClass<UFPSBaseMembership>())Visible(TEXT("BaseHint"),!Extraction&&Base->CanSelect());
}

FReply UFPSInterface::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E)
{
    if(E.GetKey()==EKeys::Escape)
    {
        if(bHelp){bHelp=false;RefreshMenu();return FReply::Handled();}
        if(auto* PC=Cast<AFPSFrontController>(GetOwningPlayer())){PC->ToggleMenu();return FReply::Handled();}
    }
    return Super::NativeOnPreviewKeyDown(G,E);
}
