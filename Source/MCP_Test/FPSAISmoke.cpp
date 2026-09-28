#include "FPSAISmoke.h"
#include "FPSMatchFlow.h"
#include "FPSBotNavigation.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoPlayerState.h"
#include "FPSWeaponComponent.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "TimerManager.h"

static void StartCrowdCheck(AFPSMatchMode* Mode)
{
#if !UE_BUILD_SHIPPING
    TWeakObjectPtr<AFPSMatchMode> WeakMode(Mode);
    FTimerHandle Timer;
    Mode->GetWorldTimerManager().SetTimer(Timer, [WeakMode, Age = 0,
        Initial = TMap<TWeakObjectPtr<AFPSBotController>, FVector>(), Active = TSet<TWeakObjectPtr<AFPSBotController>>()]() mutable
    {
        auto* Rules = WeakMode.Get();
        if (!Rules || Age > 80) return;
        int32 Bots = 0, Kills = 0;
        bool bOnMap = true;
        for (TActorIterator<AFPSBotController> It(Rules->GetWorld()); It; ++It)
        {
            ++Bots;
            const auto* PS = It->GetPlayerState<AFPSDemoPlayerState>();
            Kills += PS ? PS->GetEliminationCount() : 0;
            if (APawn* Pawn = It->GetPawn())
            {
                const TWeakObjectPtr<AFPSBotController> Key(*It);
                const FVector Position = Pawn->GetActorLocation();
                if (!Initial.Contains(Key)) Initial.Add(Key, Position);
                if (FVector::DistSquared2D(Initial[Key], Position) > FMath::Square(400.f) ||
                    (PS && (PS->GetEliminationCount() > 0 || PS->GetDeathCount() > 0))) Active.Add(Key);
                bOnMap &= Position.Z > -50 && FMath::Abs(Position.X) < 2450 && FMath::Abs(Position.Y) < 1650;
                if (Age == 80 && !Active.Contains(Key))
                    UE_LOG(LogTemp, Display, TEXT("FPS_AI_CROWD inactive=%s position=%s"), *It->GetName(), *Position.ToCompactString());
            }
        }
        if (Age % 20 == 0) UE_LOG(LogTemp, Display, TEXT("FPS_AI_CROWD age=%d bots=%d active=%d kills=%d on-map=%d"), Age, Bots, Active.Num(), Kills, bOnMap);
        if (Age == 80)
            UE_LOG(LogTemp, Display, TEXT("FPS_AI_SMOKE %s COMPLETE crowd"), Bots == 9 && Active.Num() == 9 && Kills >= 2 && bOnMap ? TEXT("PASS") : TEXT("FAIL"));
        ++Age;
    }, .5f, true, 2.f);
#endif
}

void FFPSAISmoke::Start(AFPSMatchMode* Mode)
{
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("FPSAIStress"))) { StartCrowdCheck(Mode); return; }
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSAISmoke"))) return;
    TWeakObjectPtr<AFPSMatchMode> WeakMode(Mode);
    FTimerHandle Timer;
    Mode->GetWorldTimerManager().SetTimer(Timer, [WeakMode, Age = 0, Bot = TWeakObjectPtr<AFPSBotController>(),
        bEscaped = false, bFired = false, bFailed = false]() mutable
    {
        auto* Rules = WeakMode.Get();
        if (!Rules || Age > 65) return;
        auto Check = [&bFailed](bool bPass, const TCHAR* Message)
        {
            bFailed |= !bPass;
            UE_LOG(LogTemp, Display, TEXT("FPS_AI_SMOKE %s %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), Message);
        };
        if (Age == 0)
        {
            auto* Nav = Rules->GetWorld()->GetSubsystem<UFPSBotNavigation>();
            auto CheckRoute = [&](FVector Start, FVector End, const TCHAR* Message)
            {
                TArray<FVector> Points;
                bool bValid = Nav->FindRoute(Start, End, Points);
                float Length = 0;
                FVector Previous = Start;
                for (const FVector& P : Points) { bValid &= Nav->ClearSegment(Previous, P); Length += FVector::Dist2D(Previous, P); Previous = P; }
                Check(bValid && Length > FVector::Dist2D(Start, End) + 80, Message);
            };
            CheckRoute(FVector(-2050,0,90), FVector(0,0,90), TEXT("capsule-clear route around spawn shield"));
            CheckRoute(FVector(-1150,-800,90), FVector(-350,-800,90), TEXT("capsule-clear route around cargo"));
            TArray<FVector> MarginExit;
            Check(Nav->FindRoute(FVector(810.05,-992.83,90), FVector(1050,-1250,90), MarginExit) &&
                !MarginExit.IsEmpty() && Nav->ClearSegment(FVector(810.05,-992.83,90), MarginExit[0]),
                TEXT("pawn inside clearance margin can rejoin route"));
            Check(!Nav->ClearSegment(FVector(750,-800,90), FVector(1050,-1250,90)), TEXT("solid cargo overlap cannot use margin recovery"));
            TArray<FVector> Outside;
            Check(!Nav->FindRoute(FVector(4000,0,90), FVector(0,0,90), Outside), TEXT("unreachable outside-wall start rejected"));
            for (TActorIterator<AFPSBotController> It(Rules->GetWorld()); It; ++It) { Bot = *It; break; }
            auto* Host = Rules->GetWorld()->GetFirstPlayerController();
            if (Bot.IsValid() && Bot->GetPawn() && Host && Host->GetPawn())
            {
                Bot->GetPawn()->SetActorLocation(FVector(-2050,0,100), false, nullptr, ETeleportType::TeleportPhysics);
                Host->GetPawn()->SetActorLocation(FVector(800,0,100), false, nullptr, ETeleportType::TeleportPhysics);
                Bot->Enemy.Reset(); Bot->LastSeenTime = -10; Bot->bVisibleEnemy = false;
                Bot->NextRoute = Bot->NextSense = 0; Bot->PatrolIndex = 0;
                Check(true, TEXT("live bot and opposing player positioned behind shield"));
            }
            else Check(false, TEXT("live bot fixture available"));
        }
        if (Bot.IsValid())
        {
            if (auto* Pawn = Cast<AFPSDemoCharacter>(Bot->GetPawn()))
            {
                bEscaped |= Pawn->GetActorLocation().X > -1450;
                const auto* Def = Pawn->GetWeapons()->Definition();
                bFired |= Def && Pawn->GetWeapons()->Equipped().Magazine < Def->MagazineSize;
                if (Age % 20 == 0)
                    UE_LOG(LogTemp, Display, TEXT("FPS_AI_SMOKE SNAPSHOT age=%d location=%s ammo=%d score=%d"), Age,
                        *Pawn->GetActorLocation().ToCompactString(), Pawn->GetWeapons()->Equipped().Magazine,
                        Bot->GetPlayerState<AFPSDemoPlayerState>()->GetEliminationCount());
            }
        }
        if (Age == 60)
        {
            Check(bEscaped, TEXT("live bot escaped shield using route"));
            Check(bFired, TEXT("live bot acquired enemy and fired"));
            Check(Bot.IsValid() && Bot->GetPlayerState<AFPSDemoPlayerState>()->GetEliminationCount() > 0, TEXT("live bot damage produced GAS score"));
            Check(!bFailed, TEXT("COMPLETE"));
        }
        ++Age;
    }, .5f, true, 2.f);
#endif
}
