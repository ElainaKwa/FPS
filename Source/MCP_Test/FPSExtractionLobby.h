#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "FPSExtractionLobby.generated.h"

/** Saved composition, relative to a camera already authored in the factory map. */
UCLASS()
class MCP_TEST_API UFPSExtractionLobbyPresentation : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Lobby") FName CameraAnchorTag=TEXT("FactoryCamera0");
    UPROPERTY(EditAnywhere, Category="Lobby") FVector CameraOffset=FVector(0,390,-215);
    UPROPERTY(EditAnywhere, Category="Lobby") FVector LookAtOffset=FVector(0,840,-235);
    UPROPERTY(EditAnywhere, Category="Lobby") FVector RobotOffset=FVector(130,810,-245);
    UPROPERTY(EditAnywhere, Category="Lobby") FRotator RobotRotation=FRotator(0,-78,0);
    UPROPERTY(EditAnywhere, Category="Lobby", meta=(ClampMin="20",ClampMax="100")) float FieldOfView=54;
};

/** Presentation only: the playable pawn and its transform are preserved. */
UCLASS()
class MCP_TEST_API UFPSExtractionLobbyScene : public UObject
{
    GENERATED_BODY()
public:
    void SetActive(APlayerController* Owner,UFPSExtractionLobbyPresentation* Composition,bool Active);
    void Release();
    bool IsActive() const {return Camera!=nullptr;}
private:
    TWeakObjectPtr<APlayerController> Controller;
    TWeakObjectPtr<AActor> PreviousTarget;
    TWeakObjectPtr<APawn> HiddenPawn;
    bool bPawnWasHidden=false;
    UPROPERTY(Transient) TObjectPtr<class ACameraActor> Camera;
    UPROPERTY(Transient) TObjectPtr<class AFPSFactoryRobot> Robot;
};

/** Opt-in isolated UI verification, dormant in normal play. */
UCLASS()
class MCP_TEST_API UFPSLobbyVerification : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override {RETURN_QUICK_DECLARE_CYCLE_STAT(FPSLobbyVerification,STATGROUP_Tickables);}
    virtual void Tick(float DeltaSeconds) override;
private:
    int32 Stage=0;
    double LastStep=0;
    bool bFailed=false,bDone=false;
};
