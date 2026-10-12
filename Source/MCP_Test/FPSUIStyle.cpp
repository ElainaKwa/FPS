#include "FPSUIStyle.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
#include "Brushes/SlateColorBrush.h"
#include "Misc/CommandLine.h"

FString FPSUI::AssetRoot()
{
#if !UE_BUILD_SHIPPING
    FString Variant;
    FParse::Value(FCommandLine::Get(),TEXT("FPSUIFontVariant="),Variant);
    if(Variant==TEXT("MiSans")||Variant==TEXT("HarmonyOS"))
        return TEXT("/Game/FPSDemo/Art/FontComparisons/")+Variant+TEXT("/UI/");
#endif
    return TEXT("/Game/FPSDemo/Art/Industrial/UI/");
}

const FButtonStyle& FPSUI::Button()
{
    static const FButtonStyle Style = FButtonStyle()
        .SetNormal(FSlateColorBrush(FLinearColor(.035f,.04f,.045f))).SetHovered(FSlateColorBrush(FLinearColor(.22f,.24f,.23f)))
        .SetPressed(FSlateColorBrush(FLinearColor(.38f,.33f,.10f)))
        .SetDisabled(FSlateColorBrush(FLinearColor(.08f,.09f,.09f)))
        .SetNormalForeground(Paper).SetHoveredForeground(FLinearColor::White).SetPressedForeground(Yellow);
    return Style;
}

namespace
{
struct FWeaponIcon
{
    TStrongObjectPtr<UTexture2D> Texture;
    FSlateBrush Brush;
};
FWeaponIcon& Icon(FName Id)
{
    static TMap<FName,TUniquePtr<FWeaponIcon>> Cache;
    if (auto* Found=Cache.Find(Id)) return **Found;
    auto Entry=MakeUnique<FWeaponIcon>();
    FString Name=Id.ToString(); Name.RemoveFromStart(TEXT("Weapon."));
    Name.ReplaceInline(TEXT("-"),TEXT(""));
    const FString Path=TEXT("/Game/FPSDemo/Art/Industrial/UI/T_IND_")+Name;
    Entry->Texture.Reset(LoadObject<UTexture2D>(nullptr,*Path));
    Entry->Brush.SetResourceObject(Entry->Texture.Get());
    Entry->Brush.ImageSize=FVector2D(256,128);
    Entry->Brush.DrawAs=ESlateBrushDrawType::Image;
    auto* Result=Entry.Get(); Cache.Add(Id,MoveTemp(Entry)); return *Result;
}
}
UTexture2D* FPSUI::WeaponTexture(FName Id) { return Id.IsNone() ? nullptr : Icon(Id).Texture.Get(); }
UTexture2D* FPSUI::WeaponPreviewTexture(FName Id)
{
    if(Id.IsNone())return nullptr;
    FString Name=Id.ToString();Name.RemoveFromStart(TEXT("Weapon."));Name.ReplaceInline(TEXT("-"),TEXT(""));
    return LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/FPSDemo/Art/Industrial/UI/T_UI_%s.T_UI_%s"),*Name,*Name));
}
const FSlateBrush* FPSUI::WeaponBrush(FName Id) { return &Icon(Id).Brush; }
