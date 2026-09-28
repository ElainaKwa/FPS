#pragma once

#include "CoreMinimal.h"
#include "FPSDemoGameMode.h"
#include "FPSDemoPlayerController.h"
#include "AIController.h"
#include "GameFramework/GameStateBase.h"
#include "FPSMatchFlow.generated.h"

class SWidget;
class SEditableTextBox;

/** Replicated world identity, available to clients where GameMode does not exist. */
UCLASS()
class MCP_TEST_API AFPSLobbyState : public AGameStateBase
{
    GENERATED_BODY()
};

/** Separate front end; existing teaching maps keep their original rules. */
UCLASS()
class MCP_TEST_API AFPSLobbyMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AFPSLobbyMode();
};

UCLASS()
class MCP_TEST_API AFPSMatchMode : public AFPSDemoGameMode
{
    GENERATED_BODY()
public:
    AFPSMatchMode();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void BeginPlay() override;
    virtual void InitGameState() override;
    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual FString InitNewPlayer(APlayerController* NewPlayer, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
    virtual void RestartPlayer(AController* NewPlayer) override;
    void StartRound(APlayerController* Requester);
    void SwitchTeam(APlayerController* Requester);
    bool CanDamage(const AFPSDemoCharacter* Attacker, const AFPSDemoCharacter* Victim) const;
    bool IsCombatActive() const;
private:
    friend struct FFPSFlowSmoke;
    void FinishRound();
    int32 TeamSize(int32 Team) const;
    void SpawnBots();
    int32 BotCount = 3;
    FString ModeName;
    FString RoomName;
    FTimerHandle RoundTimer;
};

UCLASS()
class MCP_TEST_API AFPSBotController : public AAIController
{
    GENERATED_BODY()
public:
    AFPSBotController();
    virtual void Tick(float DeltaSeconds) override;
private:
    friend struct FFPSAISmoke;
    float NextShot = 0;
    float NextSense = 0;
    float NextRoute = 0;
    float LastSeenTime = -10;
    float ReactionUntil = 0;
    float ProgressTime = 0;
    FVector LastSeenLocation = FVector::ZeroVector;
    FVector ProgressLocation = FVector::ZeroVector;
    FVector Destination = FVector::ZeroVector;
    TWeakObjectPtr<AFPSDemoCharacter> Enemy;
    TWeakObjectPtr<APawn> PreviousPawn;
    TArray<FVector> Route;
    int32 RouteIndex = 0;
    int32 PatrolIndex = 0;
    int32 BurstShots = 0;
    bool bVisibleEnemy = false;
};

UCLASS()
class MCP_TEST_API AFPSFrontController : public AFPSDemoPlayerController
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupInputComponent() override;
    UFUNCTION(Server, Reliable) void ServerStartRound();
    UFUNCTION(Server, Reliable) void ServerSwitchTeam();
private:
    void RefreshMenu();
    void ShowMenu();
    void HideMenu();
    void ToggleMenu();
    void Travel(const FString& Options);
    FString StatusText() const;
    TSharedPtr<SWidget> Menu;
    TSharedPtr<SEditableTextBox> BotInput;
    TSharedPtr<SEditableTextBox> AddressInput;
    TSharedPtr<SEditableTextBox> RoomInput;
    FString Error;
    bool bWasWaiting = false;
    UPROPERTY() TObjectPtr<class UInputMappingContext> MenuContext;
    UPROPERTY() TObjectPtr<class UInputAction> MenuAction;
};
