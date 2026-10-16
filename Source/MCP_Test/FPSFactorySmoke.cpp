#include "FPSFactoryBase.h"
#include "FPSExtractionSubsystem.h"
#include "FPSExtractionUI.h"
#include "FPSDemoCharacter.h"
#include "Camera/CameraActor.h"
#include "FPSDemoGameState.h"
#include "Components/CanvasPanel.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"
#include "Widgets/SWidget.h"

void AFPSFactoryMode::SmokeTick()
{
#if !UE_BUILD_SHIPPING
    if(!FParse::Param(FCommandLine::Get(),TEXT("FPSFactorySmoke"))||GetWorld()->GetTimeSeconds()<3)return;
    auto* Probe=GetGameInstance()->GetSubsystem<UFPSFactoryVerification>();if(Probe->bAwaitingReturn)return;
    auto* PC=Cast<AFPSFactoryController>(GetWorld()->GetFirstPlayerController());auto* Body=PC?Cast<AFPSDemoCharacter>(PC->GetPawn()):nullptr;
    if(!Body)return;
    const float Now=GetWorld()->GetTimeSeconds();
    auto Check=[&](bool Good,const TCHAR* Description){UE_LOG(LogTemp,Display,TEXT("FPS_FACTORY %s %s"),Good?TEXT("PASS"):TEXT("FAIL"),Description);bSmokeFailed|=!Good;};
    auto Capture=[&](const TCHAR* Name){FString Out;FParse::Value(FCommandLine::Get(),TEXT("FactoryEvidence="),Out);if(!Out.IsEmpty())FScreenshotRequest::RequestScreenshot(Out/FString(Name)+TEXT(".png"),true,false);};
    if(SmokeStage==1&&Now-SmokeAt<1.2f){Body->AddMovementInput(FVector(0,1,0),1);return;}
    if(Now-SmokeAt<.8f)return;
    if(SmokeStage==0)
    {
        Check(GetNetMode()==NM_Standalone&&GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>()->GetPhase()==EFPSExtractionPhase::Base,TEXT("walkable factory enters personal base phase"));
        Check(GetGameState<AFPSDemoGameState>()->WeaponDataError.IsEmpty(),TEXT("weapon catalog ready without combat setup"));
        int32 Stations=0;for(TActorIterator<AFPSFactoryStation> It(GetWorld());It;++It)++Stations;Check(Stations==4,TEXT("four saved facilities loaded"));
        Check(!PC->InteractWithStation(),TEXT("distant facilities cannot interact"));SmokeWalkStart=Body->GetActorLocation();Capture(TEXT("FactoryArrival"));
    }
    if(SmokeStage==1)
    {
        Check(Body->GetActorLocation().Y-SmokeWalkStart.Y>180&&Body->GetCharacterMovement()->IsMovingOnGround(),TEXT("character walks on saved floor with collision"));
        FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(FactoryWalls),false,Body);
        Check(GetWorld()->SweepSingleByChannel(Hit,FVector(0,0,90),FVector(1600,0,90),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,80),Query),TEXT("east perimeter blocks capsule"));
        Check(GetWorld()->SweepSingleByChannel(Hit,FVector(0,0,90),FVector(-1600,0,90),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,80),Query),TEXT("west perimeter blocks capsule"));
        Check(GetWorld()->SweepSingleByChannel(Hit,FVector(0,0,90),FVector(0,1300,90),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,80),Query),TEXT("departure door blocks capsule"));
    }
    // Each real station is approached, opened via the same interaction method, and closed through Slate Tab routing.
    if(SmokeStage>=2&&SmokeStage<=13)
    {
        const FName Pages[]={TEXT("Warehouse"),TEXT("Production"),TEXT("Modification"),TEXT("Overview")};const int32 Index=(SmokeStage-2)/3,Step=(SmokeStage-2)%3;
        AFPSFactoryStation* Station=nullptr;for(TActorIterator<AFPSFactoryStation> It(GetWorld());It;++It)if(It->Page==Pages[Index])Station=*It;
        if(!Station){Check(false,TEXT("station page exists"));}
        else if(Step==0)
        {
            const FVector To=Station->GetActorLocation()-Station->GetActorForwardVector()*170;Body->SetActorLocation(FVector(To.X,To.Y,94));
            Body->GetCharacterMovement()->StopMovementImmediately();PC->SetControlRotation(FRotator(-10,Station->GetActorRotation().Yaw,0));
            Check(PC->FocusStation()==Station,TEXT("nearby unobstructed station can focus"));Check(PC->InteractWithStation()&&PC->IsTerminalOpen()&&PC->IsMoveInputIgnored(),TEXT("facility opens existing terminal and locks movement"));
        }
        else if(Step==1)
        {const TCHAR* Names[]={TEXT("FactoryWarehouse"),TEXT("FactoryProduction"),TEXT("FactoryGunsmith"),TEXT("FactoryDeparture")};Capture(Names[Index]);}
        else
        {
            if(auto* UI=PC->GetTerminal())
            {auto* Parent=UI->GetParent();auto* Shell=Parent?Cast<UFPSFactoryTerminalShell>(Parent->GetOuter()):nullptr;
             // The wrapper owns the widget tree, while UMG children are outered to their own user widget.
             if(!Shell&&Parent)Shell=Parent->GetTypedOuter<UFPSFactoryTerminalShell>();
             Check(Shell!=nullptr,TEXT("facility close wrapper exists"));
             if(Shell){const FKeyEvent Tab(EKeys::Tab,FModifierKeysState(),0,false,0,0);Check(Shell->TakeWidget()->OnPreviewKeyDown(Shell->GetCachedGeometry(),Tab).IsEventHandled(),TEXT("Tab routes through terminal wrapper"));}}
            Check(!PC->IsTerminalOpen()&&!PC->IsMoveInputIgnored(),TEXT("closing terminal returns movement"));
        }
    }
    if(SmokeStage==14)
    {
        Body->SetActorLocation(FVector(0,-720,94));PC->SetControlRotation(FRotator(-10,90,0));
        Check(!PC->InteractWithStation(),TEXT("interaction remains distance gated after closing"));
        PC->OpenSettings();Check(PC->IsSettingsOpen()&&PC->IsMoveInputIgnored(),TEXT("factory settings open"));PC->SettingsBack();
        Check(!PC->IsSettingsOpen()&&!PC->IsMoveInputIgnored(),TEXT("closing settings restores walking"));
    }
    if(SmokeStage==15){Capture(TEXT("FactoryWalkable"));}
    if(SmokeStage>=16&&SmokeStage<=23)
    {
        const int32 Index=(SmokeStage-16)/2;
        if((SmokeStage-16)%2==0)
        {
            ACameraActor* Camera=nullptr;const FName Tag(*FString::Printf(TEXT("FactoryCamera%d"),Index));
            for(TActorIterator<ACameraActor> It(GetWorld());It;++It)if(It->ActorHasTag(Tag))Camera=*It;
            Check(Camera!=nullptr,TEXT("saved factory overview camera exists"));if(Camera)PC->SetViewTarget(Camera);
        }
        else{const TCHAR* Names[]={TEXT("FactoryWorkshop"),TEXT("FactoryBench"),TEXT("FactoryFabrication"),TEXT("FactoryStorage")};Capture(Names[Index]);}
    }
    if(SmokeStage==24)PC->SetViewTarget(Body);
    if(SmokeStage==25)
    {
        if(bSmokeFailed){FPlatformMisc::RequestExit(false);SmokeStage=100;return;}
        Probe->bAwaitingReturn=true;
        Check(GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>()->StartExpedition(),TEXT("factory starts real extraction travel"));
    }
    ++SmokeStage;SmokeAt=Now;
