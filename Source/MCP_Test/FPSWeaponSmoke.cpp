#include "FPSWeaponSmoke.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponPickup.h"
#include "FPSMovementComponent.h"
#include "FPSMatchFlow.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoAttributeSet.h"
#include "FPSDemoGameState.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "TimerManager.h"

void FFPSWeaponSmoke::Start(AFPSMatchMode* Mode)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSWeaponSmoke"))) return;
    FTimerHandle Timer;
    TWeakObjectPtr<AFPSMatchMode> WeakMode(Mode);
    Mode->GetWorldTimerManager().SetTimer(Timer, [WeakMode]()
    {
        if (auto* Rules = WeakMode.Get())
            if (auto* GS = Rules->GetGameState<AFPSDemoGameState>(); GS && GS->bWaiting && GS->PlayerArray.Num() == 2)
                Rules->StartRound(Rules->GetWorld()->GetFirstPlayerController());
    }, 1.f, true, 3.f);
#endif
}

void FFPSWeaponSmoke::Tick(UFPSWeaponComponent* W)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSWeaponSmoke")) || !W->CanAct()) return;
    auto* Pawn = W->Character();
    const bool bServer = Pawn->HasAuthority();
    // Exercise the remote player's actual RPC path, not the listen host's local functions.
    if (bServer == Pawn->IsLocallyControlled()) return;
    const auto* GS = Pawn->GetWorld()->GetGameState<AFPSDemoGameState>();
    const double Time = W->ServerTime() - (GS->RoundEndsAt - 300);
    struct FProgress { TSet<int32> Done; FGuid Original; FGuid NewAK; bool bFailed = false; };
    static TMap<TWeakObjectPtr<UFPSWeaponComponent>, FProgress> Progress;
    auto& P = Progress.FindOrAdd(W);
    auto Check = [&P, bServer](bool bPass, const TCHAR* What)
    {
        P.bFailed |= !bPass;
        UE_LOG(LogTemp, Display, TEXT("FPS_WEAPON_SMOKE %s %s %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), bServer ? TEXT("SERVER") : TEXT("CLIENT"), What);
    };
    auto At = [&P, Time](int32 Tenths, TFunction<void()> Action)
    {
        if (Time >= Tenths / 10.0 && !P.Done.Contains(Tenths)) { P.Done.Add(Tenths); Action(); }
    };
    auto Find = [Pawn](EFPSWeaponModel Model, int32 Magazine)
    {
        for (TActorIterator<AFPSWeaponPickup> It(Pawn->GetWorld()); It; ++It)
            if (It->IsAvailable() && It->GetInstance().Model == Model && It->GetInstance().Magazine == Magazine) return *It;
        return static_cast<AFPSWeaponPickup*>(nullptr);
    };
    auto* Movement = CastChecked<UFPSMovementComponent>(Pawn->GetCharacterMovement());
    if (bServer)
    {
        At(0, [&]()
        {
            W->Loadout.Primary.Magazine = 2; W->Loadout.Primary.Reserve = 5;
            P.Original = W->Loadout.Primary.InstanceId;
            Pawn->SetActorLocation(FVector(400, 0, 100), false, nullptr, ETeleportType::TeleportPhysics);
            W->Changed();
            Check(W->Definition() && W->Definition()->Damage == 25, TEXT("M4 definition and initial inventory"));
        });
        At(28, [&]() { Check(W->Loadout.Primary.Magazine == 0 && W->Loadout.Primary.Reserve == 5, TEXT("automatic fire stops at empty magazine")); });
        At(35, [&]() { Check(W->Loadout.Action == EFPSWeaponAction::Reloading && !W->FireAuthority(), TEXT("reload blocks authoritative fire")); });
        At(49, [&]() { Check(W->Loadout.Primary.Magazine == 0 && W->Loadout.Primary.Reserve == 5, TEXT("switch cancels reload without granting ammo")); });
        At(74, [&]() { Check(W->Loadout.Primary.Magazine == 5 && W->Loadout.Primary.Reserve == 0, TEXT("partial reload conserves ammunition")); });
        At(80, [&]() { Check(W->Loadout.bAiming, TEXT("client ADS reaches authority")); });
        At(88, [&]() { Check(W->Loadout.Action == EFPSWeaponAction::Inspecting, TEXT("client inspect reaches authority")); });
        At(94, [&]() { Check(W->Loadout.Primary.Magazine == 4 && W->Loadout.Action == EFPSWeaponAction::Idle, TEXT("fire interrupts inspect")); });
        At(104, [&]() { Check(W->Loadout.Primary.Magazine == 4 && W->Loadout.Secondary.Magazine == 14, TEXT("slots retain independent ammunition")); });
        At(116, [&]() { Check(!W->Loadout.Primary.IsValid(), TEXT("drop removes owned instance")); });
        At(126, [&]() { Check(W->Loadout.Primary.InstanceId == P.Original && W->Loadout.Primary.Magazine == 4, TEXT("pickup preserves identity and ammo")); });
        At(132, [&]()
        {
            auto Instance = W->Catalog()->CreateInstance(EFPSWeaponModel::HeavyPistol); Instance.Magazine = 3;
            auto* Pickup = Pawn->GetWorld()->SpawnActor<AFPSWeaponPickup>(FVector(1000, 1000, 30), FRotator::ZeroRotator);
            Pickup->Initialize(Instance);
        });
        At(145, [&]() { Check(W->Loadout.Primary.InstanceId == P.Original && W->Loadout.Secondary.Model == EFPSWeaponModel::M9, TEXT("distant pickup RPC rejected")); });
        At(150, [&]()
        {
            auto Instance = W->Catalog()->CreateInstance(EFPSWeaponModel::AK); Instance.Magazine = 7; Instance.Reserve = 17;
            P.NewAK = Instance.InstanceId;
            auto* Pickup = Pawn->GetWorld()->SpawnActor<AFPSWeaponPickup>(Pawn->GetActorLocation() + FVector(40, 0, -35), FRotator::ZeroRotator);
            Pickup->Initialize(Instance);
        });
        At(165, [&]()
        {
            int32 Duplicate = 0, PreservedM4 = 0;
            for (TActorIterator<AFPSWeaponPickup> It(Pawn->GetWorld()); It; ++It)
            {
                if (It->GetInstance().InstanceId == P.NewAK) ++Duplicate;
                if (It->GetInstance().InstanceId == P.Original && It->GetInstance().Magazine == 4) ++PreservedM4;
            }
            Check(W->Loadout.Primary.InstanceId == P.NewAK && W->Loadout.Primary.Magazine == 7 && Duplicate == 0 && PreservedM4 == 1, TEXT("duplicate pickup requests cannot duplicate a weapon"));
        });
        At(187, [&]() { Check(Movement->IsSprinting() && Pawn->GetVelocity().Size2D() > 600, TEXT("sprint saved move reaches server")); });
        At(206, [&]() { Check(Pawn->bIsCrouched && FMath::IsNearlyEqual(Movement->GetMaxSpeed(), 285.f), TEXT("crouch replicated movement")); });
        At(217, [&]() { Check(!Pawn->bIsCrouched, TEXT("uncrouch restored")); });
        At(238, [&]() { Check(W->Loadout.Primary.Magazine == 7 && W->Loadout.Action == EFPSWeaponAction::Idle && !W->Loadout.bAiming, TEXT("menu blocks local weapon requests")); });
        At(250, [&]() { Check(!P.bFailed, TEXT("COMPLETE")); });
    }
    else
    {
        At(15, [&]() { P.Original = W->Loadout.Primary.InstanceId; Pawn->GetController()->SetControlRotation(FRotator(45, 90, 0)); });
        At(20, [&]() { W->StartFire(); });
        At(25, [&]() { W->StopFire(); });
        At(30, [&]() { W->Reload(); });
        At(40, [&]() { W->Secondary(); });
        At(45, [&]() { W->Primary(); });
        At(50, [&]() { W->Reload(); });
        At(75, [&]() { Check(W->Loadout.Primary.Magazine == 5 && W->Loadout.Primary.Reserve == 0, TEXT("reload result replicated")); });
        At(76, [&]() { W->AimOn(); });
        At(82, [&]() { W->AimOff(); });
        At(85, [&]() { W->Inspect(); });
        At(90, [&]() { W->StartFire(); W->StopFire(); });
        At(96, [&]() { W->Secondary(); });
        At(101, [&]() { W->StartFire(); W->StopFire(); });
        At(106, [&]() { W->Primary(); });
        At(112, [&]() { W->Drop(); });
        At(120, [&]() { W->Pickup(); });
        At(130, [&]() { Check(W->Loadout.Primary.InstanceId == P.Original && W->Loadout.Primary.Magazine == 4, TEXT("pickup identity replicated")); });
        At(140, [&]() { W->Request(EFPSWeaponRequest::Pickup, Find(EFPSWeaponModel::HeavyPistol, 3)); });
        At(160, [&]() { auto* Pickup = Find(EFPSWeaponModel::AK, 7); W->Request(EFPSWeaponRequest::Pickup, Pickup); W->Request(EFPSWeaponRequest::Pickup, Pickup); });
        At(170, [&]() { Check(W->Loadout.Primary.Model == EFPSWeaponModel::AK && W->Loadout.Primary.Magazine == 7 && Pawn->GetAbilitySystemComponent()->GetNumericAttribute(UFPSDemoAttributeSet::GetWeaponDamageAttribute()) == 34, TEXT("new weapon and GAS damage replicated")); });
        At(180, [&]() { Movement->bWantsSprint = true; });
        // Foundry has cover north of the fixture; sprint south along the clear corridor.
        if (Time >= 18 && Time < 19) Pawn->AddMovementInput(FVector(0, -1, 0), 1);
        At(190, [&]() { Movement->bWantsSprint = false; });
        At(200, [&]() { Pawn->Crouch(); });
        At(210, [&]() { Pawn->UnCrouch(); });
        At(230, [&]()
        {
            CastChecked<APlayerController>(Pawn->GetController())->SetIgnoreMoveInput(true);
            W->StartFire(); W->Reload(); W->AimOn();
            Check(!W->bTriggerHeld, TEXT("menu cannot start automatic fire"));
        });
        At(240, [&]() { CastChecked<APlayerController>(Pawn->GetController())->SetIgnoreMoveInput(false); });
        At(245, [&]() { Check(!P.bFailed, TEXT("COMPLETE")); });
    }
#endif
}
