#include "FPSBaseRegion.h"
#include "FPSDemoGameState.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "FPSDemoCharacter.h"
#include "FPSBotNavigation.h"
#include "Components/CapsuleComponent.h"

AFPSBaseRegion::AFPSBaseRegion()
{
    Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
    SetRootComponent(Bounds);
    Bounds->SetBoxExtent(FVector(310, 1500, 150));
    Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bounds->SetCanEverAffectNavigation(false);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RegionLabel"));
    if (Label)
    {
        Label->SetupAttachment(Bounds);
        Label->SetWorldSize(24);
        Label->SetHorizontalAlignment(EHTA_Center);
    }
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
    for (int32 Index = 0; Index < 4; ++Index)
    {
        auto* Marker = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Boundary%d"), Index));
        if (Marker)
        {
            Marker->SetupAttachment(Bounds);
            Marker->SetStaticMesh(Cube.Object);
            Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Marker->SetCanEverAffectNavigation(false);
        }
        BoundaryMarkers.Add(Marker);
    }
}
void AFPSBaseRegion::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    const FVector Extent = Bounds->GetUnscaledBoxExtent();
    if (Label)
    {
        Label->SetText(FText::FromString(FString::Printf(TEXT("%s BASE / SAFE / NO FIRE"), TeamId == 0 ? TEXT("BLUE") : TEXT("RED"))));
        Label->SetTextRenderColor(TeamId == 0 ? FColor(75,180,255) : FColor(255,80,60));
        Label->SetRelativeLocation(FVector(0, 0, Extent.Z + 90));
        Label->SetRelativeRotation(FRotator(0, TeamId == 0 ? 0 : 180, 0));
    }
    for (int32 Index = 0; Index < FMath::Min(4, BoundaryMarkers.Num()); ++Index)
    {
        auto* Marker = BoundaryMarkers[Index].Get();
        if (!Marker) continue;
        const bool bX = Index < 2;
        Marker->SetRelativeLocation(FVector(bX ? (Index == 0 ? -Extent.X : Extent.X) : 0,
            bX ? 0 : (Index == 2 ? -Extent.Y : Extent.Y), -Extent.Z + 2));
        Marker->SetRelativeScale3D(FVector(bX ? .06 : Extent.X * .02, bX ? Extent.Y * .02 : .06, .02));
    }
}
void AFPSBaseRegion::BeginPlay()
{
    Super::BeginPlay();
    OnConstruction(GetActorTransform());
    if (const auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>(); GS && !GS->bBaseRegionsEnabled)
        SetActorHiddenInGame(true);
}
bool AFPSBaseRegion::Contains(const FVector& Location, float Tolerance) const
{
    const FVector Local = Bounds->GetComponentTransform().InverseTransformPosition(Location);
    const FVector Extent = Bounds->GetUnscaledBoxExtent() + FVector(Tolerance);
    return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}
bool AFPSBaseRegion::ValidateWorld(UWorld* World, TArray<FString>& Errors)
{
    TSet<int32> Teams;
    TSet<APlayerStart*> Seen;
    for (TActorIterator<AFPSBaseRegion> It(World); It; ++It)
    {
        if (It->TeamId < 0 || It->TeamId > 1 || Teams.Contains(It->TeamId)) Errors.Add(TEXT("Duplicate or invalid base team"));
        Teams.Add(It->TeamId);
        if (It->Starts.IsEmpty() || !IsValid(It->FallbackStart) || !It->Contains(It->FallbackStart->GetActorLocation()))
            Errors.Add(TEXT("Base requires contained starts and fallback: ") + It->GetName());
        for (APlayerStart* Start : It->Starts)
        {
            const FString Prefix = It->TeamId == 0 ? TEXT("FPSBlue") : TEXT("FPSRed");
            bool bTeamMatches = false;
            if (Start) for (const FName Tag : Start->Tags) bTeamMatches |= Tag.ToString().StartsWith(Prefix);
            if (!IsValid(Start) || !bTeamMatches || !It->Contains(Start->GetActorLocation()) || Seen.Contains(Start))
                Errors.Add(TEXT("Invalid base start ownership: ") + It->GetName());
            Seen.Add(Start);
        }
    }
    if (Teams.Num() != 2) Errors.Add(TEXT("Team match requires both base regions"));
    for (TActorIterator<APlayerStart> It(World); It; ++It)
        if (!Seen.Contains(*It)) Errors.Add(TEXT("Unassociated team start: ") + It->GetName());
    return Errors.IsEmpty();
}

