#include "FPSNavigationRegion.h"
#include "FPSBotNavigation.h"
#include "FPSMatchFlow.h"
#include "FPSBaseRegion.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "FPSWeaponPickup.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

AFPSNavigationRegion::AFPSNavigationRegion()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
}

bool AFPSNavigationRegion::IsValidLayout() const
{
    if (GridOrigin.ContainsNaN() || GridSize.X < 2 || GridSize.Y < 2 || GridSize.X > 192 || GridSize.Y > 192 || PatrolPoints.IsEmpty()) return false;
    for (const FVector& P : PatrolPoints)
        if (P.ContainsNaN() || FMath::Abs(P.Z - 90) > 1 || P.X < GridOrigin.X || P.Y < GridOrigin.Y ||
            P.X > GridOrigin.X + (GridSize.X - 1) * 100 || P.Y > GridOrigin.Y + (GridSize.Y - 1) * 100) return false;
    return true;
}

void AFPSNavigationRegion::BeginPlay()
{
    Super::BeginPlay();
#if !UE_BUILD_SHIPPING
    SetActorTickEnabled(HasAuthority() && (FParse::Param(FCommandLine::Get(), TEXT("FPSLargeMapSmoke")) ||
        FParse::Param(FCommandLine::Get(), TEXT("FPSLargeMapCapture"))));
#endif
}

void AFPSNavigationRegion::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    const float Now = GetWorld()->GetTimeSeconds();
    const bool bSmoke = FParse::Param(FCommandLine::Get(), TEXT("FPSLargeMapSmoke"));
    auto Check = [&](bool OK, const TCHAR* Label)
    {
        bFailed |= !OK;
        UE_LOG(LogTemp, Display, TEXT("FPS_LARGE_MAP %s %s"), OK ? TEXT("PASS") : TEXT("FAIL"), Label);
    };
    if (bSmoke && !bChecked && Now > 4)
    {
        bChecked = true;
        auto* Nav = GetWorld()->GetSubsystem<UFPSBotNavigation>();
        auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
        Check(IsValidLayout() && GS && GS->WeaponDataError.IsEmpty() && !GS->bWaiting, TEXT("layout and match admitted"));
        TArray<FString> Errors;
        Check(AFPSBaseRegion::ValidateWorld(GetWorld(), Errors), TEXT("both bases and spawn ownership"));
        int32 Starts = 0, Pickups = 0, Pawns = 0;
        FCollisionQueryParams Query;
        for (TActorIterator<AFPSDemoCharacter> It(GetWorld()); It; ++It) { ++Pawns; Query.AddIgnoredActor(*It); }
        for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It) Query.AddIgnoredActor(*It);
        bool Clear = true;
        TArray<FVector> Nodes = PatrolPoints;
        for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
        {
            ++Starts;
            const FVector P = It->GetActorLocation();
            Clear &= !GetWorld()->OverlapBlockingTestByChannel(P, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(38,88), Query);
            FHitResult Floor;
            Clear &= GetWorld()->LineTraceSingleByObjectType(Floor, P, P - FVector(0,0,200), FCollisionObjectQueryParams(ECC_WorldStatic));
            Nodes.Add(P);
        }
        for (TActorIterator<AFPSWeaponPickup> It(GetWorld()); It; ++It) ++Pickups;
        Check(Clear && Starts == 10 && Pawns == 10 && Pickups == 14, TEXT("ten clear starts ten characters fourteen weapons"));
        bool Routes = Nav != nullptr;
        for (int32 I = 0; Nav && I < Nodes.Num(); ++I)
        {
            TArray<FVector> Route;
            bool Valid = Nav->FindRoute(Nodes[I], Nodes[(I+1)%Nodes.Num()], Route);
            FVector Previous = Nodes[I];
            for (const FVector& Point : Route) { Valid &= Nav->ClearSegment(Previous, Point); Previous = Point; }
            Valid &= !Route.IsEmpty() && Nav->ClearSegment(Previous, Nodes[(I+1)%Nodes.Num()]);
            if (!Valid) UE_LOG(LogTemp, Display, TEXT("FPS_LARGE_MAP route-failed index=%d"), I);
            Routes &= Valid;
        }
        Check(Routes, TEXT("all authored patrol nodes and spawns connected with capsule clearance"));
        TArray<FVector> Outside;
        Check(Nav && !Nav->FindRoute(FVector(-6500,0,90), Nodes[0], Outside), TEXT("outside perimeter rejected"));
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("FPSLargeMapCapture")) && CaptureIndex < 10 && Now > 8 + CaptureIndex * 3)
    {
        const FName Tag(*FString::Printf(TEXT("CrossingCapture%d"), CaptureIndex / 2));
        for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
            if (It->ActorHasTag(Tag))
            {
                if (auto* PC = GetWorld()->GetFirstPlayerController())
                {
                    if (CaptureIndex % 2 == 0) PC->SetViewTarget(*It);
                    else FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("IndustrialCrossing") / Tag.ToString() + TEXT(".png"), false, false);
                }
                break;
            }
        ++CaptureIndex;
    }
    int32 Bots = 0, Kills = 0;
    bool OnMap = true;
    for (TActorIterator<AFPSBotController> It(GetWorld()); bSmoke && It; ++It)
    {
        ++Bots;
        if (auto* PS = It->GetPlayerState<AFPSDemoPlayerState>()) Kills += PS->GetEliminationCount();
        if (const APawn* Pawn = It->GetPawn())
        {
            const FVector P = Pawn->GetActorLocation();
            const TWeakObjectPtr<AController> Key(*It);
            if (!InitialPositions.Contains(Key)) InitialPositions.Add(Key,P);
            if (FVector::DistSquared2D(InitialPositions[Key],P) > FMath::Square(800.f)) ActiveBots.Add(Key);
            OnMap &= P.Z > -50 && P.Z < 500 && P.X > GridOrigin.X-100 && P.Y > GridOrigin.Y-100 &&
                P.X < GridOrigin.X+GridSize.X*100 && P.Y < GridOrigin.Y+GridSize.Y*100;
        }
    }
    if (bSmoke && bChecked && !OnMap) bFailed = true;
    if (bSmoke && Now > 85)
    {
        UE_LOG(LogTemp, Display, TEXT("FPS_LARGE_MAP combat bots=%d active=%d kills=%d on-map=%d"), Bots, ActiveBots.Num(), Kills, OnMap);
        Check(Bots == 9 && ActiveBots.Num() == 9 && Kills > 0 && OnMap, TEXT("live nine-bot movement combat and GAS score"));
        Check(!bFailed, TEXT("COMPLETE"));
        FPlatformMisc::RequestExit(false);
        SetActorTickEnabled(false);
    }
#endif
}
