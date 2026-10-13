#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "InputCoreTypes.h"
#include "FPSSystemSettings.generated.h"

enum class EFPSSettingKind : uint8 { Slider, Choice, Binding, Action };
struct FFPSSettingOption
{
    FName Id;
    FString Label,Description;
    EFPSSettingKind Kind=EFPSSettingKind::Choice;
    double Value=0,Min=0,Max=1,Step=1;
    TArray<FString> Choices;
    FString ValueText;
};
struct FFPSKeyBinding
{
    FName Id; FString Label;
    FKey Primary,Secondary;
};
struct FFPSGraphicsSettings
{
    int32 Mode=1; FIntPoint Resolution=FIntPoint(1920,1080);
    int32 FrameLimit=120; bool VSync=false,MotionBlur=false;
    float FOV=92;
    TArray<int32> Quality={2,2,2,2,2,2,2,2};
    bool operator==(const FFPSGraphicsSettings& Other) const;
};

/** Local settings and display transaction; never changes authority gameplay tuning. */
UCLASS()
class MCP_TEST_API UFPSSystemSettings : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return !IsTemplate() && bInitialized; }
    static UFPSSystemSettings* Get(const UObject* Context);
    TArray<FFPSSettingOption> Options(int32 Page) const;
    bool SetValue(FName Id,double Value);
    bool ApplyGraphics();
    void DiscardGraphics();
    bool HasPendingGraphics() const { return !(Draft==Graphics); }
    bool IsConfirmingDisplay() const { return DisplayDeadline>0; }
    int32 DisplaySecondsLeft() const;
    void ConfirmDisplay();
    void RevertDisplay();
    void ResetPage(int32 Page);
    void BeginCapture(FName OptionId);
    bool IsCapturing() const { return !CaptureId.IsNone(); }
    bool HasBindingConflict() const { return !Conflicts.IsEmpty(); }
    FString CapturePrompt() const;
    void CaptureKey(FKey Key);
    void ClearCapturedBinding();
    void ConfirmBindingReplacement();
    void CancelCapture();
    const TArray<FFPSKeyBinding>& Bindings() const { return KeyBindings; }
    const FFPSKeyBinding* Binding(FName Id) const;
    FString BindingText(FName Id) const;
    uint32 BindingRevision() const { return InputRevision; }
    FString Feedback;
    bool LastSaveSucceeded() const { return bSaved; }
    float Sensitivity=1,ADSSensitivity=1,HUDScale=1,BobScale=1,MasterVolume=1,GameVolume=1;
    bool InvertY=false,AimToggle=false,BackgroundMute=true;
    float BaseFOV() const { return Graphics.FOV; }
    bool MotionBlurEnabled() const { return Graphics.MotionBlur; }
    void PreviewSound();
    bool Save();
    FString StoragePath() const;
private:
    friend struct FFPSSettingsSmoke;
    void Load();
    void ApplyRuntimeGraphics(const FFPSGraphicsSettings& Value,bool Display);
    void ApplyImmediate();
    void InstallBinding(bool Replace);
    TArray<FIntPoint> Resolutions;
    TArray<FFPSKeyBinding> KeyBindings;
    FFPSGraphicsSettings Graphics,Draft,PreviousDisplay,Defaults;
    FName CaptureId;
    FKey PendingKey;
    TArray<FString> Conflicts;
    double DisplayDeadline=0;
    uint32 InputRevision=0;
    bool bInitialized=false,bSaved=true,bRuntimeApplied=false,bForceSaveFailure=false;
};
