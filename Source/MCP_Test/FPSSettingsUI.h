#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FPSSystemSettings.h"
#include "FPSSettingsUI.generated.h"

class UFPSSettingsUI;
class UFPSSettingsRow;
class UFPSUITheme;
class UFont;

UCLASS()
class MCP_TEST_API UFPSSettingsButton : public UButton
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings") FName Action;
    void Bind(UFPSSettingsUI* Owner,UFPSSettingsRow* Row=nullptr);
private:
    UFUNCTION() void Activate();
    TWeakObjectPtr<UFPSSettingsUI> OwnerUI;
    TWeakObjectPtr<UFPSSettingsRow> OwnerRow;
};

/** One saved reusable row; values refresh in place, never recreate an active slider. */
UCLASS()
class MCP_TEST_API UFPSSettingsRow : public UUserWidget
{
    GENERATED_BODY()
public:
    void Present(UFPSSettingsUI* Owner,const FFPSSettingOption& Option);
    void Activate();
    FName GetOptionId() const { return OptionId; }
    bool IsInteracting() const;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Settings") TObjectPtr<UFont> TextFont;
protected:
    virtual void NativeConstruct() override;
private:
    UFUNCTION() void SliderChanged(float Value);
    UFUNCTION() void SliderBegin();
    UFUNCTION() void SliderEnd();
    UFUNCTION() void ChoiceChanged(FString Choice,ESelectInfo::Type Selection);
    UFUNCTION() UWidget* MakeChoice(FString Choice);
    TWeakObjectPtr<UFPSSettingsUI> OwnerUI;
    FName OptionId;
    EFPSSettingKind Kind=EFPSSettingKind::Choice;
    bool bUpdating=false,bSliding=false;
    double Minimum=0,Maximum=1,Step=1;
    TArray<FString> Choices;
};

/** Shared settings surface; the host owns input blocking and restores its source page. */
UCLASS()
class MCP_TEST_API UFPSSettingsUI : public UUserWidget
{
    GENERATED_BODY()
public:
    static UFPSSettingsUI* Open(APlayerController* Owner);
    void RequestBack();
    FSimpleDelegate OnClosed;
    void Dispatch(FName Action);
    void Refresh();
    void SetOptionValue(FName Id,double Value);
    void ActivateOption(FName Id,EFPSSettingKind Kind);
    int32 GetPage() const { return Page; }
    UFPSSettingsRow* FindOptionRow(FName Id) const;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Settings") TObjectPtr<UFPSUITheme> Theme;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E) override;
    virtual FReply NativeOnKeyUp(const FGeometry& G,const FKeyEvent& E) override;
    virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& G,const FPointerEvent& E) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& G,const FPointerEvent& E) override;
protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& G,float DeltaTime) override;
private:
    enum class EDialog : uint8 { None,Exit,Reset,Display,Capture,Conflict };
    EDialog Dialog() const;
    void Close();
    void Apply(bool ReturnAfter);
    void RefreshRows();
    void RefreshDialog();
    void Capture(FKey Key,bool Modified);
    bool IsDialogButtonAt(const FVector2D& Position) const;
    UPROPERTY(Transient) TArray<TObjectPtr<UFPSSettingsRow>> Rows;
    int32 Page=0,RowsPage=-1;
    float Clock=0;
    bool bExitPrompt=false,bResetPrompt=false,bReturnAfterDisplay=false,bWasConfirming=false,bClosing=false;
    FString LocalFeedback;
    FKey ConsumedCaptureKey;
    FKey PendingModifier;
    bool bModifierChord=false;
};

UCLASS()
class MCP_TEST_API UFPSSettingsUIBuilder : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** Creates missing settings assets only; Designer edits and all existing packages are preserved. */
    UFUNCTION(BlueprintCallable, Category="Settings|Editor") static FString BuildAssets();
};
