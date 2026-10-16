#pragma once
#include "CoreMinimal.h"
#include "FPSExtractionMode.h"
#include "FPSDemoPlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/HUD.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "FPSFactoryBase.generated.h"

/** Map-authored facility: only the four existing terminal pages are accepted. */
UCLASS()
class MCP_TEST_API AFPSFactoryStation : public AActor
{
    GENERATED_BODY()
public:
    AFPSFactoryStation();
    UPROPERTY(EditAnywhere, Category="Factory") FName Page=TEXT("Overview");
    UPROPERTY(EditAnywhere, Category="Factory") FText DisplayName;
};

/** Cosmetic lobby robot. The walkable factory uses the normal first-person pawn. */
UCLASS()
class MCP_TEST_API AFPSFactoryRobot : public ACharacter
{
    GENERATED_BODY()
public:
    AFPSFactoryRobot();
    virtual void Tick(float DeltaSeconds) override;
private:
    void Segment(FName Name,const FVector& From,const FVector& To);
    UPROPERTY() TObjectPtr<class USpringArmComponent> Boom;
    UPROPERTY() TObjectPtr<class UCameraComponent> Camera;
    UPROPERTY() TMap<FName,TObjectPtr<class UStaticMeshComponent>> Parts;
    float Gait=0;
};

UCLASS()
class MCP_TEST_API UFPSFactoryTerminalShell : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<class UFPSExtractionUI> Terminal;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    UFUNCTION() void CloseTerminal();
};

UCLASS()
class MCP_TEST_API AFPSFactoryController : public AFPSDemoPlayerController
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    AFPSFactoryStation* FocusStation() const;
    bool InteractWithStation();
    bool OpenLobby();
    void CloseTerminal();
    bool IsTerminalOpen() const {return Shell!=nullptr;}
    UFPSExtractionUI* GetTerminal() const;
protected:
    virtual bool LockMenuMouseToViewport() const override { return true; }
private:
    bool OpenTerminal(FName Page);
    bool bLobbyStartupHandled=false;
    UPROPERTY() TObjectPtr<UFPSFactoryTerminalShell> Shell;
};

/** Retained for existing serialized references; the factory mode no longer creates a HUD. */
UCLASS()
class MCP_TEST_API AFPSFactoryHUD : public AHUD
{
    GENERATED_BODY()
public:
    AFPSFactoryHUD();
};

/** Opt-in playable factory. Existing extraction launcher and combat modes are unchanged. */
UCLASS()
class MCP_TEST_API AFPSFactoryMode : public AFPSExtractionMode
{
    GENERATED_BODY()
public:
    AFPSFactoryMode();
    virtual void InitGameState() override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* Player) override;
    virtual void RestartPlayer(AController* Player) override;
private:
    void SyncPreparedEquipment();
    int32 PreparedRevision=INDEX_NONE;
    TWeakObjectPtr<class AFPSDemoCharacter> EquippedPawn;
    void SmokeTick();
    int32 SmokeStage=0;
    float SmokeAt=0;
    bool bSmokeFailed=false;
    FVector SmokeWalkStart;
};

/** Test-only travel observation; dormant unless the isolated factory smoke flag is present. */
UCLASS()
class MCP_TEST_API UFPSFactoryVerification : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override {RETURN_QUICK_DECLARE_CYCLE_STAT(FPSFactoryVerification,STATGROUP_Tickables);}
    virtual void Tick(float DeltaSeconds) override;
    bool bAwaitingReturn=false;
private:
    bool bResultObserved=false;
};
