#include "FPSExtractionLobby.h"
#include "FPSFactoryBase.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void UFPSExtractionLobbyScene::SetActive(APlayerController* Owner,UFPSExtractionLobbyPresentation* Composition,bool Active)
{
    if(!Active){Release();return;}
    if(Camera||!Owner||!Composition||!Owner->GetWorld()->GetAuthGameMode<AFPSFactoryMode>())return;
    auto* World=Owner->GetWorld();AActor* Anchor=nullptr;
    for(TActorIterator<ACameraActor> It(World);It;++It)if(It->ActorHasTag(Composition->CameraAnchorTag)){Anchor=*It;break;}
    if(!Anchor)return;
    const FVector Origin=Anchor->GetActorLocation();FActorSpawnParameters Params;
    Params.ObjectFlags|=RF_Transient;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Robot=World->SpawnActor<AFPSFactoryRobot>(Origin+Composition->RobotOffset,Composition->RobotRotation,Params);
    const FVector Position=Origin+Composition->CameraOffset;
    Camera=World->SpawnActor<ACameraActor>(Position,(Origin+Composition->LookAtOffset-Position).Rotation(),Params);
    if(!Robot||!Camera){Release();return;}
    Robot->SetActorEnableCollision(false);Robot->GetCharacterMovement()->DisableMovement();
    Camera->GetCameraComponent()->SetFieldOfView(Composition->FieldOfView);
    Controller=Owner;PreviousTarget=Owner->GetViewTarget();HiddenPawn=Owner->GetPawn();
    if(HiddenPawn.IsValid()){bPawnWasHidden=HiddenPawn->IsHidden();HiddenPawn->SetActorHiddenInGame(true);}
    Owner->SetViewTarget(Camera);
}
void UFPSExtractionLobbyScene::Release()
{
    if(Controller.IsValid()&&Camera&&Controller->GetViewTarget()==Camera)
        Controller->SetViewTarget(PreviousTarget.IsValid()?PreviousTarget.Get():Controller->GetPawn());
    if(HiddenPawn.IsValid())HiddenPawn->SetActorHiddenInGame(bPawnWasHidden);
    if(Camera)Camera->Destroy();if(Robot)Robot->Destroy();
    Camera=nullptr;Robot=nullptr;Controller.Reset();PreviousTarget.Reset();HiddenPawn.Reset();
}
