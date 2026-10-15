#include "FPSWeaponReloadRig.h"
#include "FPSProceduralAnimation.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

const UFPSWeaponReloadRig* UFPSWeaponReloadRig::Find(UStaticMesh* Mesh)
{
    const auto* Rig = Mesh ? Cast<UFPSWeaponReloadRig>(Mesh->GetAssetUserDataOfClass(StaticClass())) : nullptr;
    return Rig && Rig->Body && Rig->Magazine && Rig->CyclingPart ? Rig : nullptr;
}

bool UFPSWeaponReloadRigLibrary::InstallReloadRig(UStaticMesh* Original, UStaticMesh* Body,
    UStaticMesh* Magazine, UStaticMesh* CyclingPart, FVector MagazineGrip, FVector CyclingGrip, float CyclingTravel)
{
    if (!Original || !Body || !Magazine || !CyclingPart || Original == Body || Original == Magazine ||
        Original == CyclingPart || MagazineGrip.ContainsNaN() || CyclingGrip.ContainsNaN() ||
        !FMath::IsFinite(CyclingTravel) || CyclingTravel <= 0) return false;
    Original->Modify();
    Original->RemoveUserDataOfClass(UFPSWeaponReloadRig::StaticClass());
    auto* Rig = NewObject<UFPSWeaponReloadRig>(Original);
    Rig->Body = Body; Rig->Magazine = Magazine; Rig->CyclingPart = CyclingPart;
    Rig->MagazineGrip = MagazineGrip; Rig->CyclingGrip = CyclingGrip; Rig->CyclingTravel = CyclingTravel;
    Original->AddAssetUserData(Rig); Original->MarkPackageDirty();
    return true;
}

void RefreshFPSReloadParts(UStaticMeshComponent* Parent, UStaticMesh* Original, bool FirstPerson,
    TArray<TObjectPtr<UStaticMeshComponent>>& Parts)
{
    const auto* Rig = UFPSWeaponReloadRig::Find(Original);
    Parent->SetStaticMesh(Rig ? Rig->Body.Get() : Original);
    Parts.SetNum(2);
    for (int32 I = 0; I < 2; ++I)
    {
        if (!Parts[I] && Rig)
        {
            auto* Part = NewObject<UStaticMeshComponent>(Parent->GetOwner());
            Parent->GetOwner()->AddInstanceComponent(Part); Part->SetupAttachment(Parent);
            Part->SetCollisionEnabled(ECollisionEnabled::NoCollision); Part->SetGenerateOverlapEvents(false);
            Part->SetCanEverAffectNavigation(false); Part->SetOnlyOwnerSee(FirstPerson);
            Part->SetOwnerNoSee(!FirstPerson); Part->SetCastShadow(!FirstPerson);
            Part->RegisterComponent(); Parts[I] = Part;
        }
        if (!Parts[I]) continue;
        Parts[I]->SetStaticMesh(Rig ? (I == 0 ? Rig->Magazine.Get() : Rig->CyclingPart.Get()) : nullptr);
        Parts[I]->SetRelativeTransform(FTransform::Identity);
        Parts[I]->SetVisibility(Rig != nullptr && Parent->IsVisible());
    }
}

void AnimateFPSReloadParts(const UFPSWeaponReloadRig* Rig, const FPSProceduralAnimation::FActionPose& Pose,
    bool Alive, TArray<TObjectPtr<UStaticMeshComponent>>& Parts,
    const TArray<TObjectPtr<UStaticMeshComponent>>& Attachments)
{
    // A separately authored magazine attachment replaces the built-in magazine and follows the same track.
    auto* Override = Attachments.IsValidIndex(2) ? Attachments[2].Get() : nullptr;
    const bool HasOverride = Override && Override->GetStaticMesh();
    if (HasOverride && Rig) Override->SetVisibility(Alive && Pose.MagazineVisible);
    for (int32 I = 0; I < Parts.Num(); ++I)
    {
        auto* Part = Parts[I].Get();
        if (!Part) continue;
        Part->SetVisibility(Alive && Rig && (I != 0 || (Pose.MagazineVisible && !HasOverride)));
        Part->SetRelativeLocation(I == 0 ? Pose.MagazineOffset : FVector(-Pose.Cycle * (Rig ? Rig->CyclingTravel : 0), 0, 0));
    }
}
