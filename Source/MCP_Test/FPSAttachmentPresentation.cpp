#include "FPSAttachmentPresentation.h"
#include "FPSAttachmentData.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"

void RefreshFPSAttachmentMeshes(UStaticMeshComponent* Parent, const UFPSWeaponCatalog* Catalog, const TArray<FName>& Ids,
    EFPSAttachmentView View, TArray<TObjectPtr<UStaticMeshComponent>>& Components)
{
    if (!Parent || !Catalog || !Parent->GetOwner()) return;
    Components.SetNum(3);
    const UFPSAttachmentDefinition* Parts[3] = {};
    for (FName Id : Ids)
        if (const auto* Part = Catalog->FindAttachment(Id); Part && uint8(Part->Category) < 3 && !Part->bUseWeaponPresentation) Parts[uint8(Part->Category)] = Part;
    for (int32 Index = 0; Index < 3; ++Index)
    {
        const auto* Part = Parts[Index];
        const auto* Presentation = Part ? (View == EFPSAttachmentView::FirstPerson ? &Part->FirstPerson : View == EFPSAttachmentView::ThirdPerson ? &Part->ThirdPerson : &Part->Pickup) : nullptr;
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
        if (Presentation) Components[Index]->SetRelativeTransform(Presentation->Transform);
        Components[Index]->SetVisibility(Presentation != nullptr && Parent->IsVisible());
    }
}
