#include "FPSStartMenuVerification.h"
#include "FPSMatchFlow.h"
#include "FPSInterface.h"
#include "FPSFactoryBase.h"
#include "FPSExtractionUI.h"
#include "FPSExtractionSubsystem.h"
#include "FPSDemoGameState.h"
#include "FPSDemoCharacter.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponAbilities.h"
#include "FPSAttachmentData.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "FPSWeaponReloadRig.h"
#include "FPSActionPolicy.h"
#include "FPSBaseMembership.h"
#include "FPSBaseRegion.h"
#include "FPSDemoEffects.h"
#include "FPSDemoPlayerState.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/HUD.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/WidgetPath.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"

bool UFPSStartMenuVerification::IsTickable() const
{
#if !UE_BUILD_SHIPPING
    return !IsTemplate()&&!bDone&&FParse::Param(FCommandLine::Get(),TEXT("FPSStartMenuSmoke"));
#else
    return false;
#endif
}
void UFPSStartMenuVerification::Tick(float)
{
#if !UE_BUILD_SHIPPING
    auto* W=GetWorld();if(!W||W->GetTimeSeconds()<3)return;
    if(Stage==29)if(auto* Owner=W->GetFirstPlayerController())if(APawn* P=Owner->GetPawn())P->AddMovementInput(FVector(0,1,0),1);
    if(Stage==34&&RaidBoundsStage==3)
        if(APawn* P=W->GetFirstPlayerController()->GetPawn())
        {
            TArray<UStaticMeshComponent*> Meshes;P->GetComponents(Meshes);
            for(const auto* Mesh:Meshes)if(Mesh->bOnlyOwnerSee&&Mesh->GetStaticMesh()&&Mesh->GetStaticMesh()->GetFName()==TEXT("SM_EXP_M4_MagazineExtended"))
            {
                bReloadAttachmentMoved|=Mesh->GetAttachParent()&&!Mesh->GetAttachParent()->GetRelativeLocation().IsNearlyZero();
                bReloadAttachmentHidden|=!Mesh->IsVisible();
            }
        }
    if(FPlatformTime::Seconds()-LastStep<1.2)return;
    auto* PC=Cast<AFPSDemoPlayerController>(W->GetFirstPlayerController());
    auto* Front=Cast<AFPSFrontController>(PC);auto* Factory=Cast<AFPSFactoryController>(PC);
    auto* S=GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>();
    auto* Body=PC?Cast<AFPSDemoCharacter>(PC->GetPawn()):nullptr;auto* Gun=Body?Body->GetWeapons():nullptr;
    UUserWidget* UI=Front?static_cast<UUserWidget*>(Front->MenuAsset):Factory?Factory->GetTerminal():nullptr;
    if(!UI&&!Factory)
    {
        TArray<UUserWidget*> Widgets;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this,Widgets,UFPSExtractionUI::StaticClass());
        if(Widgets.Num()==1)UI=Widgets[0];
    }
    auto Check=[&](bool OK,const TCHAR* What){UE_LOG(LogTemp,Display,TEXT("FPS_START_MENU %s stage=%d %s"),OK?TEXT("PASS"):TEXT("FAIL"),Stage,What);bFailed|=!OK;};
    auto Find=[&](const TCHAR* Name){return UI?UI->GetWidgetFromName(Name):nullptr;};
    auto Shown=[&](const TCHAR* Name){auto* Widget=Find(Name);return Widget&&Widget->IsVisible();};
    auto Label=[&](const TCHAR* Name,const TCHAR* Expected){auto* T=Cast<UTextBlock>(Find(Name));return T&&T->GetText().ToString().Contains(Expected);};
    auto Click=[&](const TCHAR* Name)
    {
        auto* B=Cast<UButton>(Find(Name));bool Routed=false;
        if(B&&B->GetIsEnabled()&&B->GetCachedWidget().IsValid())
        {
            auto& App=FSlateApplication::Get();FWidgetPath OwnerPath;
            if(App.GeneratePathToWidgetUnchecked(B->GetCachedWidget().ToSharedRef(),OwnerPath))
            {
                const auto Pos=B->GetCachedGeometry().GetAbsolutePosition()+B->GetCachedGeometry().GetAbsoluteSize()*.5;
                const auto Path=App.LocateWindowUnderMouse(Pos,{OwnerPath.GetWindow()},false);
                if(Path.ContainsWidget(B->GetCachedWidget().Get()))
                {
                    const FPointerEvent Down(0,Pos,Pos,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());
                    const FPointerEvent Up(0,Pos,Pos,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState());
                    App.RoutePointerMoveEvent(Path,Up,false);App.RoutePointerDownEvent(Path,Down);App.RoutePointerUpEvent(Path,Up);Routed=true;
                }
            }
        }
        Check(Routed,Name);
    };
    auto Capture=[&](const TCHAR* Name){FString Dir;FParse::Value(FCommandLine::Get(),TEXT("StartMenuEvidence="),Dir);if(!Dir.IsEmpty())FScreenshotRequest::RequestScreenshot(Dir/FString(Name)+TEXT(".png"),true,false);};
    auto GameplayMouse=[&]()
    {
        const auto* V=W->GetGameViewport();
        Check(PC&&!PC->bShowMouseCursor&&!PC->IsMoveInputIgnored()&&!PC->IsLookInputIgnored()&&V&&V->GetMouseCaptureMode()==EMouseCaptureMode::CapturePermanently&&V->GetMouseLockMode()==EMouseLockMode::LockOnCapture,TEXT("gameplay restores permanent mouse capture and lock"));
        Check(V&&V->Viewport&&V->Viewport->HasMouseCapture(),TEXT("Slate viewport owns mouse capture"));
    };
    auto MenuMouse=[&](){const auto* V=W->GetGameViewport();Check(PC&&PC->bShowMouseCursor&&PC->IsMoveInputIgnored()&&V&&V->GetMouseLockMode()==EMouseLockMode::LockAlways,TEXT("recovery menu keeps cursor inside viewport"));};
    auto PreparedGear=[&](bool MatchAmmo=true)
    {
        const auto Saved=S->GetView().Loadout;
        Check(Gun&&Gun->GetLoadout().Primary.InstanceId==Saved.Primary.InstanceId&&Gun->GetLoadout().Secondary.InstanceId==Saved.Secondary.InstanceId&&Gun->GetLoadout().Primary.AttachmentIds==Saved.Primary.AttachmentIds&&(!MatchAmmo||(Gun->GetLoadout().Primary.Magazine==Saved.Primary.Magazine&&Gun->GetLoadout().Primary.Reserve==Saved.Primary.Reserve)),TEXT("gear matches persisted action identity attachments and expected ammo"));
        bool VisualsMatch=Gun&&Body;
        if(VisualsMatch)for(FName Id:Saved.Primary.AttachmentIds)
        {
            const auto* Part=Gun->Catalog()->FindAttachment(Id);
            if(!Part){VisualsMatch=false;continue;}
            bool First=false,Third=false;TArray<UStaticMeshComponent*> Meshes;Body->GetComponents(Meshes);
            for(const auto* Mesh:Meshes)if(Mesh->IsVisible()&&Mesh->GetStaticMesh())
            {
                First|=Mesh->bOnlyOwnerSee&&Mesh->GetStaticMesh()==Part->FirstPerson.Mesh.Get();
                Third|=Mesh->bOwnerNoSee&&Mesh->GetStaticMesh()==Part->ThirdPerson.Mesh.Get();
            }
            VisualsMatch&=!Part->bUseWeaponPresentation&&First&&Third;
        }
        Check(VisualsMatch&&Saved.Primary.AttachmentIds.Num()==6,TEXT("all six first-person and third-person attachment meshes match action loadout"));
        bool OriginalPartsHidden=true;
        const auto* Rig=Gun&&Gun->Definition()?UFPSWeaponReloadRig::Find(Gun->Definition()->FirstPerson.Mesh.Get()):nullptr;
        if(Body){TArray<UStaticMeshComponent*> Meshes;Body->GetComponents(Meshes);for(const auto* Mesh:Meshes)
            if(Mesh->IsVisible()&&Mesh->GetStaticMesh()&&((Rig&&Mesh->GetStaticMesh()==Rig->Magazine)||Mesh->GetStaticMesh()->GetFName()==TEXT("SM_EXP_M4_MagazineStandard")||Mesh->GetStaticMesh()->GetFName()==TEXT("SM_EXP_M4_MuzzleStandard")))OriginalPartsHidden=false;}
        Check(OriginalPartsHidden,TEXT("installed magazine and muzzle replace their original meshes"));
    };
    if(Stage==0){Check(Front&&Front->MenuAsset&&Front->MenuAsset->Screen==TEXT("Start")&&Label(TEXT("MultiplayerTitle"),TEXT("多人对战"))&&Label(TEXT("RecoveryTitle"),TEXT("回收行动")),TEXT("default Entry opens saved mode selection"));Check(!Find(TEXT("FontNotice")),TEXT("start screen omits font notice"));Capture(TEXT("Start"));}
    if(Stage==1)Click(TEXT("InputSettings"));
    if(Stage==2){Check(PC&&PC->IsSettingsOpen(),TEXT("start settings opens"));if(PC)PC->SettingsBack();}
    if(Stage==3){Check(PC&&!PC->IsSettingsOpen()&&PC->IsMoveInputIgnored(),TEXT("settings returns to start with correct focus lock"));Click(TEXT("Multiplayer"));}
    if(Stage==4){Check(Shown(TEXT("LANPanel"))&&!Shown(TEXT("SoloPanel"))&&Label(TEXT("Title"),TEXT("多人对战")),TEXT("multiplayer defaults to LAN; old controls preserved"));Capture(TEXT("Multiplayer"));}
    if(Stage==5){if(auto* E=Cast<UEditableTextBox>(Find(TEXT("AddressInput"))))E->SetText(FText::FromString(TEXT("invalid")));Click(TEXT("Join"));}
    if(Stage==6){Check(Label(TEXT("Error"),TEXT("IPv4"))&&W->GetGameState<AFPSLobbyState>(),TEXT("invalid join stays in menu and displays error"));Click(TEXT("Modes"));}
    if(Stage==7){Check(Shown(TEXT("Recovery")),TEXT("multiplayer back button"));Click(TEXT("Multiplayer"));}
    if(Stage==8)
    {
        const FKeyEvent Escape(EKeys::Escape,FModifierKeysState(),0,false,0,0);
        FSlateApplication::Get().ProcessKeyDownEvent(Escape);FSlateApplication::Get().ProcessKeyUpEvent(Escape);
    }
    if(Stage==9){Check(Shown(TEXT("Recovery")),TEXT("multiplayer Escape returns to mode selection"));Click(TEXT("Multiplayer"));}
    if(Stage==10)Click(TEXT("Nav2"));
    if(Stage==11){Check(Shown(TEXT("RangePanel")),TEXT("range subpage"));Click(TEXT("Start"));}
    if(Stage==12){auto* GS=W->GetGameState<AFPSDemoGameState>();Check(GS&&GS->bTraining&&W->GetMapName().Contains(TEXT("LV_FPSTraining")),TEXT("range actually loads"));if(Front)Front->ShowMenu();}
    if(Stage==13)Click(TEXT("Return"));
    if(Stage==14){Check(Shown(TEXT("Recovery")),TEXT("range returns to mode selection"));Click(TEXT("Multiplayer"));}
    if(Stage==15)Click(TEXT("Nav0"));
    if(Stage==16){Check(Shown(TEXT("SoloPanel")),TEXT("solo subpage"));if(auto* E=Cast<UEditableTextBox>(Find(TEXT("BotInput"))))E->SetText(FText::FromString(TEXT("0")));Click(TEXT("Start"));}
    if(Stage==17){auto* GS=W->GetGameState<AFPSDemoGameState>();Check(GS&&!GS->bTraining&&!GS->bWaiting&&W->GetMapName().Contains(TEXT("LV_FPSFoundry")),TEXT("solo actually loads"));if(Front)Front->ShowMenu();}
    if(Stage==18)Click(TEXT("Return"));
    if(Stage==19)Click(TEXT("Multiplayer"));
    if(Stage==20)Click(TEXT("Host"));
    if(Stage==21){auto* GS=W->GetGameState<AFPSDemoGameState>();Check(GS&&GS->bWaiting&&W->GetNetMode()==NM_ListenServer, TEXT("host creates actual LAN waiting room"));Capture(TEXT("Host"));}
    if(Stage==22)Click(TEXT("Return"));
    if(Stage==23){Check(Shown(TEXT("Recovery")),TEXT("host returns to mode selection"));Click(TEXT("Recovery"));}
    if(Stage==24){Check(Factory&&Shown(TEXT("OverviewPanel"))&&S->GetPhase()==EFPSExtractionPhase::Base&&!FParse::Param(FCommandLine::Get(),TEXT("FactoryLobby")),TEXT("mode card opens factory lobby using travel options"));Check(Label(TEXT("LobbyDepartLabel"),TEXT("开始行动"))&&Label(TEXT("LobbyPrepareLabel"),TEXT("行动整备")),TEXT("lobby wording"));Capture(TEXT("Recovery"));}
    if(Stage==25){Click(TEXT("LobbyPrepare"));Capture(TEXT("Inventory"));}
    if(Stage==26){Check(Shown(TEXT("InventoryPanel"))&&Label(TEXT("BagCapacity"),TEXT("行动背包")),TEXT("prepare opens inventory with renamed backpack"));Click(TEXT("Overview"));}
    if(Stage==27)
    {
        // The launcher supplies an isolated profile; exercise a real base modification.
        auto* Candidate=S->CopyProfile();Candidate->Loadout.Primary.Magazine=7;Candidate->Loadout.Primary.Reserve=65;
        const FName Items[]={TEXT("Optic"),TEXT("Brake"),TEXT("Magazine"),TEXT("RearGripControl"),TEXT("StockStable"),TEXT("BarrelHeavy")};
        bool Installed=true;for(FName Id:Items)Installed&=S->TryAdd(Candidate->Warehouse,Id,1,64);
        Installed&=S->Commit(Candidate);for(FName Id:Items)Installed&=S->InstallAttachment(Id);
        Check(Installed,TEXT("isolated fixture installs all six attachments through factory service"));
        Click(TEXT("LobbyFactory"));
    }
    if(Stage==28)
    {
        auto* Camera=Body?Body->FindComponentByClass<UCameraComponent>():nullptr;
        Check(Factory&&Body&&PC->GetViewTarget()==Body&&!Body->IsHidden()&&Camera&&Camera->bUsePawnControlRotation&&FVector::Dist(Camera->GetComponentLocation(),Body->GetPawnViewLocation())<100,TEXT("workshop uses first-person character camera"));
        UE_LOG(LogTemp,Display,TEXT("FPS_START_MENU factory HUD=%s show=%d"),*GetNameSafe(PC?PC->GetHUD():nullptr),PC&&PC->GetHUD()?PC->GetHUD()->bShowHUD:0);
        Check(PC&&(!PC->GetHUD()||!PC->GetHUD()->bShowHUD),TEXT("factory HUD disabled"));PreparedGear();GameplayMouse();
        Check(Gun&&!Body->GetActionPolicy()->CanAttack()&&!Gun->CanStart(EFPSWeaponRequest::Fire)&&!Body->GetAbilitySystemComponent()->TryActivateAbilityByClass(UFPSFireAbility::StaticClass()),TEXT("factory rejects both weapon and direct GAS fire"));
        FactoryAmmo=Gun?Gun->Equipped().Magazine:0;WalkStart=Body?Body->GetActorLocation():FVector::ZeroVector;
        if(Gun)Gun->StartFire();Capture(TEXT("FactoryFirstPerson"));
    }
    if(Stage==29)
    {
        Check(Gun&&Gun->Equipped().Magazine==FactoryAmmo&&!Gun->GetLoadout().bFireActive,TEXT("holding fire in factory consumes no ammo"));if(Gun)Gun->StopFire();
        Check(Body&&FVector::Dist2D(Body->GetActorLocation(),WalkStart)>40,TEXT("first-person character can walk in factory"));if(PC)PC->OpenSettings();
    }
    if(Stage==30){Check(PC&&PC->IsSettingsOpen(),TEXT("factory settings opens"));MenuMouse();if(PC)PC->SettingsBack();}
    if(Stage==31){GameplayMouse();if(Factory)Factory->OpenLobby();}
    if(Stage==32){MenuMouse();Click(TEXT("LobbyDepart"));}
    if(Stage==33)
    {
        Check(S->GetPhase()==EFPSExtractionPhase::Raid&&W->GetMapName().Contains(TEXT("LV_FPSIndustrialCrossing")),TEXT("start action loads raid"));GameplayMouse();PreparedGear();
        auto* GS=W->GetGameState<AFPSDemoGameState>();auto* Membership=Body?Body->FindComponentByClass<UFPSBaseMembership>():nullptr;
        bool AtOriginalSpawn=false,RegionsHidden=true;int32 Regions=0,Signs=0;bool SignsHidden=true;
        auto* RaidMode=W->GetAuthGameMode<AFPSExtractionMode>();bool ExitOpposite=false;
        for(TActorIterator<AFPSBaseRegion> It(W);It;++It)
        {++Regions;RegionsHidden&=It->IsHidden();if(It->TeamId==0&&Body)AtOriginalSpawn|=It->Contains(Body->GetActorLocation(),4);if(It->TeamId==1&&RaidMode)ExitOpposite|=It->Contains(RaidMode->ExtractionLocation());}
        Check(AtOriginalSpawn&&ExitOpposite&&RaidMode&&FVector::Dist2D(Body->GetActorLocation(),RaidMode->ExtractionLocation())>9000,TEXT("original west spawn retained with extraction across map on east side"));
        for(TActorIterator<AActor> It(W);It;++It)if(It->ActorHasTag(TEXT("FPSBasePresentation"))){++Signs;SignsHidden&=It->IsHidden();}
        Check(GS&&!GS->bBaseRegionsEnabled&&AtOriginalSpawn&&Membership&&!Membership->IsProtected()&&!Membership->CanSelect()&&Body->GetActionPolicy()->CanAttack(),TEXT("raid spawn has no base protection or free loadout permission"));
        Check(Regions==2&&RegionsHidden&&Signs==2&&SignsHidden,TEXT("raid hides both base boundaries and saved safe-base signs"));
        Check(Gun&&Gun->CanStart(EFPSWeaponRequest::Fire),TEXT("raid permits fire directly at original spawn"));PC->SetControlRotation(FRotator(0,180,0));RaidAmmo=Gun?Gun->Equipped().Magazine:0;if(Gun)Gun->StartFire();Capture(TEXT("Raid"));
    }
    if(Stage==34)
    {
        if(RaidBoundsStage==0)
        {
            if(Gun)Gun->StopFire();Check(Gun&&Gun->Equipped().Magazine<RaidAmmo,TEXT("raid firing at original spawn consumes ammunition"));
            auto* Source=W->SpawnActor<AFPSBotController>();Source->SetActorTickEnabled(false);Source->GetPlayerState<AFPSDemoPlayerState>()->TeamId=1;
            const float Before=Body->GetHealth();auto* ASC=Body->GetAbilitySystemComponent();auto Context=ASC->MakeEffectContext();Context.AddInstigator(Source,Source);
            ASC->ApplyGameplayEffectToSelf(GetDefault<UFPSDemoDamageEffect>(),1,Context);
            Check(FMath::IsNearlyEqual(Body->GetHealth(),Before-25),TEXT("hostile GAS damage applies inside former own base"));Source->Destroy();
            AFPSBaseRegion* Other=nullptr;
            for(TActorIterator<AFPSBaseRegion> It(W);It;++It)
            {
                const FVector Center=It->GetActorLocation();const FVector Destination=Center+FVector(900,0,0);FVector End=Destination;
                Check(!UFPSBaseMembership::ClipAttack(W,Center-FVector(900,0,0),End)&&End.Equals(Destination),TEXT("raid shots cross former base boundaries without clipping"));
                if(It->TeamId==1)Other=*It;
            }
            Check(Other!=nullptr,TEXT("former enemy spawn region exists"));
            if(Other){Body->SetActorLocation(FVector(Other->GetActorLocation().X,0,100));PC->SetControlRotation(FRotator(0,0,0));}
            RaidBoundsStage=1;LastStep=FPlatformTime::Seconds();return;
        }
        if(RaidBoundsStage==1)
        {
            bool InsideEnemy=false;for(TActorIterator<AFPSBaseRegion> It(W);It;++It)if(It->TeamId==1)InsideEnemy|=It->Contains(Body->GetActorLocation());
            Check(InsideEnemy&&!Body->FindComponentByClass<UFPSBaseMembership>()->IsProtected()&&Gun->CanStart(EFPSWeaponRequest::Fire),TEXT("player remains inside former enemy base and can fire"));
            // Capture the settled camera before opening another UI on a later frame.
            Capture(TEXT("RaidFormerBase"));RaidBoundsStage=2;LastStep=FPlatformTime::Seconds();return;
        }
        if(RaidBoundsStage==2)
        {Check(Gun&&Gun->CanStart(EFPSWeaponRequest::Reload),TEXT("modified raid weapon can reload"));if(Gun)Gun->Reload();RaidBoundsStage=3;LastStep=FPlatformTime::Seconds();return;}
        if(RaidBoundsStage==3)
        {
            if(Gun&&Gun->GetLoadout().Action==EFPSWeaponAction::Reloading)return;
            Check(Gun&&Gun->Equipped().Magazine==Gun->EffectiveStats().MagazineSize&&bReloadAttachmentMoved&&bReloadAttachmentHidden,TEXT("extended magazine follows reload motion visibility and capacity"));
            PreparedGear(false);Capture(TEXT("RaidReloaded"));RaidBoundsStage=4;LastStep=FPlatformTime::Seconds();return;
        }
        if(auto* Raid=Cast<AFPSExtractionController>(PC))Raid->ToggleBackpack();
    }
    if(Stage==35){Check(Shown(TEXT("BagPanel")),TEXT("raid backpack opens"));MenuMouse();Click(TEXT("CloseBag"));}
    if(Stage==36){GameplayMouse();if(PC)PC->OpenSettings();}
    if(Stage==37){MenuMouse();if(PC)PC->SettingsBack();}
    if(Stage==38)
    {
        if(ExitRequestedAt==0)
        {
            GameplayMouse();auto* Mode=W->GetAuthGameMode<AFPSExtractionMode>();auto* Raid=Cast<AFPSExtractionController>(PC);
            Check(Mode&&Raid&&FVector::Dist2D(Body->GetActorLocation(),Mode->ExtractionLocation())<=240,TEXT("player reaches relocated east extraction point"));
            if(Mode&&Raid)Mode->Interact(Raid);ExitRequestedAt=FPlatformTime::Seconds();LastStep=ExitRequestedAt;return;
        }
        if(S->GetPhase()!=EFPSExtractionPhase::Result)return;
        Check(FPlatformTime::Seconds()-ExitRequestedAt>=4.8,TEXT("east extraction completes real five-second countdown and settlement"));
    }
    if(Stage==39){Check(S->GetPhase()==EFPSExtractionPhase::Result&&Label(TEXT("ReturnLabel"),TEXT("返回工厂")),TEXT("result wording"));MenuMouse();Capture(TEXT("Result"));}
    if(Stage==40)Click(TEXT("Return"));
    if(Stage==41){Check(Factory&&Shown(TEXT("OverviewPanel"))&&S->GetPhase()==EFPSExtractionPhase::Base,TEXT("settlement automatically reopens factory lobby"));MenuMouse();Capture(TEXT("Returned"));}
    if(Stage==42)Click(TEXT("LobbyFactory"));
    if(Stage==43){GameplayMouse();PreparedGear();Check(Gun&&!Gun->CanStart(EFPSWeaponRequest::Fire),TEXT("factory remains unable to fire after raid return"));Capture(TEXT("ReturnedFactory"));}
    if(Stage==44){if(Factory)Factory->OpenLobby();}
    if(Stage==45)Click(TEXT("LobbyModes"));
    if(Stage==46){Check(Shown(TEXT("Recovery"))&&Shown(TEXT("Multiplayer")),TEXT("factory returns to mode selection"));Capture(TEXT("ReturnedStart"));}
    if(Stage==47){UE_LOG(LogTemp,Display,TEXT("FPS_START_MENU %s COMPLETE"),bFailed?TEXT("FAIL"):TEXT("PASS"));bDone=true;Click(TEXT("Quit"));}
    if(bFailed){UE_LOG(LogTemp,Error,TEXT("FPS_START_MENU FAIL stopping at stage=%d"),Stage);bDone=true;FPlatformMisc::RequestExit(false);}
    ++Stage;LastStep=FPlatformTime::Seconds();
#endif
}
