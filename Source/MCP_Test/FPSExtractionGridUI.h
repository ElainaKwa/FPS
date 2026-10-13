#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "FPSExtractionTypes.h"
#include "FPSExtractionGridUI.generated.h"

class UFPSExtractionUI;

UCLASS()
class MCP_TEST_API UFPSExtractionGridDrag : public UDragDropOperation
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<UFPSExtractionUI> OwnerUI;
    UPROPERTY() FGuid StackId;
    UPROPERTY() FName ItemId;
    UPROPERTY() int32 Quantity = 0;
    UPROPERTY() EFPSExtractionContainer From = EFPSExtractionContainer::Warehouse;
    UPROPERTY() int32 GrabX = 0;
    UPROPERTY() int32 GrabY = 0;
    UPROPERTY() int32 Width = 1;
    UPROPERTY() int32 Height = 1;
    UPROPERTY() bool bRotated = false;
    void Rotate();
    virtual void DragCancelled_Implementation(const FPointerEvent& Event) override;
};

/** Saved, editable grid style; item geometry is drawn from real stack coordinates.
 * There are no per-refresh child widgets to steal focus or cancel a drag. */
UCLASS()
class MCP_TEST_API UFPSExtractionGridUI : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory") EFPSExtractionContainer Container = EFPSExtractionContainer::Warehouse;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory") float CellSize = 58.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory") TObjectPtr<class UFont> TextFont;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory") TObjectPtr<class UFont> NumberFont;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory") FLinearColor GridColor = FLinearColor(.14f,.18f,.19f,1);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory") FLinearColor ItemColor = FLinearColor(.075f,.11f,.12f,1);
    void Present(UFPSExtractionUI* Owner,const FFPSExtractionView& View,const TArray<FFPSExtractionItem>& Items);
public:
    // Public native event overrides also permit development tests to drive the same UI path.
    virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled) const override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual void NativeOnDragDetected(const FGeometry& Geometry,const FPointerEvent& Event,UDragDropOperation*& Operation) override;
    virtual bool NativeOnDragOver(const FGeometry& Geometry,const FDragDropEvent& Event,UDragDropOperation* Operation) override;
    virtual void NativeOnDragLeave(const FDragDropEvent& Event,UDragDropOperation* Operation) override;
    virtual bool NativeOnDrop(const FGeometry& Geometry,const FDragDropEvent& Event,UDragDropOperation* Operation) override;
private:
    const FFPSExtractionItem* Definition(FName Id) const;
    const FFPSExtractionStack* Hit(int32 X,int32 Y) const;
    bool PreviewFits(const UFPSExtractionGridDrag& Drag,int32 X,int32 Y,FString& Reason) const;
    FIntPoint CellAt(const FGeometry& Geometry,FVector2D Absolute) const;
    TWeakObjectPtr<UFPSExtractionUI> OwnerUI;
    TArray<FFPSExtractionStack> Stacks;
    TArray<FFPSExtractionItem> Definitions;
    int32 Columns = 8,Rows = 8;
    FGuid PressedStack;
    FIntPoint PressedCell = FIntPoint::ZeroValue;
    bool bPreview = false,bFits = false;
    int32 PreviewX=0,PreviewY=0,PreviewW=1,PreviewH=1;
};
