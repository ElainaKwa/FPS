#include "FPSCharacterSmoke.h"
#include "AbilitySystemComponent.h"
#include "FPSDemoCharacter.h"
#include "FPSLifeComponent.h"
#include "FPSActionPolicy.h"
#include "FPSModeRules.h"
#include "FPSBaseRegion.h"
#include "FPSBaseMembership.h"
#include "FPSCharacterInput.h"
#include "FPSMatchFlow.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "FPSDemoEffects.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponShot.h"
#include "FPSMovementComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "TimerManager.h"

void FFPSCharacterSmoke::Start(AFPSMatchMode* Mode)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSCharacterSmoke"))) return;
    FTimerHandle Timer;
    Mode->GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(Mode, [Mode]()
    {
        auto* GS = Mode->GetGameState<AFPSDemoGameState>();
        if (!GS->bWaiting || GS->PlayerArray.Num() != 2) return;
        for (auto It = Mode->GetWorld()->GetPlayerControllerIterator(); It; ++It)
            if (!CastChecked<AFPSFrontController>(It->Get())->IsWeaponContentVerified()) return;
        Mode->StartRound(Mode->GetWorld()->GetFirstPlayerController());
    }), .2f, true);
#endif
}
void FFPSCharacterSmoke::Tick(AFPSDemoCharacter* Pawn)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSCharacterSmoke")) || !Pawn->IsLocallyControlled()) return;
    auto* World = Pawn->GetWorld();
    auto* GS = World->GetGameState<AFPSDemoGameState>();
    if (!GS || GS->bWaiting || GS->RoundEndsAt <= 0 || GS->PlayerArray.Num() != 2) return;
    const bool Server = Pawn->HasAuthority();
    const double Time = GS->GetServerWorldTimeSeconds() - (GS->RoundEndsAt - 300);
    struct FProgress
    {
        TSet<int32> Done;
        bool Failed = false;
        TWeakObjectPtr<AController> VictimController;
        TWeakObjectPtr<AFPSDemoCharacter> OldBody;
        FVector MovementStart;
        FGuid SpawnGun;
    };
    static TMap<TWeakObjectPtr<UWorld>, FProgress> Progress;
    auto& P = Progress.FindOrAdd(World);
    auto Check = [&](bool Pass, const TCHAR* Name)
    {
        P.Failed |= !Pass;
        UE_LOG(LogTemp, Display, TEXT("FPS_CHARACTER_SMOKE %s %s %s"), Pass ? TEXT("PASS") : TEXT("FAIL"), Server ? TEXT("SERVER") : TEXT("CLIENT"), Name);
    };
    auto At = [&](int32 Tenth, TFunction<void()> Action)
    { if (Time >= Tenth * .1 && !P.Done.Contains(Tenth)) { P.Done.Add(Tenth); Action(); } };
    if (Server)
    {
        At(5, [&]()
        {
            TArray<FString> Errors;
            Check(AFPSBaseRegion::ValidateWorld(World, Errors), TEXT("two base regions cover all starts and valid fallback ownership"));
            const auto* Rules = UFPSModeRules::ForWorld(World);
            Check(Rules && Rules->Kind == EFPSGameplayKind::TeamMatch && Rules->DeathDrop == EFPSDeathDropPolicy::None,
                TEXT("LAN retains team-match asset identity and no death-drop policy"));
            Check(Pawn->GetWeapons()->Catalog()->RequiredIds().Contains(TEXT("Weapon.BR3")), TEXT("all selection definitions are admitted content"));
            for (auto It = World->GetPlayerControllerIterator(); It; ++It)
                if (It->Get() != Pawn->GetController()) { P.VictimController = It->Get(); P.OldBody = Cast<AFPSDemoCharacter>(It->Get()->GetPawn()); break; }
            Check(P.OldBody.IsValid(), TEXT("remote admitted pawn exists"));
            if (!P.OldBody.IsValid()) return;
            auto* Victim = P.OldBody.Get();
            const auto* PS = Victim->GetPlayerState<AFPSDemoPlayerState>();
            bool bOwned = false;
            for (TActorIterator<AFPSBaseRegion> It(World); It; ++It)
                bOwned |= It->TeamId == PS->TeamId && It->Contains(Victim->GetActorLocation());
            Check(bOwned, TEXT("spawn is inside its own base"));
            P.SpawnGun = Victim->GetWeapons()->Equipped().InstanceId;
            // Real movement RPCs must still work without any weapon instances.
            Victim->GetWeapons()->Loadout = FFPSWeaponLoadout();
            Victim->GetWeapons()->Changed();
            Victim->SetActorLocation(FVector(-400,-1250,100));
            Victim->GetController()->SetControlRotation(FRotator::ZeroRotator);
            P.MovementStart = Victim->GetActorLocation();
            Pawn->SetActorLocation(FVector(-1200,-1250,100));
        });
        At(29, [&]()
        {
            auto* Victim = P.OldBody.Get();
            Check(Victim && FVector::Dist2D(Victim->GetActorLocation(), P.MovementStart) > 350,
                TEXT("server receives predicted client sprint with empty inventory"));
        });
        At(32, [&]()
        {
            auto* Victim = P.OldBody.Get();
            if (!Victim) { Check(false, TEXT("victim survives until injection")); return; }
            auto* ASC = Victim->GetAbilitySystemComponent();
            FActorSpawnParameters Params; Params.Name = TEXT("EnvironmentDamageTest");
            auto* Causer = World->SpawnActor<AActor>(Params);
            FGameplayEffectContextHandle Context(new FGameplayEffectContext());
            Context.AddInstigator(nullptr, Causer);
            auto Spec = ASC->MakeOutgoingSpec(UFPSWeaponDamageEffect::StaticClass(), 1, Context);
            Spec.Data->SetSetByCallerMagnitude(FName(TEXT("WeaponDamage")), -200.f);
            ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
            ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
            Victim->GetLife()->NotifyHealthExhausted(Context);
            Check(Victim->IsEliminated() && Victim->GetLife()->IsDead() && Victim->GetHealth() == 0,
                TEXT("non-pawn damage enters target GAS death and body transition"));
            Check(GS->GetEliminations() == 1 && Victim->GetPlayerState<AFPSDemoPlayerState>()->GetDeathCount() == 1 &&
                Pawn->GetPlayerState<AFPSDemoPlayerState>()->GetEliminationCount() == 0,
                TEXT("repeated effects and notification score once without inventing a killer"));
            Check(Victim->GetKillerName() == TEXT("EnvironmentDamageTest"), TEXT("causer attribution survives absent instigator"));
            Causer->Destroy();
        });
        At(65, [&]()
        {
            auto* NewBody = P.VictimController.IsValid() ? Cast<AFPSDemoCharacter>(P.VictimController->GetPawn()) : nullptr;
            Check(NewBody && NewBody != P.OldBody.Get() && NewBody->GetHealth() == 100 && !NewBody->GetLife()->IsDead(),
                TEXT("new life is a new pawn with fresh ASC and health"));
            Check(NewBody && NewBody->GetWeapons()->Equipped().InstanceId.IsValid() && NewBody->GetWeapons()->Equipped().InstanceId != P.SpawnGun &&
                NewBody->GetPlayerState<AFPSDemoPlayerState>()->GetDeathCount() == 1,
                TEXT("fresh default weapon instance and persistent player identity"));
        });
        At(75, [&]()
        {
            auto* Victim = Cast<AFPSDemoCharacter>(P.VictimController->GetPawn());
            Victim->SetActorLocation(FVector(1000,-1250,100));
            Victim->FindComponentByClass<UFPSBaseMembership>()->RefreshAuthority();
            const auto Shot = FFPSWeaponShotContext::Capture(Pawn, Pawn->GetWeapons()->Equipped(), *Pawn->GetWeapons()->Definition());
            for (int32 Hit = 0; Hit < 5; ++Hit) Pawn->ApplyDamageEffectTo(Victim, Shot);
            Check(Victim->IsEliminated() && GS->GetEliminations() == 2 && Pawn->GetPlayerState<AFPSDemoPlayerState>()->GetEliminationCount() == 1 &&
                Victim->GetKillerName() == Pawn->GetPlayerState<AFPSDemoPlayerState>()->GetPlayerName().Left(32),
                TEXT("weapon effect attribution and duplicate rejection without shooter death callback"));
        });
    }
    else
    {
        At(9, [&]()
        {
            P.OldBody = Pawn;
            Check(Pawn->CharacterInput && Pawn->GetActionPolicy()->CanMove() && !Pawn->GetWeapons()->Equipped().IsValid(),
                TEXT("owner has input component and movement permission without a gun"));
            P.MovementStart = Pawn->GetActorLocation();
        });
        if (Time >= 1 && Time < 2.4) { Pawn->SprintOn(); Pawn->MoveForward(FInputActionValue(1.f)); }
        At(25, [&]()
        {
            Pawn->SprintOff();
            Check(FVector::Dist2D(Pawn->GetActorLocation(), P.MovementStart) > 350,
                TEXT("owner predicted movement advances with empty inventory"));
        });
        At(38, [&]()
        {
            Check(Pawn->GetLife()->IsDead() && Pawn->IsEliminated() && Pawn->GetHealth() == 0 && GS->GetEliminations() == 1,
                TEXT("environment death health and score replicate"));
            Check(Pawn->GetKillerName() == TEXT("EnvironmentDamageTest") && !Pawn->GetActionPolicy()->CanMove(),
                TEXT("causer display and dead movement restriction replicate"));
        });
        At(68, [&]()
        {
            Check(Pawn != P.OldBody.Get() && !Pawn->IsEliminated() && !Pawn->GetLife()->IsDead() && Pawn->GetHealth() == 100 &&
                Pawn->GetPlayerState<AFPSDemoPlayerState>()->GetDeathCount() == 1 && Pawn->GetWeapons()->Equipped().IsValid(),
                TEXT("owner receives new life and default loadout with retained death count"));
        });
        At(82, [&]() { Check(Pawn->IsEliminated() && GS->GetEliminations() == 2 && Pawn->GetPlayerState<AFPSDemoPlayerState>()->GetDeathCount() == 2,
            TEXT("weapon-effect death and score replicate through same life endpoint")); });
    }
    At(90, [&]() { Check(!P.Failed, TEXT("COMPLETE")); });
#endif
}
