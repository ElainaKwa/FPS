#include "FPSFlowSmoke.h"
#include "AbilitySystemComponent.h"
#include "FPSDemoEffects.h"
#include "FPSWeaponShot.h"
#include "FPSWeaponComponent.h"
#include "FPSMatchFlow.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "FPSDemoPlayerController.h"
#include "Misc/CommandLine.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"

void FFPSFlowSmoke::Start(AFPSMatchMode* Mode)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSFlowSmoke"))) return;
    TWeakObjectPtr<AFPSMatchMode> WeakMode(Mode);
    FTimerHandle Handle;
    // Run against the actual loaded map, PlayerControllers, GAS and net driver.
    Mode->GetWorldTimerManager().SetTimer(Handle, [WeakMode, Step = 0, Shooter = TWeakObjectPtr<AFPSDemoCharacter>(), Victim = TWeakObjectPtr<AFPSDemoCharacter>(), VictimController = TWeakObjectPtr<AController>()]() mutable
    {
        auto* Rules = WeakMode.Get();
        if (!Rules) return;
        auto* GS = Rules->GetGameState<AFPSDemoGameState>();
        if (!GS) return;
        auto Check = [](bool bPass, const TCHAR* Name)
        {
            UE_LOG(LogTemp, Display, TEXT("FPS_SMOKE %s %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), Name);
        };
        auto* Host = Rules->GetWorld()->GetFirstPlayerController();
        if (Step == 0)
        {
            if (!Host || (GS->bWaiting && GS->PlayerArray.Num() < 2)) return;
            Check(GS->bStructuredMatch, TEXT("structured game state"));
            if (GS->bWaiting)
            {
                APlayerController* Guest = nullptr;
                for (auto It = Rules->GetWorld()->GetPlayerControllerIterator(); It; ++It)
                    if (It->Get() != Host) { Guest = It->Get(); break; }
                if (Guest)
                {
                    Rules->StartRound(Guest);
                    Check(GS->bWaiting, TEXT("non-host start rejected"));
                    TArray<AFPSDemoPlayerState*> Fillers;
                    for (int32 Team = 0; Team < 2; ++Team)
                        while (Rules->TeamSize(Team) < 5)
                        {
                            auto* Filler = Rules->GetWorld()->SpawnActor<AFPSDemoPlayerState>();
                            Filler->TeamId = Team;
                            Fillers.Add(Filler);
                        }
                    auto* GuestPS = Guest->GetPlayerState<AFPSDemoPlayerState>();
                    const int32 PreviousTeam = GuestPS->TeamId;
                    Rules->SwitchTeam(Guest);
                    Check(GuestPS->TeamId == PreviousTeam, TEXT("five-player team capacity enforced"));
                    Check(Rules->InitNewPlayer(Host, FUniqueNetIdRepl(), TEXT(""), TEXT("")) == TEXT("Room full (10/10)."), TEXT("full room admission rejected"));
                    for (auto* Filler : Fillers) Filler->Destroy();
                }
                Rules->StartRound(Host);
            }
            Check(!GS->bWaiting, TEXT("round started"));
            Check(GS->bTraining || FMath::Abs(GS->RoundEndsAt - GS->GetServerWorldTimeSeconds() - 300) < 10, TEXT("five minute deadline"));
            Shooter = Cast<AFPSDemoCharacter>(Host->GetPawn());
            for (auto It = Rules->GetWorld()->GetControllerIterator(); It; ++It)
                if (It->Get() != Host)
                    if (auto* Pawn = Cast<AFPSDemoCharacter>(It->Get()->GetPawn()))
                    {
                        if (Rules->CanDamage(Shooter.Get(), Pawn)) { Victim = Pawn; VictimController = It->Get(); break; }
                    }
            if (!Shooter.IsValid() || !Victim.IsValid()) { Check(false, TEXT("opposing pawns present")); Step = 100; return; }
            // Keep unrelated range dummies out of this test's firing corridor.
            // Actual traces must still hit the first blocking actor during normal play.
            int32 Bystander = 0;
            for (auto It = Rules->GetWorld()->GetControllerIterator(); It; ++It)
                if (auto* Pawn = Cast<AFPSDemoCharacter>(It->Get()->GetPawn()); Pawn && Pawn != Shooter.Get() && Pawn != Victim.Get())
                {
                    Pawn->SetActorLocation(FVector(-1400 + Bystander++ * 180, -700, 100), false, nullptr, ETeleportType::TeleportPhysics);
                    Pawn->GetCharacterMovement()->DisableMovement();
                }
            Shooter->SetActorLocation(FVector(-400, 0, 100), false, nullptr, ETeleportType::TeleportPhysics);
            Victim->SetActorLocation(FVector(400, 0, 100), false, nullptr, ETeleportType::TeleportPhysics);
            Shooter->GetCharacterMovement()->DisableMovement();
            Victim->GetCharacterMovement()->DisableMovement();
            // Verify friendly fire before restoring enemy assignment.
            auto* A = Shooter->GetPlayerState<AFPSDemoPlayerState>();
            auto* B = Victim->GetPlayerState<AFPSDemoPlayerState>();
            const int32 OldTeam = B->TeamId;
            B->TeamId = A->TeamId;
            const float Before = Victim->GetHealth();
            Shooter->ApplyDamageEffectTo(Victim.Get(), FFPSWeaponShotContext::Capture(Shooter.Get(), Shooter->GetWeapons()->Equipped(), *Shooter->GetWeapons()->Definition()));
            Check(Victim->GetHealth() == Before, TEXT("friendly damage rejected"));
            B->TeamId = OldTeam;
            Check(Rules->CanDamage(Shooter.Get(), Victim.Get()), TEXT("enemy damage allowed"));
        }
        else if (Step >= 1 && Step <= 4 && Shooter.IsValid() && Victim.IsValid())
        {
            Shooter->GetController()->SetControlRotation((Victim->GetPawnViewLocation() - Shooter->GetPawnViewLocation()).Rotation());
            Shooter->FireBotShot();
            if (Step == 4)
            {
                Check(Victim->IsEliminated(), TEXT("four authoritative shots eliminate"));
				Check(FMath::Abs(Victim->GetRespawnSeconds() - AFPSDemoCharacter::RespawnDelay) < .1f &&
					Victim->GetKillerName() == Shooter->GetPlayerState<AFPSDemoPlayerState>()->GetPlayerName().Left(32), TEXT("server death countdown and killer"));
				if (auto* PC = Cast<AFPSDemoPlayerController>(Host))
				{
					const float KillUntil = PC->GetKillMarkerUntil();
					PC->ConfirmHit(false); // A later nonlethal hit must not remove the kill toast.
					Check(KillUntil > Rules->GetWorld()->GetTimeSeconds() && PC->GetKillMarkerUntil() == KillUntil, TEXT("kill toast survives subsequent hit"));
				}
                Check(Shooter->GetPlayerState<AFPSDemoPlayerState>()->GetEliminationCount() == 1, TEXT("GAS kill score equals one"));
            }
        }
        else if (Step == 9)
        {
            Check(VictimController.IsValid() && VictimController->GetPawn() && VictimController->GetPawn() != Victim.Get(), TEXT("server respawn"));
            // End a round while another respawn timer is pending.
            if (VictimController.IsValid())
            {
                Victim = Cast<AFPSDemoCharacter>(VictimController->GetPawn());
                auto* ASC = Victim->GetAbilitySystemComponent();
                const auto Spec = ASC->MakeOutgoingSpec(UFPSWeaponDamageEffect::StaticClass(), 1, FGameplayEffectContextHandle(new FGameplayEffectContext()));
                Spec.Data->SetSetByCallerMagnitude(FName(TEXT("WeaponDamage")), -1000.f);
                ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
            }
            Rules->FinishRound();
            Check(GS->bFinished && GS->Podium.Num() > 0 && GS->Podium.Num() <= 3, TEXT("final podium snapshot"));
            Check(!Rules->IsCombatActive(), TEXT("combat stops at finish"));
            if (Shooter.IsValid() && VictimController.IsValid())
            {
                auto* Respawned = Cast<AFPSDemoCharacter>(VictimController->GetPawn());
                const float Before = Respawned ? Respawned->GetHealth() : -1;
                Shooter->ApplyDamageEffectTo(Respawned, FFPSWeaponShotContext::Capture(Shooter.Get(), Shooter->GetWeapons()->Equipped(), *Shooter->GetWeapons()->Definition()));
                Check(Respawned && Respawned->GetHealth() == Before, TEXT("post-match damage rejected"));
            }
        }
        else if (Step == 13)
        {
            Check(Victim.IsValid() && VictimController.IsValid() && VictimController->GetPawn() == Victim.Get(), TEXT("finished round preserves death view"));
        }
        ++Step;
    }, 1.0f, true, 5.0f);
#endif
}
