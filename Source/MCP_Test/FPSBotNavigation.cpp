#include "FPSBotNavigation.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FPSNavigationRegion.h"
#include "FPSBotSettings.h"

FVector UFPSBotNavigation::Cell(int32 Index) const
{
    return FVector(Origin.X + (Index % Width) * 100, Origin.Y + (Index / Width) * 100, 90);
}

const TArray<FVector>& UFPSBotNavigation::GetPatrolPoints()
{
    BuildGrid();
    return AuthoredPatrol.IsEmpty() ? GetDefault<UFPSBotSettings>()->PatrolPoints : AuthoredPatrol;
}

bool UFPSBotNavigation::ClearSegment(const FVector& Start, const FVector& End) const
{
    FHitResult Hit;
    // Capsule clearance includes a margin over the character's 38 cm radius.
    const FVector From(Start.X, Start.Y, 90), To(End.X, End.Y, 90);
    const FCollisionObjectQueryParams StaticObjects(ECC_WorldStatic);
    const FCollisionShape PaddedCapsule = FCollisionShape::MakeCapsule(43, 88);
    if (!GetWorld()->SweepSingleByObjectType(Hit, From, To, FQuat::Identity, StaticObjects, PaddedCapsule)) return true;
    // Character collision can leave a pawn within the extra navigation margin.
    // Permit an exit from that margin only when the real capsule clears the whole
    // segment and the destination restores full clearance. Never escape solid geometry.
    if (!Hit.bStartPenetrating || GetWorld()->OverlapAnyTestByObjectType(To, FQuat::Identity, StaticObjects, PaddedCapsule)) return false;
    const FCollisionShape ActualCapsule = FCollisionShape::MakeCapsule(38, 88);
    if (GetWorld()->OverlapAnyTestByObjectType(From, FQuat::Identity, StaticObjects, ActualCapsule)) return false;
    return !GetWorld()->SweepSingleByObjectType(Hit, From, To, FQuat::Identity, StaticObjects, ActualCapsule);
}

void UFPSBotNavigation::BuildGrid()
{
    if (bBuilt || GetWorld()->GetNetMode() == NM_Client) return;
    int32 Regions = 0;
    for (TActorIterator<AFPSNavigationRegion> It(GetWorld()); It; ++It)
    {
        if (++Regions > 1 || !It->IsValidLayout())
        {
            UE_LOG(LogTemp, Error, TEXT("FPS_NAV invalid or duplicate navigation region"));
            bBuilt = true;
            return;
        }
        Width = It->GridSize.X; Height = It->GridSize.Y; Origin = It->GridOrigin;
        AuthoredPatrol = It->PatrolPoints;
    }
    Walkable.Init(0, Width * Height);
    for (int32 Index = 0; Index < Walkable.Num(); ++Index)
    {
        const FVector P = Cell(Index);
        FHitResult Floor;
        const bool bGround = GetWorld()->LineTraceSingleByObjectType(Floor, P + FVector(0, 0, 400), P - FVector(0, 0, 130),
            FCollisionObjectQueryParams(ECC_WorldStatic)) && FMath::Abs(Floor.ImpactPoint.Z) < 5 && Floor.ImpactNormal.Z > .95f;
        Walkable[Index] = bGround && !GetWorld()->OverlapAnyTestByObjectType(P, FQuat::Identity,
            FCollisionObjectQueryParams(ECC_WorldStatic), FCollisionShape::MakeCapsule(43, 88));
    }
    bBuilt = true;
}

int32 UFPSBotNavigation::NearestCell(const FVector& Point) const
{
    int32 Best = INDEX_NONE;
    double Distance = TNumericLimits<double>::Max();
    for (int32 Index = 0; Index < Walkable.Num(); ++Index)
    {
        const double D = FVector::DistSquared2D(Point, Cell(Index));
        if (Walkable[Index] && D < Distance && ClearSegment(Point, Cell(Index))) { Best = Index; Distance = D; }
    }
    return Best;
}

bool UFPSBotNavigation::FindRoute(const FVector& Start, const FVector& Goal, TArray<FVector>& OutPoints)
{
    OutPoints.Reset();
    BuildGrid();
    if (Walkable.IsEmpty()) return false;
    const int32 Source = NearestCell(Start), Destination = NearestCell(Goal);
    if (Source == INDEX_NONE || Destination == INDEX_NONE) return false;
    TArray<float> Cost;
    TArray<int32> Parent;
    TArray<uint8> Closed;
    Cost.Init(MAX_flt, Walkable.Num()); Parent.Init(INDEX_NONE, Walkable.Num()); Closed.Init(0, Walkable.Num());
    TArray<int32> Open;
    Cost[Source] = 0; Open.Add(Source);
    while (!Open.IsEmpty())
    {
        int32 BestSlot = 0;
        float BestScore = MAX_flt;
        for (int32 Slot = 0; Slot < Open.Num(); ++Slot)
        {
            const int32 Node = Open[Slot];
            const float Score = Cost[Node] + FVector::Dist2D(Cell(Node), Cell(Destination));
            if (Score < BestScore) { BestScore = Score; BestSlot = Slot; }
        }
        const int32 Current = Open[BestSlot];
        Open.RemoveAtSwap(BestSlot);
        if (Current == Destination)
        {
            TArray<FVector> Reverse;
            for (int32 Node = Destination; Node != INDEX_NONE; Node = Parent[Node]) Reverse.Add(Cell(Node));
            // Pull the path tight, but retain capsule clearance around every corner.
            FVector Previous = Start;
            for (int32 Last = Reverse.Num() - 1; Last >= 0;)
            {
                int32 Next = Last;
                for (int32 Candidate = 0; Candidate <= Last; ++Candidate)
                    if (ClearSegment(Previous, Reverse[Candidate])) { Next = Candidate; break; }
                OutPoints.Add(Reverse[Next]); Previous = Reverse[Next]; Last = Next - 1;
            }
            return !OutPoints.IsEmpty();
        }
        Closed[Current] = 1;
        const int32 X = Current % Width, Y = Current / Width;
        for (int32 DY = -1; DY <= 1; ++DY) for (int32 DX = -1; DX <= 1; ++DX)
        {
            if ((!DX && !DY) || X + DX < 0 || X + DX >= Width || Y + DY < 0 || Y + DY >= Height) continue;
            const int32 Next = (Y + DY) * Width + X + DX;
            if (!Walkable[Next] || Closed[Next]) continue;
            if (DX && DY && (!Walkable[Y * Width + X + DX] || !Walkable[(Y + DY) * Width + X])) continue;
            const float NewCost = Cost[Current] + (DX && DY ? 141.42136f : 100.f);
            if (NewCost >= Cost[Next] || !ClearSegment(Cell(Current), Cell(Next))) continue;
            if (Cost[Next] == MAX_flt) Open.Add(Next);
            Cost[Next] = NewCost; Parent[Next] = Current;
        }
    }
    return false;
}
