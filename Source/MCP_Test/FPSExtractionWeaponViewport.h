#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"
#include "FPSAttachmentData.h"
#include "Styling/SlateBrush.h"
#include "FPSExtractionWeaponViewport.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UStaticMeshComponent;

/** Isolated presentation actor, never edits the weapon catalog or player equipment. */
UCLASS(Transient, NotPlaceable)
class MCP_TEST_API AFPSExtractionPreviewStage : public AActor
{
    GENERATED_BODY()
public:
    AFPSExtractionPreviewStage();
    bool Present(const FFPSWeaponInstance& Instance);
    void Frame(float Yaw,float Pitch,float Zoom,FIntPoint Size);
    FVector2D ProjectMount(EFPSAttachmentCategory Category) const;
    UTextureRenderTarget2D* GetImage() const { return Target; }
    bool HasModel() const;
    FName VisualFor(EFPSAttachmentCategory Category) const;
private:
    UPROPERTY() TObjectPtr<USceneComponent> ModelRoot;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Core;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
    UPROPERTY() TArray<TObjectPtr<UStaticMesh>> PreviewMeshes;
    UPROPERTY() TObjectPtr<USceneCaptureComponent2D> Capture;
    UPROPERTY() TObjectPtr<UTextureRenderTarget2D> Target;
    FString Configuration;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFPSPreviewSlotSelected, EFPSAttachmentCategory);

/** Real captured mesh, projected mount lines, selectable slots and orbit controls. */
UCLASS()
class MCP_TEST_API UFPSExtractionWeaponViewport : public UUserWidget
{
    GENERATED_BODY()
public:
    void Present(const FFPSWeaponInstance& Instance);
    void SetPreviewActive(bool Active);
    void SetSelectedCategory(EFPSAttachmentCategory Category) { Selected=Category; }
    bool HasModel() const { return Stage && Stage->HasModel(); }
    FName VisualFor(EFPSAttachmentCategory Category) const { return Stage?Stage->VisualFor(Category):NAME_None; }
    FOnFPSPreviewSlotSelected OnSlotSelected;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E) override;
    virtual FReply NativeOnMouseMove(const FGeometry& G,const FPointerEvent& E) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& G,const FPointerEvent& E) override;
protected:
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& G,float DeltaTime) override;
    virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& C,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool Enabled) const override;
private:
    void UpdateFrame();
    UPROPERTY(Transient) TObjectPtr<AFPSExtractionPreviewStage> Stage;
    UPROPERTY(Transient) TObjectPtr<class UFont> LabelFont;
    FSlateBrush ImageBrush;
    FFPSWeaponInstance PendingInstance;
    EFPSAttachmentCategory Selected=EFPSAttachmentCategory::Optic;
    bool bActive=false,bOrbiting=false,bFrameDirty=true;
    int32 WarmupFrames=0;
    float Yaw=0,Pitch=0,Zoom=1;
    FIntPoint FrameSize=FIntPoint::ZeroValue;
};
