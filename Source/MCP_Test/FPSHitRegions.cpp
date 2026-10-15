#include "FPSHitRegions.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"

void UFPSHitRegions::BindPose(const TMap<FString, TObjectPtr<USceneComponent>>& Nodes)
{
    if (!Bodies.IsEmpty()) return;
    auto Add = [&](FName Name, const FString& NodeName, EFPSHitRegion Region, FVector Center, FVector Extent)
    {
        const auto* Node = Nodes.Find(NodeName); if (!Node || !*Node) return;
        auto* Box = NewObject<UBoxComponent>(GetOwner(), FName(*(TEXT("Hit_") + Name.ToString())));
        GetOwner()->AddInstanceComponent(Box);
        Box->SetupAttachment(*Node); Box->SetRelativeLocation(Center); Box->SetBoxExtent(Extent);
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionResponseToAllChannels(ECR_Ignore);
        Box->SetCollisionResponseToChannel(ShotChannel, ECR_Block);
        Box->SetGenerateOverlapEvents(false); Box->SetCanEverAffectNavigation(false);
        Box->SetHiddenInGame(true); Box->RegisterComponent();
        Bodies.Add(Name, Box); Regions.Add(Name, Region);
    };
    // Centimeters in the rigid avatar's local coordinates. Segment X is normalized [0,1].
    Add(TEXT("Head"), TEXT("Helmet"), EFPSHitRegion::Head, FVector(0,0,0), FVector(12,12,15));
    Add(TEXT("Neck"), TEXT("Torso"), EFPSHitRegion::Torso, FVector(0,0,35), FVector(7.7,7.7,4));
    Add(TEXT("Torso"), TEXT("Torso"), EFPSHitRegion::Torso, FVector(0,0,17), FVector(13,20,20));
    Add(TEXT("Pelvis"), TEXT("Pelvis"), EFPSHitRegion::Torso, FVector(0,0,0), FVector(12,18,10));
    for (const TCHAR* Side : {TEXT("L"), TEXT("R")})
    {
        for (const TCHAR* Segment : {TEXT("UpperArm"), TEXT("Forearm"), TEXT("Thigh"), TEXT("Shin")})
        {
            const FString Name = FString(Side) + Segment;
            const float Radius = FString(Segment) == TEXT("Thigh") ? 8.f : (FString(Segment) == TEXT("Shin") ? 7.f : 6.f);
            Add(FName(*Name), Name, EFPSHitRegion::Limb, FVector(.5,0,0), FVector(.5,Radius,Radius));
        }
        const FString Shoulder = FString(Side)+TEXT("Shoulder");
        Add(FName(*Shoulder), FString(Side)+TEXT("UpperArm"), EFPSHitRegion::Limb, FVector(0,0,0), FVector(.15,9,10));
        const FString Glove = FString(Side)+TEXT("Glove"), Boot = FString(Side)+TEXT("Boot");
        Add(FName(*Glove), Glove, EFPSHitRegion::Limb, FVector(1,0,-3), FVector(4,6,8));
        Add(FName(*Boot), Boot, EFPSHitRegion::Limb, FVector(5,0,3), FVector(14,7,8));
    }
}
void UFPSHitRegions::SetEnabled(bool bEnabled)
{
    for (const auto& Pair : Bodies) Pair.Value->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}
EFPSHitRegion UFPSHitRegions::Classify(const FHitResult& Hit) const
{
    if (Hit.GetActor() != GetOwner()) return EFPSHitRegion::None;
    for (const auto& Pair : Bodies) if (Pair.Value == Hit.GetComponent()) return Regions.FindChecked(Pair.Key);
    return EFPSHitRegion::None;
}
UBoxComponent* UFPSHitRegions::FindBody(FName Name) const { return Bodies.FindRef(Name); }
