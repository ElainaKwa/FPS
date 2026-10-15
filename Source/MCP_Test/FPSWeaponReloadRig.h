#pragma once
#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FPSWeaponReloadRig.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
namespace FPSProceduralAnimation { struct FActionPose; }

/** Optional assembly, in the original weapon mesh's local centimetres. */
UCLASS(BlueprintType)
class MCP_TEST_API UFPSWeaponReloadRig : public UAssetUserData
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reload") TObjectPtr<UStaticMesh> Body;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reload") TObjectPtr<UStaticMesh> Magazine;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reload") TObjectPtr<UStaticMesh> CyclingPart;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reload") FVector MagazineGrip;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reload") FVector CyclingGrip;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reload") float CyclingTravel = 4.5f;
    static const UFPSWeaponReloadRig* Find(UStaticMesh* Mesh);
};

UCLASS()
class MCP_TEST_API UFPSWeaponReloadRigLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="FPS|Art")
    static bool InstallReloadRig(UStaticMesh* Original, UStaticMesh* Body, UStaticMesh* Magazine,
        UStaticMesh* CyclingPart, FVector MagazineGrip, FVector CyclingGrip, float CyclingTravel);
};

void RefreshFPSReloadParts(UStaticMeshComponent* Parent, UStaticMesh* Original, bool FirstPerson,
    TArray<TObjectPtr<UStaticMeshComponent>>& Parts);
void AnimateFPSReloadParts(const UFPSWeaponReloadRig* Rig, const FPSProceduralAnimation::FActionPose& Pose,
    bool Alive, TArray<TObjectPtr<UStaticMeshComponent>>& Parts,
    const TArray<TObjectPtr<UStaticMeshComponent>>& Attachments);
