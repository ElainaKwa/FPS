#pragma once
#include "CoreMinimal.h"
#include "FPSMatchFlow.h"
#include "FPSExtractionMode.generated.h"

UCLASS()
class MCP_TEST_API AFPSExtractionLoot : public AActor
{
    GENERATED_BODY()
public:
    AFPSExtractionLoot();
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY() FName ItemId;
    UPROPERTY() int32 Quantity = 0;
};

UCLASS()
class MCP_TEST_API AFPSExtractionMode : public AFPSMatchMode
{
    GENERATED_BODY()
public:
    AFPSExtractionMode();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void InitGameState() override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void RestartPlayer(AController* Player) override;
    virtual void HandleLifeEnded(class UFPSLifeComponent* Life, const struct FFPSDeathContext& Context) override;
    void Interact(class AFPSExtractionController* Player);
    bool DropCargo(class AFPSExtractionController* Player, FName Id, int32 Quantity);
    bool IsRaid() const { return bRaid; }
    FVector ExtractionLocation() const { return ExitLocation; }
private:
    friend struct FFPSExtractionSmoke;
    AFPSExtractionLoot* FocusLoot(const class AFPSDemoCharacter* Body) const;
    AFPSExtractionLoot* SpawnCargo(FName Id, int32 Quantity, const FVector& Location);
    bool bRaid = false;
    bool bRestored = false;
    FVector ExitLocation = FVector(4920, 0, 90);
    float ExitStarted = -1;
};

UCLASS()
class MCP_TEST_API AFPSExtractionController : public AFPSFrontController
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupInputComponent() override;
    bool IsExtractionPanelOpen() const;
    void ToggleBackpack();
    void Interact();
    void EscapeMenu();
protected:
    virtual bool LockMenuMouseToViewport() const override { return true; }
private:
    uint32 SettingsMappingRevision=MAX_uint32;
    UPROPERTY() TObjectPtr<class UFPSExtractionUI> ExtractionUI;
    UPROPERTY() TObjectPtr<class UInputMappingContext> ExtractionContext;
    UPROPERTY() TObjectPtr<class UInputAction> BackpackAction;
    UPROPERTY() TObjectPtr<class UInputAction> EscapeAction;
    UPROPERTY() TObjectPtr<class UInputAction> InteractAction;
    bool bPreviousPanel = false;
    bool bPanelInputInitialized = false;
};
