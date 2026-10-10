#include "FPSRecoilSmoke.h"
#include "FPSWeaponComponent.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"

void FFPSRecoilSmoke::Tick(UFPSWeaponComponent* W)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSRecoilSmoke")) || !W->IsEnabled()) return;
    auto* Pawn = Cast<AFPSDemoCharacter>(W->GetOwner());
    const bool Server = Pawn->HasAuthority();
    if (Server == Pawn->IsLocallyControlled()) return;
    const auto* GS = W->GetWorld()->GetGameState<AFPSDemoGameState>();
    if (!GS || GS->bWaiting) return;
    const double Age = W->ServerTime() - (GS->RoundEndsAt - 300);
    AFPSDemoCharacter* Target = nullptr;
    for (TActorIterator<AFPSDemoCharacter> It(W->GetWorld()); It; ++It) if (*It != Pawn) { Target = *It; break; }
    if (!Target) return;
    struct FProgress { bool Setup = false, Fired = false, Released = false, Done = false, SampledAim = false; float FirstPitch = 0, MaxPitchDelta = 0; };
    static TMap<TWeakObjectPtr<UFPSWeaponComponent>, FProgress> Runs;
    auto& P = Runs.FindOrAdd(W);
    if (Age >= 2 && Age < 6 && Pawn->GetController())
    {
        const float Pitch = Pawn->GetController()->GetControlRotation().Pitch;
        if (!P.SampledAim) { P.SampledAim = true; P.FirstPitch = Pitch; }
        P.MaxPitchDelta = FMath::Max(P.MaxPitchDelta, FMath::Abs(FMath::FindDeltaAngleDegrees(P.FirstPitch, Pitch)));
    }
    if (Server && !P.Setup)
    {
        P.Setup = true;
        Pawn->SetActorLocation(FVector(-400,0,100));
        Pawn->GetCharacterMovement()->DisableMovement();
        Target->GetCharacterMovement()->DisableMovement();
        Target->GetAbilitySystemComponent()->SetNumericAttributeBase(UFPSDemoAttributeSet::GetMaxHealthAttribute(), 10000);
        Target->GetAbilitySystemComponent()->SetNumericAttributeBase(UFPSDemoAttributeSet::GetHealthAttribute(), 10000);
    }
    if (Server) Target->SetActorLocation(FVector(400, 100 * FMath::Sin(Age * 1.5), 100));
    else
    {
        // Track the replicated moving target. Recoil is applied after this input by the real fire Ability.
        if (!P.Fired || !FParse::Param(FCommandLine::Get(), TEXT("FPSRecoilUncompensated")))
            Pawn->GetController()->SetControlRotation((Target->GetActorLocation() + FVector(0,0,30) - Pawn->GetPawnViewLocation()).Rotation());
        if (Age >= 2 && !P.Fired) { P.Fired = true; W->StartFire(); }
        if (Age >= 6 && !P.Released) { P.Released = true; W->StopFire(); }
    }
    if (Age >= 9 && !P.Done)
    {
        P.Done = true;
        const float Damage = 10000 - Target->GetHealth();
        const bool Uncompensated = FParse::Param(FCommandLine::Get(), TEXT("FPSRecoilUncompensated"));
        const bool Pass = W->Equipped().ShotSerial == 30 && W->Equipped().Magazine == 0 &&
            (Uncompensated ? P.MaxPitchDelta > 20.f : Damage > 0);
        UE_LOG(LogTemp, Display, TEXT("FPS_RECOIL_SMOKE %s %s COMPLETE shots=%u damage=%.1f movingTarget=1 uncompensated=%d pitchSpan=%.2f"),
            Pass ? TEXT("PASS") : TEXT("FAIL"), Server ? TEXT("SERVER") : TEXT("CLIENT"), W->Equipped().ShotSerial, Damage, Uncompensated, P.MaxPitchDelta);
    }
#endif
}
