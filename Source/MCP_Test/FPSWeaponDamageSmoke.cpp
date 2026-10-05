#include "FPSWeaponDamageSmoke.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponShot.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoAttributeSet.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"

void FFPSWeaponDamageSmoke::Tick(UFPSWeaponComponent* W)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSWeaponDamageSmoke")) || !W->IsEnabled()) return;
    auto* Pawn = W->Character();
    const bool Server = Pawn->HasAuthority();
    if (Server == Pawn->IsLocallyControlled()) return;
    auto* GS = Pawn->GetWorld()->GetGameState<AFPSDemoGameState>();
    if (!GS || GS->bWaiting) return;
    AFPSDemoCharacter* Target = nullptr;
    for (TActorIterator<AFPSDemoCharacter> It(Pawn->GetWorld()); It; ++It)
        if (*It != Pawn && It->GetPlayerState()) { Target = *It; break; }
    if (!Target) return;
    const double Time = W->ServerTime() - (GS->RoundEndsAt - 300);
    struct FProgress { TSet<int32> Done; bool Failed = false; FFPSWeaponShotContext FirstShot; FDelegateHandle Probe; };
    static TMap<TWeakObjectPtr<UFPSWeaponComponent>, FProgress> Progress;
    auto& P = Progress.FindOrAdd(W);
    auto Check = [&](bool Pass, const TCHAR* What)
    {
        P.Failed |= !Pass;
        UE_LOG(LogTemp, Display, TEXT("FPS_DAMAGE_SMOKE %s %s %s health=%.3f weapon=%s mag=%d"),
            Pass ? TEXT("PASS") : TEXT("FAIL"), Server ? TEXT("SERVER") : TEXT("CLIENT"), What,
            Target->GetHealth(), *W->Equipped().WeaponId.ToString(), W->Equipped().Magazine);
    };
    auto At = [&](int32 T, auto Fn) { if (Time >= T / 10. && !P.Done.Contains(T)) { P.Done.Add(T); Fn(); } };
    auto Fire = [&]()
    {
        Pawn->GetController()->SetControlRotation((Target->GetActorLocation() + FVector(0,0,40) - Pawn->GetPawnViewLocation()).Rotation());
        W->StartFire(); W->StopFire();
    };
    At(0, [&]()
    {
        Pawn->GetCharacterMovement()->DisableMovement();
        if (!Server) return;
        Target->GetCharacterMovement()->DisableMovement();
        Pawn->SetActorLocation(FVector(0,0,2000)); Target->SetActorLocation(FVector(500,0,2000));
        Pawn->GetPlayerState<AFPSDemoPlayerState>()->TeamId = 0;
        Target->GetPlayerState<AFPSDemoPlayerState>()->TeamId = 1;
        Target->GetAbilitySystemComponent()->SetNumericAttributeBase(UFPSDemoAttributeSet::GetHealthAttribute(), 100);
        W->StopAllActions();
        W->Loadout.Primary = W->Catalog()->CreateInstance(TEXT("Weapon.AK"));
        W->Loadout.Secondary = W->Catalog()->CreateInstance(TEXT("Weapon.M9"));
        W->Loadout.ActiveSlot = EFPSWeaponSlot::Primary; W->Changed();
        P.Probe = W->OnFireReadinessChanged.AddLambda([W]()
        {
            auto& State = Progress.FindChecked(W);
            if (State.FirstShot.InstanceId.IsValid() || W->Equipped().ShotSerial == 0) return;
            State.FirstShot = FFPSWeaponShotContext::Capture(W->Character(), W->Equipped(), *W->Definition());
            UE_LOG(LogTemp, Display, TEXT("FPS_DAMAGE_PROBE server view=%s direction=%s"), *W->Character()->GetPawnViewLocation().ToString(), *W->Character()->GetController()->GetControlRotation().Vector().ToString());
            // Deliberately corrupt the display cache between commit and resolution, without changing the definition.
            W->Character()->GetAbilitySystemComponent()->SetNumericAttributeBase(UFPSDemoAttributeSet::GetWeaponDamageAttribute(), 999);
        });
    });
    At(5, [&]() { if (!Server) Pawn->GetController()->SetControlRotation((Target->GetActorLocation() + FVector(0,0,40) - Pawn->GetPawnViewLocation()).Rotation()); });
    At(15, [&]() { if (!Server) Fire(); });
    At(35, [&]()
    {
        Check(FMath::IsNearlyEqual(Target->GetHealth(), 66.f) && W->GetLoadout().Primary.Magazine == 29,
            TEXT("AK shot uses captured 34 damage despite display cache 999"));
        if (Server)
        {
            Check(P.FirstShot.WeaponId == FName(TEXT("Weapon.AK")) && P.FirstShot.InstanceId == W->Equipped().InstanceId &&
                P.FirstShot.ShotSerial == 1 && P.FirstShot.BaseDamage == 34, TEXT("committed shot identity and base damage captured"));
            W->OnFireReadinessChanged.Remove(P.Probe);
        }
    });
    At(40, [&]() { if (!Server) W->Secondary(); });
    At(55, [&]()
    {
        if (!Server) return;
        Check(W->Equipped().WeaponId == FName(TEXT("Weapon.M9")), TEXT("switch reached authority"));
        Target->GetAbilitySystemComponent()->SetNumericAttributeBase(UFPSDemoAttributeSet::GetHealthAttribute(), 100);
        // Boundary injection: resolve an already captured AK payload after switching. No claim of deferred hits in gameplay.
        Pawn->ApplyDamageEffectTo(Target, P.FirstShot);
        Check(FMath::IsNearlyEqual(Target->GetHealth(), 66.f), TEXT("captured AK payload keeps its damage after M9 equip"));
    });
    At(65, [&]()
    {
        Check(W->Equipped().WeaponId == FName(TEXT("Weapon.M9")) && FMath::IsNearlyEqual(Target->GetHealth(), 66.f),
            TEXT("switch and snapshot damage converge"));
        if (!Server)
        {
            FFPSWeaponShotContext Forged; Forged.Source = Pawn; Forged.InstanceId = W->Equipped().InstanceId;
            Forged.WeaponId = W->Equipped().WeaponId; Forged.BaseDamage = 999;
            Pawn->ApplyDamageEffectTo(Target, Forged);
            Check(FMath::IsNearlyEqual(Target->GetHealth(), 66.f), TEXT("client cannot apply forged damage locally"));
        }
    });
    At(70, [&]() { if (!Server) Fire(); });
    At(90, [&]() { Check(FMath::IsNearlyEqual(Target->GetHealth(), 41.f), TEXT("next M9 shot uses 25 damage")); });
    At(95, [&]() { if (Server) Target->GetPlayerState<AFPSDemoPlayerState>()->TeamId = 0; });
    At(105, [&]() { if (!Server) Fire(); });
    At(125, [&]() { Check(FMath::IsNearlyEqual(Target->GetHealth(), 41.f) && W->Equipped().ShotSerial == 2, TEXT("friendly shot consumes ammo but applies no damage")); });
    At(130, [&]()
    {
        if (!Server) return;
        GS->bFinished = true;
        Target->GetPlayerState<AFPSDemoPlayerState>()->TeamId = 1;
        Pawn->ApplyDamageEffectTo(Target, P.FirstShot);
        Check(FMath::IsNearlyEqual(Target->GetHealth(), 41.f), TEXT("post-match captured payload rejected"));
    });
    At(150, [&]() { Check(FMath::IsNearlyEqual(Target->GetHealth(), 41.f), TEXT("final health converges")); Check(!P.Failed, TEXT("COMPLETE")); });
#endif
}