#endif
}
bool UFPSFactoryVerification::IsTickable() const
{
#if !UE_BUILD_SHIPPING
    return !IsTemplate()&&FParse::Param(FCommandLine::Get(),TEXT("FPSFactorySmoke"));
#else
    return false;
#endif
}
void UFPSFactoryVerification::Tick(float)
{
#if !UE_BUILD_SHIPPING
    if(!bAwaitingReturn||!GetWorld()||GetWorld()->GetTimeSeconds()<5)return;
    auto* Loop=GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>();
    if(auto* Mode=GetWorld()->GetAuthGameMode<AFPSExtractionMode>();Mode&&Mode->IsRaid())
    {
        if(Loop->GetPhase()==EFPSExtractionPhase::Raid)
        {
            const bool Good=Cast<AFPSDemoCharacter>(UGameplayStatics::GetPlayerPawn(this,0))&&Loop->FinishExpedition(false);
            UE_LOG(LogTemp,Display,TEXT("FPS_FACTORY %s arrived in real raid and settled isolated expedition"),Good?TEXT("PASS"):TEXT("FAIL"));
            if(!Good){FPlatformMisc::RequestExit(false);return;}bResultObserved=true;
        }
        else if(bResultObserved&&Loop->GetPhase()==EFPSExtractionPhase::Result)
        {if(!Loop->ReturnToBase()){UE_LOG(LogTemp,Display,TEXT("FPS_FACTORY FAIL return travel"));FPlatformMisc::RequestExit(false);}}
    }
    else if(bResultObserved&&GetWorld()->GetAuthGameMode<AFPSFactoryMode>())
    {
        const bool Good=Cast<AFPSDemoCharacter>(UGameplayStatics::GetPlayerPawn(this,0))&&Loop->GetPhase()==EFPSExtractionPhase::Base;
        UE_LOG(LogTemp,Display,TEXT("FPS_FACTORY %s raid returns to walkable factory"),Good?TEXT("PASS"):TEXT("FAIL"));
        if(Good)UE_LOG(LogTemp,Display,TEXT("FPS_FACTORY COMPLETE"));FPlatformMisc::RequestExit(false);
    }
#endif
}