bool AFPSBaseRegion::SegmentEntry(const FVector& Start, const FVector& End, const FVector& Padding, float& Entry) const
{
    const auto Transform = Bounds->GetComponentTransform();
    const FVector A = Transform.InverseTransformPosition(Start), B = Transform.InverseTransformPosition(End);
    const FVector Scale = Transform.GetScale3D().GetAbs();
    const FVector Extent = Bounds->GetUnscaledBoxExtent() + Padding / Scale;
    float Near = 0, Far = 1;
    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        const double D = B[Axis] - A[Axis];
        if (FMath::Abs(D) < UE_SMALL_NUMBER) { if (FMath::Abs(A[Axis]) > Extent[Axis]) return false; continue; }
        float Low = (-Extent[Axis] - A[Axis]) / D, High = (Extent[Axis] - A[Axis]) / D;
        if (Low > High) Swap(Low, High);
        Near = FMath::Max(Near, Low); Far = FMath::Min(Far, High);
        if (Near > Far) return false;
    }
    Entry = Near; return true;
}
FVector AFPSBaseRegion::OutsidePosition(const FVector& Location, float Padding) const
{
    auto Local = Bounds->GetComponentTransform().InverseTransformPosition(Location);
    const auto Extent = Bounds->GetUnscaledBoxExtent() + FVector(Padding);
    const int32 Axis = Extent.X - FMath::Abs(Local.X) < Extent.Y - FMath::Abs(Local.Y) ? 0 : 1;
    Local[Axis] = (Local[Axis] < 0 ? -1 : 1) * Extent[Axis];
    return Bounds->GetComponentTransform().TransformPosition(Local);
}
bool AFPSBaseRegion::FindSpawn(AController* Controller, FTransform& Result) const
{
    if (!HasAuthority() || !Controller) return false;
    const auto* Body = GetDefault<AFPSDemoCharacter>()->GetCapsuleComponent();
    const float Radius = Body->GetScaledCapsuleRadius(), Height = Body->GetScaledCapsuleHalfHeight();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(FPSBaseSpawn));
    for (APlayerStart* Start : Starts) Query.AddIgnoredActor(Start);
    auto Validate = [&](FVector Candidate, bool bFallback)
    {
        FHitResult Floor;
        if (!GetWorld()->LineTraceSingleByObjectType(Floor, Candidate + FVector(0,0,400), Candidate - FVector(0,0,400), FCollisionObjectQueryParams(ECC_WorldStatic), Query) || Floor.ImpactNormal.Z < .7f) return false;
        Candidate = Floor.ImpactPoint + FVector(0,0,Height + 2);
        if (!Contains(Candidate) || GetWorld()->OverlapBlockingTestByChannel(Candidate, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Radius, Height), Query)) return false;
        for (TActorIterator<APawn> It(GetWorld()); It; ++It)
            if (!It->IsActorBeingDestroyed() && It->GetController() && FMath::Abs(It->GetActorLocation().Z - Candidate.Z) < Height * 2 && FVector::DistSquared2D(It->GetActorLocation(), Candidate) < FMath::Square(Radius * 2 + 16)) return false;
        if (!Controller->IsPlayerController())
        {
            auto* Nav = GetWorld()->GetSubsystem<UFPSBotNavigation>();
            TArray<FVector> Route;
            if (!Nav || !FallbackStart || !Nav->FindRoute(Candidate, FallbackStart->GetActorLocation(), Route)) return false;
        }
        Result = FTransform(FRotator(0, TeamId == 0 ? 0 : 180, 0), Candidate);
        return true;
    };
    const auto Extent = Bounds->GetUnscaledBoxExtent();
    for (int32 Attempt = 0; Attempt < FMath::Clamp(SpawnAttempts,0,128); ++Attempt)
    {
        const FVector Local(FMath::FRandRange(-Extent.X + Radius + 8, Extent.X - Radius - 8), FMath::FRandRange(-Extent.Y + Radius + 8, Extent.Y - Radius - 8), 0);
        if (Validate(Bounds->GetComponentTransform().TransformPosition(Local), false)) return true;
    }
    return IsValid(FallbackStart) && Validate(FallbackStart->GetActorLocation(), true);
}
