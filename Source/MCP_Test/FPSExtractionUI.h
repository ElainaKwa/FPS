#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FPSExtractionTypes.h"
#include "FPSAttachmentData.h"
#include "FPSExtractionUI.generated.h"

class UFPSExtractionSubsystem;
class UFPSExtractionUI;
class UFPSUITheme;
class UFPSExtractionGridDrag;

/** Saved button metadata; the subsystem remains the authority for every action. */
UCLASS()
class MCP_TEST_API UFPSExtractionButton : public UButton
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Extraction") FName Action;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Extraction") FName ItemId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Extraction") int32 Quantity = 1;
    void Bind(UFPSExtractionUI* Target);
private:
    UFUNCTION() void Activate();
    UFUNCTION() void HoverCandidate();
    UFUNCTION() void LeaveCandidate();
    TWeakObjectPtr<UFPSExtractionUI> OwnerUI;
};

/** Independent extraction UI; no changes to the existing match UI/controller. */
UCLASS()
class MCP_TEST_API UFPSExtractionUI : public UUserWidget
{
    GENERATED_BODY()
public:
    static UFPSExtractionUI* Open(APlayerController* Owner, bool bOverlay=false);
    UFUNCTION(BlueprintCallable) void SetPanelOpen(bool bOpen);
    UFUNCTION(BlueprintPure) bool IsPanelOpen() const;
    UFUNCTION(BlueprintCallable) void Refresh();
    void Dispatch(FName Action, FName ItemId=NAME_None, int32 Quantity=1);
    void SelectInventoryStack(EFPSExtractionContainer Container,FGuid StackId);
    bool IsInventorySelected(EFPSExtractionContainer Container,FGuid StackId) const;
    bool CanInventoryMove(EFPSExtractionContainer From,EFPSExtractionContainer To) const;
    bool IsInventoryDragging() const { return ActiveDrag!=nullptr; }
    void BeginInventoryDrag(UFPSExtractionGridDrag* Drag);
    void EndInventoryDrag();
    void CommitInventoryDrag(UFPSExtractionGridDrag* Drag,EFPSExtractionContainer To,int32 X,int32 Y);
    void InventoryFeedback(const FString& Text);
    void SelectAttachmentCategory(EFPSAttachmentCategory Category);
    EFPSAttachmentCategory GetSelectedAttachmentCategory() const { return SelectedCategory; }
    bool IsWeaponEditing() const { return bWeaponEditing; }
    bool IsAttachmentDrawerOpen() const { return bAttachmentDrawer; }
    EFPSWeaponSlot GetViewedWeaponSlot() const { return bViewSecondary?EFPSWeaponSlot::Secondary:EFPSWeaponSlot::Primary; }
    void WorkbenchBack();
    void PreviewAttachmentStats(FName Action,FName Id);
    void ClearAttachmentStats(FName Action,FName Id);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Extraction") TObjectPtr<UFPSUITheme> Theme;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Extraction") TObjectPtr<class UFPSExtractionLobbyPresentation> LobbyPresentation;
protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    UFPSExtractionSubsystem* Model() const;
    void RebuildRows(const FFPSExtractionView& View);
    void RebuildAttachmentRows(const FFPSExtractionView& View);
    void RefreshAttachmentStats(const FFPSExtractionView& View);
    void RefreshWeaponOverview(const FFPSExtractionView& View);
    bool CanModifyViewedWeapon(const FFPSExtractionView& View) const;
    void SetText(FName Name, const FString& Value);
    void SetShown(FName Name, bool bShow);
    void SetEnabled(FName Name, bool bEnabled);
    FString RowSignature(const FFPSExtractionView& View) const;
    FString LastRows;
    FString LocalFeedback,LastBackendFeedback;
    FGuid SelectedStack;
    EFPSExtractionContainer SelectedContainer = EFPSExtractionContainer::Warehouse;
    UPROPERTY(Transient) TObjectPtr<UFPSExtractionGridDrag> ActiveDrag;
    int32 Tab = 0;
    float RefreshClock = 0;
    bool bBagOpen = false;
    bool bConfirmAbort = false;
    EFPSExtractionPhase Phase = EFPSExtractionPhase::Base;
    EFPSAttachmentCategory SelectedCategory = EFPSAttachmentCategory::Optic;
    FName ComparedAction,ComparedId;
    bool bWeaponEditing=false,bAttachmentDrawer=false,bViewSecondary=false;
    UPROPERTY(Transient) TObjectPtr<class UFPSExtractionLobbyScene> LobbyScene;
};

UCLASS()
class MCP_TEST_API UFPSExtractionUIBuilder : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** Creates only missing assets; never overwrites a Designer-edited tree. */
    UFUNCTION(BlueprintCallable, Category="Extraction|Editor") static FString BuildAssets();
    /** Explicit v2 migration, rebuilds only this suite's terminal; backup in the upgrade script. */
    UFUNCTION(BlueprintCallable, Category="Extraction|Editor") static FString UpgradeInventoryAssets();
    UFUNCTION(BlueprintCallable, Category="Extraction|Editor") static FString UpgradeWeaponWorkbenchAssets();
    UFUNCTION(BlueprintCallable, Category="Extraction|Editor") static FString UpgradeWeaponWorkbenchReferenceAssets(bool Rebuild=false);
    UFUNCTION(BlueprintCallable, Category="Extraction|Editor") static FString UpgradeLobbyAssets(bool Rebuild=false);
};
