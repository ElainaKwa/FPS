#include "FPSWeaponShot.h"
#include "FPSWeaponComponent.h"
#include "FPSDifficultySmoke.h"
#include "FPSMatchFlow.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoAttributeSet.h"
#include "FPSDemoPlayerState.h"
#include "FPSDemoGameState.h"
#include "FPSWeaponPickup.h"
#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"

void FFPSDifficultySmoke::Start(AFPSMatchMode* Mode)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSDifficultySmoke"))) return;
    for (TActorIterator<AFPSBotController> It(Mode->GetWorld()); It; ++It) It->SetActorTickEnabled(false);
    FTimerHandle Timer;
    TWeakObjectPtr<AFPSMatchMode> WeakMode(Mode);
    Mode->GetWorldTimerManager().SetTimer(Timer, [WeakMode, Stage = 0, Failed = false]() mutable
    {
        auto* Rules = WeakMode.Get();
        if (!Rules || Stage > 1) return;
        auto* Player = Cast<AFPSDemoCharacter>(Rules->GetWorld()->GetFirstPlayerController()->GetPawn());
        TArray<AFPSDemoCharacter*> Bots;
        for (TActorIterator<AFPSBotController> It(Rules->GetWorld()); It; ++It)
            if (auto* Bot = Cast<AFPSDemoCharacter>(It->GetPawn())) Bots.Add(Bot);
        auto Check = [&](bool Pass, const TCHAR* What)
        {
            Failed |= !Pass;
            UE_LOG(LogTemp, Display, TEXT("FPS_DIFFICULTY_SMOKE %s %s"), Pass ? TEXT("PASS") : TEXT("FAIL"), What);
        };
        if (!Player || Bots.Num() < 2) { Check(false, TEXT("fixture available")); Stage = 2; return; }
        auto ResetHealth = [](AFPSDemoCharacter* P) { P->GetAbilitySystemComponent()->SetNumericAttributeBase(UFPSDemoAttributeSet::GetHealthAttribute(), 100); };
        auto TestDamage = [&](AFPSDemoCharacter* A, AFPSDemoCharacter* B, float Damage, const TCHAR* What)
        {
            ResetHealth(B); A->ApplyDamageEffectTo(B, FFPSWeaponShotContext::Capture(A, A->GetWeapons()->Equipped(), *A->GetWeapons()->Definition())); Check(FMath::IsNearlyEqual(B->GetHealth(), 100 - Damage, .001f), What);
        };
        if (Stage == 0)
        {
            float Expected = -1; FParse::Value(FCommandLine::Get(), TEXT("ExpectedMultiplier="), Expected);
            Player->GetPlayerState<AFPSDemoPlayerState>()->TeamId = 0;
            Bots[0]->GetPlayerState<AFPSDemoPlayerState>()->TeamId = 1;
            Bots[1]->GetPlayerState<AFPSDemoPlayerState>()->TeamId = 0;
            TestDamage(Bots[0], Player, 25 * Expected, TEXT("bot to player GAS multiplier"));
            TestDamage(Bots[0], Bots[1], 25, TEXT("bot to bot normal damage"));
            TestDamage(Player, Bots[0], 25, TEXT("player to bot normal damage"));
            if (!Rules->GetGameState<AFPSDemoGameState>()->bTraining) TestDamage(Player, Bots[1], 0, TEXT("friendly fire still rejected"));
            auto* W = Bots[0]->GetWeapons();
            Bots[0]->GetController()->SetControlRotation(FRotator(80,0,0));
            const int32 Before = W->Equipped().Magazine;
            Bots[0]->FireBotShot(); Bots[0]->FireBotShot();
            Check(W->Equipped().Magazine == Before - 1, TEXT("AI uses shared ability cooldown"));
            W->Reload(); Bots[0]->FireBotShot();
            Check(W->GetLoadout().Action == EFPSWeaponAction::Reloading && W->Equipped().Magazine == Before - 1, TEXT("AI cannot shoot through magazine reload"));
            // Move a bot-owned instance through the same pickup Ability used by players.
            W->Drop();
            AFPSWeaponPickup* Dropped = nullptr;
            for (TActorIterator<AFPSWeaponPickup> It(Rules->GetWorld()); It; ++It)
                if (It->GetInstance().Magazine == Before - 1) { Dropped = *It; break; }
            Check(Dropped != nullptr, TEXT("bot drop created pickup"));
            if (Dropped)
            {
                Player->SetActorLocation(FVector(400,0,100));
                Dropped->SetActorLocation(Player->GetActorLocation() + FVector(40,0,0));
                const auto Id = Dropped->GetInstance().InstanceId;
                Player->GetWeapons()->Request(EFPSWeaponRequest::Pickup, Dropped);
                Check(Player->GetWeapons()->Equipped().InstanceId == Id, TEXT("player acquires bot instance through GAS"));
            }
        }
        else
        {
            TestDamage(Player, Bots[0], 25, TEXT("picked-up bot weapon has no difficulty residue"));
            Rules->GetGameState<AFPSDemoGameState>()->bFinished = true;
            TestDamage(Bots[0], Player, 0, TEXT("post-match damage rejected"));
            Check(!Failed, TEXT("COMPLETE"));
        }
        ++Stage;
    }, 1.f, true, 2.f);
#endif
}
