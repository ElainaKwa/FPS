#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"

class UTexture2D;
namespace FPSUI
{
    FString AssetRoot();
    const FLinearColor Background(.025f,.031f,.036f,.97f);
    const FLinearColor Panel(.075f,.086f,.092f,.97f);
    const FLinearColor Paper(.83f,.85f,.81f,1);
    const FLinearColor Ink(.025f,.031f,.036f,1);
    const FLinearColor Yellow(.89f,.72f,.16f,1);
    const FLinearColor Muted(.57f,.63f,.63f,1);
    const FButtonStyle& Button();
    UTexture2D* WeaponTexture(FName Id);
    UTexture2D* WeaponPreviewTexture(FName Id);
    const FSlateBrush* WeaponBrush(FName Id);
}
