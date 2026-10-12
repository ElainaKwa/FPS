#include "FPSArtCapture.h"
#include "FPSMatchFlow.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoAttributeSet.h"
#include "FPSDemoEffects.h"
#include "FPSWeaponComponent.h"
#include "FPSInterface.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/StaticMeshComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/WidgetPath.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraActor.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void FFPSArtCapture::Tick(AFPSFrontController* PC)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(),TEXT("FPSArtCapture")) || !PC->IsLocalController()) return;
    auto* World=PC->GetWorld();
    struct FRun { TSet<int32> Done; TWeakObjectPtr<ACameraActor> Camera; };
    static TMap<TWeakObjectPtr<UWorld>,FRun> Runs;
    auto& Run=Runs.FindOrAdd(World);
    auto At=[&](int32 Second,TFunction<void()> Action){if(World->GetTimeSeconds()>=Second && !Run.Done.Contains(Second)){Run.Done.Add(Second);Action();}};
    FString Dir;FParse::Value(FCommandLine::Get(),TEXT("FPSArtOutput="),Dir);
    if(Dir.IsEmpty()) Dir=FPaths::ProjectSavedDir()/TEXT("IndustrialArt/Captures");
    auto Capture=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Dir/TEXT("")+Name+TEXT(".png"),true,false);UE_LOG(LogTemp,Display,TEXT("FPS_ART_CAPTURE frame=%s"),Name);};
    auto End=[&](){UE_LOG(LogTemp,Display,TEXT("FPS_ART_CAPTURE PASS sequence-complete"));FPlatformMisc::RequestExit(false);};
    auto* GS=World->GetGameState<AFPSDemoGameState>();
    const bool Interactions=FParse::Param(FCommandLine::Get(),TEXT("FPSArtInteractions"));
    auto Click=[&](UFPSInterface* UI,const TCHAR* Name)
    {
        auto* Button=UI?Cast<UFPSActionButton>(UI->GetWidgetFromName(Name)):nullptr;
        bool Routed=false;
        if(Button && Button->GetCachedWidget().IsValid())
        {
            auto& App=FSlateApplication::Get();FWidgetPath OwnerPath;
            if(App.GeneratePathToWidgetUnchecked(Button->GetCachedWidget().ToSharedRef(),OwnerPath))
            {
                const FVector2D Position=Button->GetCachedGeometry().GetAbsolutePosition()+Button->GetCachedGeometry().GetAbsoluteSize()*.5;
                const auto Path=App.LocateWindowUnderMouse(Position,{OwnerPath.GetWindow()},false);
                if(Path.ContainsWidget(Button->GetCachedWidget().Get()))
                {
                    const FPointerEvent Down(0,Position,Position,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());
                    const FPointerEvent Up(0,Position,Position,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState());
                    App.RoutePointerMoveEvent(Path,Up,false);
                    App.RoutePointerDownEvent(Path,Down);App.RoutePointerUpEvent(Path,Up);Routed=true;
                }
            }
        }
        UE_LOG(LogTemp,Display,TEXT("FPS_UI_ACTION %s pointer-button=%s"),Routed?TEXT("PASS"):TEXT("FAIL"),Name);
    };
    auto Check=[&](bool OK,const TCHAR* Name){UE_LOG(LogTemp,Display,TEXT("FPS_UI_ACTION %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),Name);};
    if(!GS)
    {
        if(Interactions)
        {
            At(5,[&](){Click(PC->MenuAsset,TEXT("Nav1"));});
            At(6,[&](){Check(PC->MenuAsset&&PC->MenuAsset->GetWidgetFromName(TEXT("LANPanel"))->IsVisible(),TEXT("LAN-tab-visible"));Capture(TEXT("LAN"));});
            At(7,[&](){if(auto* E=PC->MenuAsset?Cast<UEditableTextBox>(PC->MenuAsset->GetWidgetFromName(TEXT("AddressInput"))):nullptr)E->SetText(FText::FromString(TEXT("invalid")));Click(PC->MenuAsset,TEXT("Join"));Check(!PC->Error.IsEmpty(),TEXT("invalid-address-feedback"));PC->Error.Empty();Click(PC->MenuAsset,TEXT("Nav2"));});
            At(8,[&](){Check(PC->MenuAsset&&PC->MenuAsset->GetWidgetFromName(TEXT("RangePanel"))->IsVisible(),TEXT("range-tab-visible"));Capture(TEXT("Range"));});
            At(9,[&](){Click(PC->MenuAsset,TEXT("Nav0"));});
            At(10,[&](){Click(PC->MenuAsset,TEXT("Plus"));if(auto* E=PC->MenuAsset?Cast<UEditableTextBox>(PC->MenuAsset->GetWidgetFromName(TEXT("BotInput"))):nullptr)Check(E->GetText().ToString()==TEXT("4"),TEXT("bot-stepper"));Click(PC->MenuAsset,TEXT("Difficulty"));Click(PC->MenuAsset,TEXT("Help"));});
            At(11,[&](){Check(PC->MenuAsset&&PC->MenuAsset->GetWidgetFromName(TEXT("HelpPanel"))->IsVisible(),TEXT("help-expanded"));Capture(TEXT("Help"));});
            At(12,[&](){Click(PC->MenuAsset,TEXT("Help"));});
            At(13,[&](){Capture(TEXT("Lobby"));});At(15,End);
        }
        else{At(10,[&](){Capture(TEXT("Lobby"));});At(13,End);}
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("FPSArtReport")))
    {At(10,[&](){if(auto* Mode=World->GetAuthGameMode<AFPSMatchMode>())Mode->FinishRound();});At(12,[&](){Capture(TEXT("Report"));});At(15,End);return;}
    if(GS->bWaiting){At(10,[&](){Capture(TEXT("Waiting"));});At(13,End);return;}
    auto* Body=Cast<AFPSDemoCharacter>(PC->GetPawn());if(!Body)return;
    auto* W=Body->GetWeapons();
    if(FParse::Param(FCommandLine::Get(),TEXT("FPSArtFontCompare")))
    {
        At(3,[&](){Body->SetActorLocation(FVector(0,0,110));PC->SetControlRotation(FRotator::ZeroRotator);});
        At(6,[&](){Capture(TEXT("HUD"));});At(8,[&](){PC->ToggleLoadout();});
        At(10,[&](){Check(PC->IsLoadoutOpen(),TEXT("font-comparison-loadout-visible"));Capture(TEXT("Loadout"));});At(12,End);return;
    }
    auto CheckHands=[&]()
    {
        int32 HiddenPlaceholders=0,VisibleGloves=0;
        TInlineComponentArray<UStaticMeshComponent*> Meshes(Body);
        for(auto* Mesh:Meshes)
        {
            const FName Name=Mesh->GetFName();
            if((Name==TEXT("FPLeftHand")||Name==TEXT("FPRightHand"))&&(!Mesh->IsVisible()||Mesh->bHiddenInGame))++HiddenPlaceholders;
            if((Name==TEXT("Avatar_FPLGlove")||Name==TEXT("Avatar_FPRGlove"))&&Mesh->IsVisible()&&!Mesh->bHiddenInGame)++VisibleGloves;
        }
        Check(HiddenPlaceholders==2&&VisibleGloves==2,TEXT("legacy-hand-spheres-hidden-current-gloves-visible"));
    };
    if(FParse::Param(FCommandLine::Get(),TEXT("FPSUIScroll")))
    {
        auto Wheel=[&](float Delta)
        {
            auto* Scroll=PC->LoadoutAsset?Cast<UScrollBox>(PC->LoadoutAsset->GetWidgetFromName(TEXT("PrimaryScroll"))):nullptr;
            bool Handled=false;
            if(Scroll&&Scroll->GetCachedWidget().IsValid())
            {
                auto& App=FSlateApplication::Get();FWidgetPath OwnerPath;
                if(App.GeneratePathToWidgetUnchecked(Scroll->GetCachedWidget().ToSharedRef(),OwnerPath))
                {
                    const FVector2D Pos=Scroll->GetCachedGeometry().GetAbsolutePosition()+Scroll->GetCachedGeometry().GetAbsoluteSize()*.5;
                    const auto Path=App.LocateWindowUnderMouse(Pos,{OwnerPath.GetWindow()},false);
                    const FPointerEvent Event(0,Pos,Pos,TSet<FKey>{},EKeys::Invalid,Delta,FModifierKeysState());
                    App.RoutePointerMoveEvent(Path,Event,false);Handled=App.RouteMouseWheelOrGestureEvent(Path,Event).IsEventHandled();
                }
            }
            Check(Handled,TEXT("primary-list-wheel-routed"));
        };
        At(5,[&](){PC->ToggleLoadout();});At(6,[&](){Capture(TEXT("Loadout"));});
        At(7,[&](){Wheel(-30);});
        At(8,[&](){auto* Scroll=Cast<UScrollBox>(PC->LoadoutAsset->GetWidgetFromName(TEXT("PrimaryScroll")));Check(Scroll&&Scroll->GetScrollOffset()>100,TEXT("primary-list-scrolled"));Capture(TEXT("LoadoutScrolled"));});
        At(9,[&](){Click(PC->LoadoutAsset,TEXT("CardUMP45"));});
        At(10,[&](){Check(W->GetLoadout().Primary.WeaponId==TEXT("Weapon.UMP45")&&PC->LastSelectionSucceeded(),TEXT("scrolled-card-equipped-UMP45"));Capture(TEXT("LoadoutSelected"));});
        At(11,[&](){Wheel(30);});At(12,[&](){Click(PC->LoadoutAsset,TEXT("CardM4"));});
        At(13,[&](){Check(W->GetLoadout().Primary.WeaponId==TEXT("Weapon.M4"),TEXT("scroll-return-equipped-M4"));Click(PC->LoadoutAsset,TEXT("Close"));Check(!PC->IsLoadoutOpen(),TEXT("widget-close"));});At(14,End);return;
    }
    At(8,[&](){Capture(TEXT("HUD"));});
    At(10,[&](){PC->ToggleLoadout();});At(11,[&](){Capture(TEXT("Loadout"));});
    if(Interactions)At(12,[&](){Click(PC->LoadoutAsset,TEXT("CardAK"));});
    At(13,[&](){if(Interactions){Check(W->GetLoadout().Primary.WeaponId==TEXT("Weapon.AK")&&PC->LastSelectionSucceeded(),TEXT("widget-button-equipped-AK"));Click(PC->LoadoutAsset,TEXT("Close"));Check(!PC->IsLoadoutOpen(),TEXT("widget-close"));}else PC->HideLoadout();PC->ToggleMenu();});At(14,[&](){Capture(TEXT("Pause"));});
    At(16,[&](){PC->HideMenu();W->AimOn();});At(17,[&](){Capture(TEXT("Aim"));});
    At(19,[&](){W->AimOff();W->StartFire();});At(22,[&](){W->StopFire();Capture(TEXT("LowAmmo"));});
    At(23,[&](){W->Reload();});At(24,[&](){Capture(TEXT("Reload"));});
    At(26,[&](){W->Inspect();});At(27,[&](){Check(W->GetLoadout().Action==EFPSWeaponAction::Inspecting,TEXT("inspect-pose-active"));CheckHands();Capture(TEXT("Inspect"));});
    At(29,[&](){W->Secondary();});At(30,[&](){CheckHands();Capture(TEXT("Pistol"));});
    if(FParse::Param(FCommandLine::Get(),TEXT("FPSArtQuick"))){At(31,End);return;}
    const TCHAR* Names[]={TEXT("AK"),TEXT("BR3"),TEXT("MP5"),TEXT("UMP45"),TEXT("HeavyPistol"),TEXT("M4")};
    for(int32 I=0;I<6;++I)
    {At(32+I*3,[&,I](){PC->RequestSelection(FName(*(FString(TEXT("Weapon."))+Names[I])));});At(33+I*3,[&,I](){Capture(Names[I]);});}
    At(51,[&](){Body->Crouch();Run.Camera=World->SpawnActor<ACameraActor>();if(Run.Camera.IsValid()){const FVector P=Body->GetActorLocation()+FVector(180,160,25);Run.Camera->SetActorLocation(P);Run.Camera->SetActorRotation((Body->GetActorLocation()-P).Rotation());PC->SetViewTarget(Run.Camera.Get());}});
    At(53,[&](){Capture(TEXT("CharacterCrouch"));});At(55,[&](){Body->UnCrouch();});At(57,[&](){Capture(TEXT("Character"));});
    At(59,[&](){PC->SetViewTarget(Body);if(Run.Camera.IsValid())Run.Camera->Destroy();Body->GetAbilitySystemComponent()->SetNumericAttributeBase(UFPSDemoAttributeSet::GetHealthAttribute(),20);});
    At(60,[&](){Capture(TEXT("LowHealth"));});
    At(62,[&](){auto* ASC=Body->GetAbilitySystemComponent();auto Spec=ASC->MakeOutgoingSpec(UFPSWeaponDamageEffect::StaticClass(),1,ASC->MakeEffectContext());Spec.Data->SetSetByCallerMagnitude(FName(TEXT("WeaponDamage")),-200);ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());});
    At(63,[&](){UE_LOG(LogTemp,Display,TEXT("FPS_ART_CAPTURE %s death-state"),Body->IsEliminated() ? TEXT("PASS") : TEXT("FAIL"));Capture(TEXT("Death"));});At(67,End);
#endif
}
