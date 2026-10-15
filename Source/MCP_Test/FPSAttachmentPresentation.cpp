#include "FPSAttachmentPresentation.h"
#include "FPSAttachmentData.h"
#include "FPSWeaponReloadRig.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"

void RefreshFPSAttachmentMeshes(UStaticMeshComponent* Parent, const UFPSWeaponCatalog* Catalog, const UFPSWeaponDefinition* Definition, const TArray<FName>& Ids,
    EFPSAttachmentView View, TArray<TObjectPtr<UStaticMeshComponent>>& Components)
{
    if (!Parent || !Catalog || !Definition || !Parent->GetOwner()) return;
    if (Definition->ModularBody.Get())
    {
        const bool HasRig = View != EFPSAttachmentView::Pickup && UFPSWeaponReloadRig::Find(
            View == EFPSAttachmentView::FirstPerson ? Definition->FirstPerson.Mesh.Get() : Definition->ThirdPerson.Mesh.Get());
        Parent->SetStaticMesh(HasRig && Definition->ModularReloadBody.Get() ? Definition->ModularReloadBody.Get() : Definition->ModularBody.Get());
    }
    Components.SetNum(FPSAttachments::CategoryCount);
    const UFPSAttachmentDefinition* Parts[FPSAttachments::CategoryCount] = {};
    for (FName Id : Ids)
        if (const auto* Part = Catalog->FindAttachment(Id); Part && FPSAttachments::IsCategory(Part->Category) && !Part->bUseWeaponPresentation) Parts[uint8(Part->Category)] = Part;
    for (int32 Index = 0; Index < FPSAttachments::CategoryCount; ++Index)
    {
        const auto* Part = Parts[Index];
        const auto* Presentation = Part ? (View == EFPSAttachmentView::FirstPerson ? &Part->FirstPerson : View == EFPSAttachmentView::ThirdPerson ? &Part->ThirdPerson : &Part->Pickup) : Definition->BuiltinParts.Find(FPSAttachments::Mount(EFPSAttachmentCategory(Index)));
        if (!Presentation && !Components[Index]) continue;
        if (!Components[Index])
        {
            auto* Piece = NewObject<UStaticMeshComponent>(Parent->GetOwner());
            Piece->SetupAttachment(Parent); Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Piece->SetOnlyOwnerSee(View == EFPSAttachmentView::FirstPerson); Piece->SetOwnerNoSee(View == EFPSAttachmentView::ThirdPerson);
            Piece->SetCastShadow(View != EFPSAttachmentView::FirstPerson);
            Parent->GetOwner()->AddInstanceComponent(Piece); Piece->RegisterComponent(); Components[Index] = Piece;
        }
        Components[Index]->SetStaticMesh(Presentation ? Presentation->Mesh.Get() : nullptr);
        Components[Index]->AttachToComponent(Parent, FAttachmentTransformRules::KeepRelativeTransform);
        if (Presentation) Components[Index]->SetRelativeTransform(Presentation->Transform);
        Components[Index]->SetVisibility(Presentation != nullptr && Parent->IsVisible());
    }
}
