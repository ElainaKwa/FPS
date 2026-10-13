#pragma once

#include "CoreMinimal.h"
#include "FPSWeaponData.h"
#include "FPSExtractionTypes.generated.h"

UENUM(BlueprintType)
enum class EFPSExtractionPhase : uint8 { Base, Raid, Result };

UENUM(BlueprintType)
enum class EFPSExtractionContainer : uint8 { Warehouse, Backpack };

USTRUCT(BlueprintType)
struct FFPSExtractionStack
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ItemId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Quantity = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FGuid StackId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 X = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Y = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bRotated = false;
};

USTRUCT(BlueprintType)
struct FFPSExtractionItem
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ItemId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Name;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Description;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 StackLimit = 20;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SellPrice = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName AttachmentId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Width = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Height = 1;
};

USTRUCT(BlueprintType)
struct FFPSExtractionRecipe
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName RecipeId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Name;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FFPSExtractionStack> Inputs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FFPSExtractionStack Output;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CreditCost = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Duration = 5;
};

/** UI consumes this snapshot; all mutations go through the subsystem. */
USTRUCT(BlueprintType)
struct FFPSExtractionView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) EFPSExtractionPhase Phase = EFPSExtractionPhase::Base;
    UPROPERTY(BlueprintReadOnly) TArray<FFPSExtractionStack> Warehouse;
    UPROPERTY(BlueprintReadOnly) TArray<FFPSExtractionStack> Backpack;
    UPROPERTY(BlueprintReadOnly) TArray<FFPSExtractionStack> LastHaul;
    UPROPERTY(BlueprintReadOnly) FFPSWeaponLoadout Loadout;
    UPROPERTY(BlueprintReadOnly) int32 Credits = 0;
    UPROPERTY(BlueprintReadOnly) int32 BackpackSlots = 20;
    UPROPERTY(BlueprintReadOnly) int32 BackpackWidth = 5;
    UPROPERTY(BlueprintReadOnly) int32 BackpackHeight = 4;
    UPROPERTY(BlueprintReadOnly) int32 WarehouseWidth = 8;
    UPROPERTY(BlueprintReadOnly) int32 WarehouseHeight = 8;
    UPROPERTY(BlueprintReadOnly) int32 BackpackUsed = 0;
    UPROPERTY(BlueprintReadOnly) int32 WarehouseSlots = 64;
    UPROPERTY(BlueprintReadOnly) int32 WarehouseUsed = 0;
    UPROPERTY(BlueprintReadOnly) FName ProductionRecipe;
    UPROPERTY(BlueprintReadOnly) float ProductionRemaining = 0;
    UPROPERTY(BlueprintReadOnly) bool bProductionReady = false;
    UPROPERTY(BlueprintReadOnly) bool bLastSuccess = false;
    UPROPERTY(BlueprintReadOnly) bool bStorageHealthy = true;
    UPROPERTY(BlueprintReadOnly) FString Feedback;
    UPROPERTY(BlueprintReadOnly) FString Interaction;
    UPROPERTY(BlueprintReadOnly) float ExtractionRemaining = -1;
    UPROPERTY(BlueprintReadOnly) float ExtractionDistance = 0;
};
