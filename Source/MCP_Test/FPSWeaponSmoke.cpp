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
#include "Components/StaticMeshComponent.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

void FFPSWeaponSmoke::Start(AFPSMatchMode* Mode)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSAttachmentSmoke")) && !FParse::Param(FCommandLine::Get(), TEXT("FPSWeaponSmoke")) && !FParse::Param(FCommandLine::Get(), TEXT("FPSWeaponAbilitySmoke")) && !FParse::Param(FCommandLine::Get(), TEXT("FPSWeaponDamageSmoke")) && !FParse::Param(FCommandLine::Get(), TEXT("FPSWeaponPresentationSmoke")) && !FParse::Param(FCommandLine::Get(), TEXT("FPSRecoilSmoke"))) return;
    FTimerHandle Timer;
    TWeakObjectPtr<AFPSMatchMode> WeakMode(Mode);
    Mode->GetWorldTimerManager().SetTimer(Timer, [WeakMode]()
    {
        if (auto* Rules = WeakMode.Get())
            if (auto* GS = Rules->GetGameState<AFPSDemoGameState>(); GS && GS->bWaiting && (FParse::Param(FCommandLine::Get(), TEXT("FPSAttachmentSmoke")) ? Rules->GetWorld()->GetNumPlayerControllers() >= 2 : GS->PlayerArray.Num() == 2))
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
    struct FProgress { TSet<int32> Done; FGuid Original; FGuid NewAK; FGuid BurstId; FVector SprintOrigin = FVector::ZeroVector; bool bFailed = false; };
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
    auto Find = [Pawn](FName WeaponId, int32 Magazine)
    {
        for (TActorIterator<AFPSWeaponPickup> It(Pawn->GetWorld()); It; ++It)
            if (It->IsAvailable() && It->GetInstance().WeaponId == WeaponId && It->GetInstance().Magazine == Magazine) return *It;
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
        At(35, [&]() { Check(W->Loadout.Action == EFPSWeaponAction::Reloading && !W->CommitShot(FFPSWeaponActionTicket()), TEXT("reload blocks authoritative fire")); });
        At(49, [&]() { Check(W->Loadout.Primary.Magazine == 0 && W->Loadout.Primary.Reserve == 5, TEXT("switch cancels reload without granting ammo")); });
        At(74, [&]() { Check(W->Loadout.Primary.Magazine == 5 && W->Loadout.Primary.Reserve == 0, TEXT("partial reload conserves ammunition")); });
        At(80, [&]() { Check(W->Loadout.bAiming, TEXT("client ADS reaches authority")); });
        At(88, [&]() { Check(W->Loadout.Action == EFPSWeaponAction::Inspecting, TEXT("client inspect reaches authority")); });
        At(94, [&]() { Check(W->Loadout.Primary.Magazine == 4 && W->Loadout.Action == EFPSWeaponAction::Idle, TEXT("fire interrupts inspect")); });
        At(104, [&]() { Check(W->Loadout.Primary.Magazine == 4 && W->Loadout.Secondary.Magazine == 14, TEXT("slots retain independent ammunition")); });
        At(116, [&]() { Check(!W->Loadout.Primary.IsValid(), TEXT("drop removes owned instance")); });
        At(117, [&]()
        {
            for (TActorIterator<AFPSWeaponPickup> It(Pawn->GetWorld()); It; ++It)
                if (It->GetInstance().InstanceId == P.Original)
                    Pawn->SetActorLocation(It->GetActorLocation() + FVector(0,0,90), false, nullptr, ETeleportType::TeleportPhysics);
        });
        At(126, [&]() { Check(W->Loadout.Primary.InstanceId == P.Original && W->Loadout.Primary.Magazine == 4, TEXT("pickup preserves identity and ammo")); });
        At(132, [&]()
        {
            auto Instance = W->Catalog()->CreateInstance(FName(TEXT("Weapon.HeavyPistol"))); Instance.Magazine = 3;
            auto* Pickup = Pawn->GetWorld()->SpawnActor<AFPSWeaponPickup>(FVector(1000, 1000, 30), FRotator::ZeroRotator);
            Pickup->Initialize(Instance);
        });
        At(145, [&]() { Check(W->Loadout.Primary.InstanceId == P.Original && W->Loadout.Secondary.WeaponId == FName(TEXT("Weapon.M9")), TEXT("distant pickup RPC rejected")); });
        At(150, [&]()
        {
            auto Instance = W->Catalog()->CreateInstance(FName(TEXT("Weapon.AK"))); Instance.Magazine = 7; Instance.Reserve = 17;
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
        At(180, [&]() { P.SprintOrigin = Pawn->GetActorLocation(); });
        At(187, [&]() { Check(Movement->IsSprinting() && Pawn->GetVelocity().Size2D() > 600 &&
            FVector::DotProduct(Pawn->GetActorLocation() - P.SprintOrigin, FVector(0,-1,0)) > 200,
            TEXT("forward sprint saved move and actual displacement reach server")); });
        At(206, [&]() { Check(Pawn->bIsCrouched && FMath::IsNearlyEqual(Movement->GetMaxSpeed(), 240.f), TEXT("crouch replicated movement")); });
        At(217, [&]() { Check(!Pawn->bIsCrouched, TEXT("uncrouch restored")); });
        At(238, [&]() { Check(W->Loadout.Primary.Magazine == 7 && W->Loadout.Action == EFPSWeaponAction::Idle && !W->Loadout.bAiming, TEXT("menu blocks local weapon requests")); });
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
        At(125, [&]() { W->Primary(); });
        At(130, [&]() { Check(W->Loadout.Primary.InstanceId == P.Original && W->Loadout.Primary.Magazine == 4, TEXT("pickup identity replicated")); });
        At(140, [&]() { W->Request(EFPSWeaponRequest::Pickup, Find(FName(TEXT("Weapon.HeavyPistol")), 3)); });
        At(160, [&]() { auto* Pickup = Find(FName(TEXT("Weapon.AK")), 7); W->Request(EFPSWeaponRequest::Pickup, Pickup); W->Request(EFPSWeaponRequest::Pickup, Pickup); });
        At(170, [&]() { Check(W->Loadout.Primary.WeaponId == FName(TEXT("Weapon.AK")) && W->Loadout.Primary.Magazine == 7 && Pawn->GetAbilitySystemComponent()->GetNumericAttribute(UFPSDemoAttributeSet::GetWeaponDamageAttribute()) == 34, TEXT("new weapon and GAS damage replicated")); });
        At(180, [&]()
        {
            Pawn->GetController()->SetControlRotation(FRotator(45,-90,0));
            Pawn->SetActorRotation(FRotator(0,-90,0));
            P.SprintOrigin = Pawn->GetActorLocation();
            Movement->RequestSprint(true);
        });
        // Foundry has cover north of the fixture; sprint south along the clear corridor.
        if (Time >= 18 && Time < 19) Pawn->AddMovementInput(FVector(0, -1, 0), 1);
        At(187, [&]() { Check(Movement->IsSprinting() && Pawn->GetVelocity().Size2D() > 600 &&
            FVector::DotProduct(Pawn->GetActorLocation() - P.SprintOrigin, FVector(0,-1,0)) > 200,
            TEXT("owner predicts forward sprint with actual displacement")); });
        At(190, [&]() { Movement->RequestSprint(false); });
        At(200, [&]() { Movement->RequestCrouch(true); });
        At(210, [&]() { Movement->RequestCrouch(false); });
        At(230, [&]()
        {
            CastChecked<APlayerController>(Pawn->GetController())->SetIgnoreMoveInput(true);
            W->StartFire(); W->Reload(); W->AimOn();
            Check(!W->bTriggerHeld, TEXT("menu cannot start automatic fire"));
        });
        At(240, [&]() { CastChecked<APlayerController>(Pawn->GetController())->SetIgnoreMoveInput(false); });
    }
    // Exercise all migrated definitions over the remote player's real RPC path.
    const FName Models[] = {TEXT("Weapon.AK"), TEXT("Weapon.M4"), TEXT("Weapon.MP5"), TEXT("Weapon.UMP45"), TEXT("Weapon.M9"), TEXT("Weapon.HeavyPistol")};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Models); ++Index)
    {
        const FName Id = Models[Index];
        const int32 Start = 260 + Index * 25;
        if (bServer)
        {
            At(Start, [&]()
            {
                const auto* Def = W->Catalog()->Find(Id);
                Check(Def && !Def->HasAnyFlags(RF_Transient) && Def->RecoilCurve, TEXT("saved definition and recoil loaded"));
                if (!Def) return;
                auto Instance = W->Catalog()->CreateInstance(Id);
                Check(Instance.Magazine == Def->InitialMagazine && Instance.Reserve == Def->InitialReserve, TEXT("initial ammunition from definition"));
                Instance.Magazine = 1;
                (Def->Slot == EFPSWeaponSlot::Primary ? W->Loadout.Primary : W->Loadout.Secondary) = Instance;
                W->Loadout.ActiveSlot = Def->Slot;
                W->Changed();
            });
            At(Start + 11, [&]()
            {
                Check(W->Equipped().WeaponId == Id && W->Equipped().Magazine == 0, *FString::Printf(TEXT("six-gun authoritative shot %s"), *Id.ToString()));
            });
        }
        else
        {
            At(Start + 5, [&]()
            {
                const auto* Def = W->Definition();
                Check(Def && Def->WeaponId == Id && W->Equipped().Magazine == 1, *FString::Printf(TEXT("six-gun owner identity %s"), *Id.ToString()));
                if (!Def) return;
                TArray<UStaticMeshComponent*> Meshes;
                Pawn->GetComponents(Meshes);
                bool First = false, Third = false;
                for (auto* Mesh : Meshes)
                {
                    if (Mesh->GetName() == TEXT("FirstPersonRifle")) First = Mesh->GetStaticMesh() == Def->FirstPerson.Mesh.Get() && Mesh->GetRelativeTransform().Equals(Def->FirstPerson.Transform);
                    if (Mesh->GetName() == TEXT("WorldRifle")) Third = Mesh->GetStaticMesh() == Def->ThirdPerson.Mesh.Get();
                }
                Check(First && Third, TEXT("explicit first and third person meshes applied"));
                Pawn->GetController()->SetControlRotation(FRotator::ZeroRotator);
                W->StartFire(); W->StopFire();
            });
            At(Start + 13, [&]()
            {
                if (FParse::Param(FCommandLine::Get(), TEXT("FPSWeaponCapture")))
                    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/WindowsEditor") / (Id.ToString() + TEXT(".png")), false, false);
            });
            At(Start + 10, [&]() { Check(W->Equipped().WeaponId == Id && W->Equipped().Magazine == 0, TEXT("six-gun ammo correction replicated")); });
        }
    }
    // Saved extension asset, acquired from the actual map stand; no definition mutation.
    if (bServer)
    {
        At(420, [&]()
        {
            int32 Count = 0;
            AFPSWeaponPickup* Target = nullptr;
            for (TActorIterator<AFPSWeaponPickup> It(Pawn->GetWorld()); It; ++It)
                if (It->IsAvailable() && It->GetInstance().WeaponId == FName(TEXT("Weapon.BR3"))) { ++Count; Target = *It; }
            Check(Count == 2, TEXT("BR3 spawned from both saved Foundry stands"));
            if (Target) { P.BurstId = Target->GetInstance().InstanceId; Target->SetActorLocation(Pawn->GetActorLocation() + FVector(40,0,-35)); }
        });
        At(440, [&]() { Check(W->Equipped().InstanceId == P.BurstId && W->Definition()->FireMode == EFPSFireMode::Burst, TEXT("BR3 map instance acquired through GAS")); });
        At(470, [&]() { Check(W->Equipped().Magazine == 27 && W->Equipped().RecoilIndex == 3, TEXT("saved BR3 release completes three shots; cooldown click not queued")); });
        At(487, [&]() { Check(W->Equipped().Magazine == 24, TEXT("saved BR3 fresh press fires next burst")); });
        At(520, [&]() { Check(W->Equipped().Magazine == 30 && W->Equipped().Reserve == 84, TEXT("saved BR3 reload conserves ammunition")); });
        At(580, [&]() { Check(W->Equipped().InstanceId == P.BurstId && W->Equipped().Reserve == 84, TEXT("saved BR3 drop pickup preserves instance")); });
        At(552, [&]()
        {
            for (TActorIterator<AFPSWeaponPickup> It(Pawn->GetWorld()); It; ++It)
                if (It->GetInstance().InstanceId == P.BurstId)
                    Pawn->SetActorLocation(It->GetActorLocation() + FVector(0,0,90), false, nullptr, ETeleportType::TeleportPhysics);
        });
    }
    else
    {
        At(430, [&]() { W->Pickup(); });
        At(435, [&]() { W->Primary(); }); // Ordinary pickup keeps the active secondary slot.
        At(445, [&]()
        {
            P.BurstId = W->Equipped().InstanceId; W->LocalShotPresentations = 0;
            Check(W->Equipped().WeaponId == FName(TEXT("Weapon.BR3")) && W->Equipped().Magazine == 30, TEXT("saved BR3 owner identity and ammo"));
            Pawn->GetController()->SetControlRotation(FRotator(45,90,0));
        });
        At(450, [&]() { W->StartFire(); W->StopFire(); });
        At(454, [&]() { W->StartFire(); W->StopFire(); });
        At(473, [&]() { Check(W->Equipped().Magazine == 27 && W->LocalShotPresentations == 3, TEXT("saved BR3 predicts all three shots and converges")); });
        At(480, [&]() { W->StartFire(); W->StopFire(); });
        At(490, [&]() { W->Reload(); });
        At(523, [&]() { Check(W->Equipped().Magazine == 30 && W->Equipped().Reserve == 84, TEXT("saved BR3 reload reaches owner")); });
        At(530, [&]()
        {
            Pawn->GetController()->SetControlRotation(FRotator::ZeroRotator);
            if (FParse::Param(FCommandLine::Get(),TEXT("FPSWeaponCapture")))
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/WindowsEditor/Weapon.BR3.png"),false,false);
        });
        At(540, [&]() { W->Drop(); });
        At(560, [&]() { W->Pickup(); });
        At(565, [&]() { W->Primary(); });
        At(583, [&]() { Check(W->Equipped().InstanceId == P.BurstId && W->Equipped().Reserve == 84, TEXT("saved BR3 transferred identity reaches owner")); });
    }
    At(600, [&]() { Check(!P.bFailed, TEXT("COMPLETE")); });
#endif
}
