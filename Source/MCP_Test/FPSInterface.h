#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FPSStaminaPresentation.h"
#include "FPSInterface.generated.h"

/** Shared presentation data for saved HUD widgets and the lightweight Canvas fallback. */
struct FFPSStaminaHUDState
{
    float Value = 0.f, Ratio = 0.f;
    bool Visible = false, Alert = false, Exhausted = false;
    FString Status;
};

/** Editable common palette. Layouts and typography live in the six Widget Blueprints. */
UCLASS(BlueprintType)
class MCP_TEST_API UFPSUITheme : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Background = FLinearColor(.008f,.012f,.014f,.98f);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Panel = FLinearColor(.017f,.025f,.029f,.96f);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Paper = FLinearColor(.76f,.78f,.74f,1);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Ink = FLinearColor(.009f,.014f,.017f,1);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Accent = FLinearColor(.94f,.68f,.045f,1);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Muted = FLinearColor(.36f,.42f,.43f,1);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Blue = FLinearColor(.045f,.39f,.8f,1);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Red = FLinearColor(.82f,.13f,.10f,1);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<class UTexture2D> PanelTexture;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FButtonStyle DarkButton;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FButtonStyle LightButton;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FButtonStyle AccentButton;
};

/** Buttons keep their action identifier in the saved Designer tree. */
UCLASS()
class MCP_TEST_API UFPSActionButton : public UButton
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPS") FName Action;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPS") FName WeaponId;
    void Connect();
private:
    UFUNCTION() void Activate();
    UFUNCTION() void Preview();
};

/** Behaviour only: every visual child is authored and saved in a Widget Blueprint. */
UCLASS()
class MCP_TEST_API UFPSInterface : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FPS") FName Screen;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FPS") TObjectPtr<UFPSUITheme> Theme;
    static UFPSInterface* Open(class APlayerController* Owner, const TCHAR* Page);
    void Dispatch(FName Action, FName WeaponId = NAME_None);
    void FocusWeapon(FName Id) { ViewedWeapon=Id; }
    bool HasControl(FName Name) const;
    static FFPSStaminaHUDState ReadStaminaHUD(const class AFPSDemoCharacter* Body);
protected:
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
    void Text(FName Name, const FString& Value);
    void Visible(FName Name, bool Show);
    void Tint(FName Name, FLinearColor Color);
    void RefreshLoadout();
    void RefreshHUD(float DeltaTime);
    FFPSStaminaPresentation StaminaPresentation;
    TWeakObjectPtr<const class AFPSDemoCharacter> StaminaOwner;
    void RefreshMenu();
    FString Input(FName Name) const;
    FName ViewedWeapon;
    int32 Tab = 0;
    bool bHelp = false;
    bool bInputSettings = false;
};

UCLASS()
class MCP_TEST_API UFPSUIAssetBuilder : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="FPS|Editor") static FString BuildAssets();
    UFUNCTION(BlueprintCallable, Category="FPS|Editor") static FString ExtendMovementAssets();
};
