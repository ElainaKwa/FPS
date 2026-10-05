#include "FPSWeaponPresentationSmoke.h"
#include "FPSWeaponComponent.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void FFPSWeaponPresentationSmoke::Tick(UFPSWeaponComponent* W)
{
#if !UE_BUILD_SHIPPING
    const bool LateObserver = FParse::Param(FCommandLine::Get(), TEXT("FPSWeaponPresentationObserver"));
    if ((!LateObserver && !FParse::Param(FCommandLine::Get(), TEXT("FPSWeaponPresentationSmoke"))) || !W->IsEnabled()) return;
    auto* Pawn = W->Character();
    const bool Server = Pawn->HasAuthority();
    const auto* GS = W->GetWorld()->GetGameState<AFPSDemoGameState>();
    if (!GS || GS->bWaiting || GS->bFinished) return;
    const double Time = W->ServerTime() - (GS->RoundEndsAt - 300);
    if (LateObserver)
    {
        if (Server || Pawn->IsLocallyControlled()) return;
        struct FObservation { double EndsAt = 0, SeenAt = 0; bool Failed = false, Done = false, SawPose = false; };
        static TMap<TWeakObjectPtr<UFPSWeaponComponent>, FObservation> Observed;
        auto& O = Observed.FindOrAdd(W);
        const auto& State = W->GetLoadout();
        if (!O.EndsAt && State.Action == EFPSWeaponAction::Inspecting)
        {
            O.EndsAt = State.ActionEndsAt; O.SeenAt = W->ServerTime();
            O.Failed = W->LastResultSerial != 0 || State.ActionEndsAt <= W->ServerTime();
            UE_LOG(LogTemp, Display, TEXT("FPS_PRESENTATION_OBSERVER %s CLIENT joins during inspect; state restored without past start event remaining=%.2f"), O.Failed ? TEXT("FAIL") : TEXT("PASS"), O.EndsAt - O.SeenAt);
        }
        // Component tick can precede the character pose tick or see a newly arrived end snapshot.
        // Require an observed pose while the action is present, not a pose after its state ended.
        if (O.EndsAt && !O.Done && State.Action == EFPSWeaponAction::Inspecting)
            O.SawPose |= FMath::IsNearlyEqual(Pawn->WorldRifle->GetRelativeRotation().Roll, 40.f, .1f);
        if (O.EndsAt && !O.Done && W->ServerTime() > O.EndsAt + .5)
        {
            O.Done = true;
            const bool Pass = !O.Failed && O.SawPose && State.Action == EFPSWeaponAction::Idle && W->LastResultSerial > 0;
            UE_LOG(LogTemp, Display, TEXT("FPS_PRESENTATION_OBSERVER %s CLIENT COMPLETE pose=%d action=%d event=%llu"), Pass ? TEXT("PASS") : TEXT("FAIL"), O.SawPose, int32(State.Action), W->LastResultSerial);
        }
        return;
    }
    struct FProgress { TSet<int32> Done; bool Failed = false; };
    static TMap<TWeakObjectPtr<UFPSWeaponComponent>, FProgress> Progress;
    if (!Progress.Contains(W) && Time > 15) return; // late observer pawn is not a fixture driver
    auto& P = Progress.FindOrAdd(W);
    auto At = [&](int32 T, TFunction<void()> Action) { if (Time >= T * .1 && !P.Done.Contains(T)) { P.Done.Add(T); Action(); } };
    auto Check = [&](bool Pass, const TCHAR* What)
    {
        P.Failed |= !Pass;
        UE_LOG(LogTemp, Display, TEXT("FPS_PRESENTATION_SMOKE %s %s %s starts=%u commits=%u dedup=%u"), Pass ? TEXT("PASS") : TEXT("FAIL"), Server ? TEXT("SERVER") : TEXT("CLIENT"), What, W->ReloadStartPresentations, W->ReloadCommitPresentations, W->ConfirmedStartDuplicates);
    };
    auto* Def = const_cast<UFPSWeaponDefinition*>(W->Catalog()->Find(TEXT("Weapon.M4")));
    At(0, [&]() { Def->ReloadDuration = 1.f; Def->InspectDuration = 20.f; });
    // Listen host performs a separate action, observed on the actual remote client proxy.
    if (Server && Pawn->IsLocallyControlled())
    {
        At(45, [&]() { W->EquippedMutable().Magazine = 10; W->EquippedMutable().Reserve = 20; W->Changed(); });
        At(50, [&]() { W->Reload(); });
        At(160, [&]() { W->Inspect(); UE_LOG(LogTemp, Display, TEXT("FPS_PRESENTATION_LATE_WINDOW")); });
        return;
    }
    if (!Server && !Pawn->IsLocallyControlled())
    {
        At(55, [&]()
        {
            Check(W->GetLoadout().Action == EFPSWeaponAction::Reloading && W->ReloadStartPresentations == 1, TEXT("remote proxy receives reload state and one spatial start"));
            const auto Snapshot = W->VisibleState;
            W->Loadout = {}; W->OnRep_VisibleState();
            Check(W->GetLoadout().Action == Snapshot.Action && W->GetLoadout().ActionEndsAt == Snapshot.ActionEndsAt && W->Equipped().Reserve == 0 && W->ReloadStartPresentations == 1, TEXT("persistent snapshot restores in-progress pose without replaying audio or exposing ammo"));
        });
        At(57, [&]() { Check(Pawn->WorldRifle->GetRelativeRotation().Pitch < -20, TEXT("remote rendered component has reload pose")); });
        At(65, [&]() { Check(W->ReloadCommitPresentations == 1 && W->GetLoadout().Action == EFPSWeaponAction::Idle, TEXT("remote receives exactly one authoritative reload completion")); });
        return;
    }
    if (Server)
    {
        At(0, [&]() { W->EquippedMutable().Magazine = 10; W->EquippedMutable().Reserve = 20; W->Changed(); });
        At(28, [&]() { W->EquippedMutable().Magazine = 20; W->EquippedMutable().Reserve = 10; W->Changed(); });
        At(42, [&]() { Check(W->GetLoadout().Primary.Magazine == 20 && W->GetLoadout().Primary.Reserve == 10 && W->ReloadCommitPresentations == 1, TEXT("switch cancels reload without commit or completion sound")); });
        At(85, [&]() { W->EquippedMutable().Magazine = Def->MagazineSize; W->Changed(); });
        At(103, [&]() { Check(W->ReloadCommitPresentations == 1 && W->GetLoadout().Action == EFPSWeaponAction::Idle, TEXT("rejected client reload never commits on authority")); });
    }
    else
    {
        At(5, [&]() { W->StartFire(); W->StopFire(); });
        At(10, [&]() { W->Reload(); });
        At(27, [&]()
        {
            Check(W->ReloadStartPresentations == 1 && W->ReloadCommitPresentations == 1 && W->ConfirmedStartDuplicates == 1, TEXT("predicted reload start and authority confirmation play once"));
            W->EquippedMutable().Magazine = 0; W->OnRep_OwnerState();
            Check(W->ReloadCommitPresentations == 1, TEXT("ammo correction does not infer reload completion"));
            FFPSWeaponActionResult Duplicate;
            Duplicate.EventSerial = W->LastResultSerial; Duplicate.Phase = EFPSWeaponResultPhase::Committed;
            Duplicate.Command = EFPSWeaponRequest::Reload; Duplicate.WeaponId = W->Equipped().WeaponId;
            W->MulticastActionResult_Implementation(Duplicate);
            Check(W->ReloadCommitPresentations == 1, TEXT("duplicate authority result is ignored"));
        });
        At(30, [&]() { W->Reload(); });
        At(33, [&]() { W->Secondary(); });
        At(42, [&]() { Check(W->ReloadCommitPresentations == 1, TEXT("cancelled predicted reload has no completion cue")); });
        At(70, [&]() { W->Primary(); });
        At(78, [&]() { W->Inspect(); W->AimOn(); W->AimOff(); });
        At(90, [&]() { W->EquippedMutable().Magazine = 1; W->Reload(); });
        At(103, [&]() { Check(W->ReloadCommitPresentations == 1 && W->Equipped().Magazine == Def->MagazineSize && W->GetLoadout().Action == EFPSWeaponAction::Idle, TEXT("real GAS rejection restores ammo without false completion")); });
        At(110, [&]() { Pawn->BaseFieldOfView = 100; Def->AimTransitionTime = .2f; Def->EquipBlendTime = 5.f; W->AimOn(); });
        At(113, [&]() { Check(FMath::IsNearlyEqual(Pawn->FirstPersonCamera->FieldOfView, Def->AimFOV), TEXT("ADS reaches exact FOV within declared duration independent of equip")); });
        At(114, [&]() { W->AimOff(); });
        At(117, [&]() { Check(FMath::IsNearlyEqual(Pawn->FirstPersonCamera->FieldOfView, 100.f), TEXT("ADS exit restores camera-owned base FOV")); });
        At(120, [&]() { Def->AimTransitionTime = 0; W->AimOn(); });
        At(121, [&]() { Check(FMath::IsNearlyEqual(Pawn->FirstPersonCamera->FieldOfView, Def->AimFOV) && Pawn->AimVisualBlend.Value(W->GetWorld()->GetTimeSeconds()) == 1.f, TEXT("zero ADS duration snaps camera and gun blend")); });
        At(122, [&]() { Pawn->PlayWeaponShot(FVector(0,0,1)); });
        At(126, [&]() { Check(Pawn->Recoil == 0, TEXT("visual recoil returns exactly without waiting for equip blend")); });
        At(141, [&]() { Def->EquipBlendTime = 0; W->Secondary(); W->Primary(); });
        At(145, [&]() { Check(FVector::Dist(Pawn->GunRoot->GetRelativeLocation(), Def->HipLocation) < .1f, TEXT("zero equip duration reaches exact hip pose after real switch")); });
        At(146, [&]() { Def->EquipBlendTime = 1.f; W->Secondary(); W->Primary(); });
        At(147, [&]() { Check(Pawn->GunRoot->GetRelativeLocation().Z < Def->HipLocation.Z - 15.f, TEXT("positive equip duration retains independent lowering offset")); });
        if (Time >= 15.8 && W->ServerTime() - W->GetLoadout().EquipStartedAt >= Def->EquipBlendTime + .1f)
            At(159, [&]()
            {
                Check(FVector::Dist(Pawn->GunRoot->GetRelativeLocation(), Def->HipLocation) < .1f,
                    *FString::Printf(TEXT("positive equip endpoint age=%.3f duration=%.3f location=%s expected=%s"),
                        W->ServerTime() - W->GetLoadout().EquipStartedAt, Def->EquipBlendTime,
                        *Pawn->GunRoot->GetRelativeLocation().ToString(), *Def->HipLocation.ToString()));
            });
        At(225, [&]() { Check(P.Done.Contains(159), TEXT("authoritative equip deadline observed within test window")); });
    }
    At(135, [&]()
    {
        if (!Server && FParse::Param(FCommandLine::Get(), TEXT("FPSWeaponCapture")))
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/WindowsEditor/WeaponPresentation.png"), false, false);
    });
    At(130, [&]()
    {
        FPSWeaponVisualTiming::FTransition Blend;
        Blend.Update(0, 1, 1);
        const float Middle = Blend.Update(.5, 1, 1);
        const float Reversed = Blend.Update(.5, 0, .2f);
        Check(FMath::IsNearlyEqual(Middle, .5f) && Middle == Reversed && Blend.Update(.8, 0, .2f) == 0 && FPSWeaponVisualTiming::Progress(.5, 0) == 1 && FPSWeaponVisualTiming::Progress(1, 1) == 1, TEXT("duration endpoints, continuous reversal and zero equip duration"));
    });
    At(400, [&]()
    {
        bool AllPassed = !P.Failed;
        for (const auto& Pair : Progress) AllPassed &= !Pair.Value.Failed;
        Check(AllPassed, TEXT("COMPLETE"));
    });
#endif
}
