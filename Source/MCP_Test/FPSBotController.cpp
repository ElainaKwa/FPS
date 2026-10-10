#include "FPSMatchFlow.h"
#include "FPSBotSettings.h"
#include "FPSBotNavigation.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponPickup.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"

AFPSBotController::AFPSBotController()
{
    bWantsPlayerState = true;
    PrimaryActorTick.bCanEverTick = true;
}

void AFPSBotController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("FPSFlowSmoke")) || FParse::Param(FCommandLine::Get(), TEXT("FPSMapSmoke"))) return;
#endif
    auto* Rules = GetWorld()->GetAuthGameMode<AFPSMatchMode>();
    auto* Self = Cast<AFPSDemoCharacter>(GetPawn());
    const auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
    if (!HasAuthority() || !Rules || !GS || GS->bTraining || !Rules->IsCombatActive() || !Self || Self->IsEliminated()) return;
    const auto& Settings = *GetDefault<UFPSBotSettings>();
    const float Now = GetWorld()->GetTimeSeconds();
    auto* Navigation = GetWorld()->GetSubsystem<UFPSBotNavigation>();
    auto* Weapon = Self->GetWeapons();
    if (PreviousPawn.Get() != Self)
    {
        PreviousPawn = Self; Enemy.Reset(); Route.Reset(); RouteIndex = 0;
        NextRoute = NextSense = NextShot = 0; BurstShots = 0;
        ProgressLocation = Self->GetActorLocation(); ProgressTime = Now;
        PatrolIndex = GetPlayerState<AFPSDemoPlayerState>() ? GetPlayerState<AFPSDemoPlayerState>()->GetPlayerId() % Settings.PatrolPoints.Num() : 0;
    }
    if (Now >= NextSense)
    {
        NextSense = Now + .25f;
        AFPSDemoCharacter* Visible = nullptr;
        float Best = FMath::Square(Settings.SenseRange);
        for (TActorIterator<AFPSDemoCharacter> It(GetWorld()); It; ++It)
        {
            if (It->IsEliminated() || !Rules->CanDamage(Self, *It)) continue;
            float Distance = FVector::DistSquared2D(Self->GetActorLocation(), It->GetActorLocation());
            if (*It == Enemy.Get()) Distance *= .7f; // retain targets instead of switching every scan
            if (Distance >= Best || !LineOfSightTo(*It)) continue;
            Best = Distance; Visible = *It;
        }
        bVisibleEnemy = Visible != nullptr;
        if (!bVisibleEnemy) ClearFocus(EAIFocusPriority::Gameplay);
        if (Visible)
        {
            if (Enemy.Get() != Visible) { ReactionUntil = Now + .45f; BurstShots = 0; }
            Enemy = Visible; LastSeenTime = Now; LastSeenLocation = Visible->GetActorLocation();
        }
        else if (!Enemy.IsValid() || Enemy->IsEliminated() || Now - LastSeenTime > 3.f) Enemy.Reset();
    }
    // Pursue only a visible opponent or its last observed position. Otherwise patrol.
    const auto& Patrol = Settings.PatrolPoints;
    if (Enemy.IsValid()) Destination = LastSeenLocation;
    else
    {
        if (FVector::DistSquared2D(Self->GetActorLocation(), Patrol[PatrolIndex % Patrol.Num()]) < FMath::Square(Settings.ArrivalDistance))
        { ++PatrolIndex; NextRoute = 0; }
        Destination = Patrol[PatrolIndex % Patrol.Num()];
    }
    bool bNeedAmmo = false;
    if (Weapon->Equipped().Magazine <= 0)
    {
        if (Weapon->Equipped().Reserve > 0) Weapon->Reload();
        else if (Weapon->GetLoadout().ActiveSlot == EFPSWeaponSlot::Primary &&
            Weapon->GetLoadout().Secondary.Magazine + Weapon->GetLoadout().Secondary.Reserve > 0) Weapon->Secondary();
        else bNeedAmmo = true;
    }
    if (bNeedAmmo)
    {
        AFPSWeaponPickup* Ammo = nullptr;
        float Nearest = MAX_flt;
        for (TActorIterator<AFPSWeaponPickup> It(GetWorld()); It; ++It)
        {
            const float D = FVector::DistSquared2D(Self->GetActorLocation(), It->GetActorLocation());
            if (It->IsAvailable() && It->GetInstance().Magazine + It->GetInstance().Reserve > 0 && D < Nearest)
            { Nearest = D; Ammo = *It; }
        }
        if (Ammo)
        {
            // Stop beside the display plinth rather than asking for a path inside it.
            const FVector Toward = (Self->GetActorLocation() - Ammo->GetActorLocation()).GetSafeNormal2D();
            Destination = Ammo->GetActorLocation() + Toward * Settings.PickupStandOff;
            if (Nearest < FMath::Square(Settings.ArrivalDistance)) Weapon->Request(EFPSWeaponRequest::Pickup, Ammo);
        }
    }
    const bool bEngage = Enemy.IsValid() && bVisibleEnemy && !bNeedAmmo &&
        FVector::DistSquared2D(Self->GetActorLocation(), LastSeenLocation) < FMath::Square(Settings.EngageDistance);
    if (!bEngage)
    {
        if (Now >= NextRoute)
        {
            NextRoute = Now + .9f;
            Navigation->FindRoute(Self->GetActorLocation(), Destination, Route);
            RouteIndex = 0;
        }
        while (RouteIndex < Route.Num() && FVector::DistSquared2D(Self->GetActorLocation(), Route[RouteIndex]) < FMath::Square(65.f))
        {
            if (RouteIndex + 1 < Route.Num() && !Navigation->ClearSegment(Self->GetActorLocation(), Route[RouteIndex + 1])) break;
            ++RouteIndex;
        }
        if (RouteIndex < Route.Num())
        {
            FVector Direction = (Route[RouteIndex] - Self->GetActorLocation()).GetSafeNormal2D();
            // A small local separation steers around teammates without changing the static grid.
            for (TActorIterator<AFPSDemoCharacter> It(GetWorld()); It; ++It)
                if (*It != Self && !It->IsEliminated())
                {
                    const FVector Away = Self->GetActorLocation() - It->GetActorLocation();
                    const float Distance = Away.Size2D();
                    if (Distance > 1 && Distance < 120) Direction += Away.GetSafeNormal2D() * (1.f - Distance / 120.f);
                }
            Direction = Direction.GetSafeNormal2D();
            if (!Navigation->ClearSegment(Self->GetActorLocation(), Self->GetActorLocation() + Direction * 65))
                Direction = (Route[RouteIndex] - Self->GetActorLocation()).GetSafeNormal2D();
            Self->AddMovementInput(Direction, 1.f);
            if (!bVisibleEnemy)
            {
                ClearFocus(EAIFocusPriority::Gameplay);
                SetControlRotation(Direction.Rotation());
                Self->FaceRotation(GetControlRotation(), DeltaSeconds);
            }
        }
        if (Now - ProgressTime > 2.f)
        {
            if (FVector::DistSquared2D(Self->GetActorLocation(), ProgressLocation) < FMath::Square(80.f))
            { NextRoute = 0; if (!Enemy.IsValid()) ++PatrolIndex; }
            ProgressTime = Now; ProgressLocation = Self->GetActorLocation();
        }
    }
    if (Enemy.IsValid() && bVisibleEnemy)
    {
        SetFocus(Enemy.Get());
        const FRotator Aim = (Enemy->GetPawnViewLocation() - Self->GetPawnViewLocation()).Rotation();
        SetControlRotation(FMath::RInterpTo(GetControlRotation(), Aim, DeltaSeconds, 9.f));
        Self->FaceRotation(GetControlRotation(), DeltaSeconds);
        if (bEngage && Now >= NextShot && Now >= ReactionUntil && Weapon->Equipped().Magazine > 0)
        {
            FHitResult Hit;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(FPSBotAim), true, Self);
            GetWorld()->LineTraceSingleByChannel(Hit, Self->GetPawnViewLocation(), Enemy->GetPawnViewLocation(), ECC_Visibility, Query);
            if (Hit.GetActor() == Enemy.Get() && FMath::Abs(FRotator::NormalizeAxis(GetControlRotation().Yaw - Aim.Yaw)) < 5.f)
            {
                // Brief bursts with modest spread. Damage still goes through the normal server shot path.
                SetControlRotation(Aim + FRotator(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.5f, 1.5f), 0));
                Self->FireBotShot();
                NextShot = Now + (++BurstShots >= 3 ? .85f : .24f);
                if (BurstShots >= 3) BurstShots = 0;
            }
            else NextShot = Now + .1f;
        }
    }
}

