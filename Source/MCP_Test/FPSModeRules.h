#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Info.h"
#include "FPSWeaponData.h"
#include "FPSModeRules.generated.h"

UENUM(BlueprintType)
enum class EFPSGameplayKind : uint8 { TeamMatch, Training };
UENUM(BlueprintType)
enum class EFPSSelectionPolicy : uint8 { UntilFirstSpawnRegionExit, Always };
UENUM(BlueprintType)
enum class EFPSReplacementPolicy : uint8 { Recycle, Drop };
UENUM(BlueprintType)
enum class EFPSDeathDropPolicy : uint8 { None, All };

/** Definition preferences only. Every spawn receives fresh GUID/ammunition from definitions. */
UCLASS(BlueprintType)
class MCP_TEST_API UFPSSpawnLoadout : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UFPSWeaponDefinition> Primary;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UFPSWeaponDefinition> Secondary;
};

/** Gameplay identity is explicit; standalone/listen/dedicated are independent session choices. */
UCLASS(BlueprintType)
class MCP_TEST_API UFPSModeRules : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EFPSGameplayKind Kind = EFPSGameplayKind::TeamMatch;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UFPSSpawnLoadout> DefaultLoadout;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TSoftObjectPtr<UFPSWeaponDefinition>> SelectableWeapons;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EFPSSelectionPolicy Selection = EFPSSelectionPolicy::UntilFirstSpawnRegionExit;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EFPSReplacementPolicy Replacement = EFPSReplacementPolicy::Recycle;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EFPSDeathDropPolicy DeathDrop = EFPSDeathDropPolicy::None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float DynamicDropLifetime = 180.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float SelectionRequestInterval = .1f;
    // -1 means undecided, NOT unlimited. Stage three must reject activation until a policy is agreed.
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 DynamicDropLimit = -1;
    virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("FPSModeRules"), GetFName()); }
    void GatherWeaponIds(TArray<FName>& Ids, TArray<FString>& Errors) const;
    static UFPSModeRules* ForWorld(UWorld* World, TArray<FString>* Errors = nullptr);
};

/** Saved level reference shared by server and clients before content admission. */
UCLASS()
class MCP_TEST_API AFPSModeSetup : public AInfo
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UFPSModeRules> Rules;
};
