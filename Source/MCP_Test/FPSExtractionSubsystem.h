#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "FPSExtractionTypes.h"
#include "FPSExtractionSubsystem.generated.h"

UCLASS()
class MCP_TEST_API UFPSExtractionSave : public USaveGame
{
    GENERATED_BODY()
public:
    // Keep the legacy CDO default: v1 saves may omit a property equal to its default.
    UPROPERTY() int32 Version = 1;
    UPROPERTY() int32 Revision = 0;
    UPROPERTY() TArray<FFPSExtractionStack> Warehouse;
    UPROPERTY() TArray<FFPSExtractionStack> PreparedBackpack;
    UPROPERTY() FFPSWeaponLoadout Loadout;
    UPROPERTY() int32 Credits = 60;
    UPROPERTY() bool bExpeditionActive = false;
    UPROPERTY() FName ProductionRecipe;
    UPROPERTY() FDateTime ProductionEnds;
};

/** Standalone experiment, explicitly rejects remote/client use. */
UCLASS()
class MCP_TEST_API UFPSExtractionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    UFUNCTION(BlueprintPure) FFPSExtractionView GetView() const;
    UFUNCTION(BlueprintPure) TArray<FFPSExtractionItem> GetItems() const { return Items; }
    UFUNCTION(BlueprintPure) TArray<FFPSExtractionRecipe> GetRecipes() const { return Recipes; }
    UFUNCTION(BlueprintPure) FText ItemName(FName Id) const;
    UFUNCTION(BlueprintCallable) bool StartExpedition();
    UFUNCTION(BlueprintCallable) bool ReturnToBase();
    UFUNCTION(BlueprintCallable) bool Sell(FName ItemId, int32 Quantity = 1);
    UFUNCTION(BlueprintCallable) bool StartProduction(FName RecipeId);
    UFUNCTION(BlueprintCallable) bool ClaimProduction();
    UFUNCTION(BlueprintCallable) bool InstallAttachment(FName ItemId);
    UFUNCTION(BlueprintCallable) bool RemoveAttachment(FName AttachmentId);
    UFUNCTION(BlueprintCallable) bool Resupply();
    UFUNCTION(BlueprintCallable) bool DropItem(FName ItemId, int32 Quantity = 1);
    UFUNCTION(BlueprintCallable) bool AbortExpedition();
    UFUNCTION(BlueprintCallable) bool MoveStack(EFPSExtractionContainer From, FGuid StackId, EFPSExtractionContainer To, int32 X, int32 Y, bool bRotated);
    UFUNCTION(BlueprintCallable) bool AutoTransferStack(EFPSExtractionContainer From, FGuid StackId, EFPSExtractionContainer To);
    UFUNCTION(BlueprintCallable) bool DropStack(FGuid StackId);
    UFUNCTION(BlueprintCallable) bool SellStack(FGuid StackId);
    bool CanPlace(const TArray<FFPSExtractionStack>& Inventory, FName Id, int32 X, int32 Y, bool bRotated, int32 Capacity) const;
    bool ValidateGrid(const TArray<FFPSExtractionStack>& Inventory, int32 Capacity) const;
    // Mode-only operations check the active standalone mode and phase.
    bool EnterWorld(bool bRaid);
    bool Collect(FName Id, int32 Quantity);
    bool FinishExpedition(bool bSuccess);
    bool RestoreEquipment(class AFPSDemoCharacter* Body);
    void SetInteraction(const FString& Text, float Distance, float Remaining);
    void SetFeedback(const FString& Text) { Feedback = Text; }
    EFPSExtractionPhase GetPhase() const { return Phase; }
    const FFPSExtractionItem* FindItem(FName Id) const;
    const FFPSExtractionRecipe* FindRecipe(FName Id) const;
    int32 SlotsUsed(const TArray<FFPSExtractionStack>& Inventory) const;
    bool TryAdd(TArray<FFPSExtractionStack>& Inventory, FName Id, int32 Quantity, int32 Capacity) const;
    bool TryRemove(TArray<FFPSExtractionStack>& Inventory, FName Id, int32 Quantity) const;
    int32 Count(const TArray<FFPSExtractionStack>& Inventory, FName Id) const;
private:
    friend struct FFPSExtractionSmoke;
    bool IsAuthorized() const;
    bool IsBase() const;
    bool LoadProfile();
    bool Commit(UFPSExtractionSave* Candidate);
    bool ValidateProfile(const UFPSExtractionSave* Candidate) const;
    bool UpgradeProfile(UFPSExtractionSave* Candidate) const;
    bool MutateGrid(EFPSExtractionContainer From, FGuid StackId, EFPSExtractionContainer To, int32 X, int32 Y, bool bRotated, bool bAuto);
    UFPSExtractionSave* CopyProfile() const;
    bool Fail(const FString& Text);
    void Travel(bool bRaid);
    void CreateCatalog();
    UPROPERTY() TObjectPtr<UFPSExtractionSave> Profile;
    UPROPERTY() TArray<FFPSExtractionItem> Items;
    UPROPERTY() TArray<FFPSExtractionRecipe> Recipes;
    UPROPERTY() TArray<FFPSExtractionStack> Backpack;
    UPROPERTY() TArray<FFPSExtractionStack> LastHaul;
    FString SlotPrefix;
    FString Feedback, Interaction;
    EFPSExtractionPhase Phase = EFPSExtractionPhase::Base;
    bool bLoaded = false, bStorageHealthy = true, bLastSuccess = false;
    float ExtractionRemaining = -1, ExtractionDistance = 0;
};
