#include "FPSAnimationCapture.h"
#include "FPSCharacterPresentation.h"
#include "FPSDemoCharacter.h"
#include "FPSMovementComponent.h"
#include "FPSSystemSettings.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponReloadRig.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"

void TickFPSAnimationCapture(AFPSDemoCharacter* C, UFPSCharacterPresentation* Presentation,
    USceneComponent* Gun, UCameraComponent* Camera)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSAnimationCapture")) || !C->IsLocallyControlled()) return;
    auto* PC = Cast<APlayerController>(C->GetController());
    auto* W = C->GetWeapons();
    auto* Move = Cast<UFPSMovementComponent>(C->GetCharacterMovement());
    if (!PC || !Move || !W->Definition()) return;
    struct FRun
    {
        double Epoch = -1, NextFrame = 0;
        int32 Frames = 0;
        float SavedBob = 1;
        float SavedAimDuration = 0;
        FRotator View = FRotator::ZeroRotator;
        TSet<int32> Done;
        bool Failed = false, Walk = false, Sprint = false, Air = false, Reload = false, Inspect = false;
        bool RifleMag = false, RifleCycle = false, PistolMag = false, PistolCycle = false, WorldMag = false, WorldCycle = false;
        bool RifleHandMag = false, RifleHandCycle = false, PistolHandMag = false, PistolHandCycle = false;
        double HandContactError = 0, RightGripError = 0;
        TWeakObjectPtr<ACameraActor> Observer;
    };
    static TMap<TWeakObjectPtr<AFPSDemoCharacter>, FRun> Runs;
    auto& Run = Runs.FindOrAdd(C);
    const double Now = C->GetWorld()->GetTimeSeconds();
    auto* Settings = UFPSSystemSettings::Get(C);
    if (Run.Epoch < 0)
    {
        Run.Epoch = Now + 3; Run.View = PC->GetControlRotation();
        if (Settings) { Run.SavedBob = Settings->BobScale; Settings->BobScale = 1; }
    }
    const double T = Now - Run.Epoch;
    if (T < 0) return;
    auto At = [&](int32 Deciseconds, TFunctionRef<void()> Work)
    {
        if (T >= Deciseconds * .1 && !Run.Done.Contains(Deciseconds)) { Run.Done.Add(Deciseconds); Work(); }
    };
    auto Check = [&](bool Pass, const TCHAR* Name)
    {
        Run.Failed |= !Pass;
        UE_LOG(LogTemp, Display, TEXT("FPS_ANIMATION_CAPTURE %s %s"), Pass ? TEXT("PASS") : TEXT("FAIL"), Name);
    };
    if (T > 1 && T < 2) C->AddMovementInput(Run.View.Vector());
    if (T > 2 && T < 3) C->AddMovementInput(FRotationMatrix(Run.View).GetUnitAxis(EAxis::Y));
    if (T > 3 && T < 4) PC->SetControlRotation(Run.View + FRotator(0, FMath::Sin((T-3) * 2*PI) * 20, 0));
    At(40, [&] { PC->SetControlRotation(Run.View); });
    At(50, [&] { Move->RequestSprint(true); });
    if (T > 5 && T < 6) C->AddMovementInput(Run.View.Vector());
    At(60, [&] { Move->RequestSprint(false); });
    At(65, [&] { Move->RequestJump(); });
    At(80, [&] { W->AimOn(); });
    At(87, [&] {
        Check(W->GetLoadout().bAiming && Presentation->GetAmbientOffset().IsZero(), TEXT("ADS removes ambient offsets"));
        Check(Gun->GetRelativeLocation().Equals(W->EffectiveStats().AimLocation, .01f), TEXT("ADS reaches authored sight alignment"));
        Check(Camera->GetRelativeLocation().Equals(FVector(0,0,C->BaseEyeHeight), .01f), TEXT("cosmetics preserve camera origin"));
    });
    At(90, [&] { W->AimOff(); });
    At(95, [&] { W->StartFire(); });
    At(100, [&] { W->StopFire(); });
    At(102, [&] { W->Reload(); });
    At(130, [&] { W->Inspect(); });
    At(141, [&] { W->Secondary(); });
    At(150, [&] { W->Primary(); });
    At(158, [&] { Move->RequestCrouch(true); });
    At(168, [&] { Move->RequestCrouch(false); });
    At(169, [&] { W->Inspect(); });
    At(171, [&] {
        auto* Def = const_cast<UFPSWeaponDefinition*>(W->Definition());
        Run.SavedAimDuration = Def->AimTransitionTime; Def->AimTransitionTime = 0;
        W->AimOn();
    });
    At(172, [&] {
        Check(W->GetLoadout().bAiming && Gun->GetRelativeLocation().Equals(W->EffectiveStats().AimLocation, .01f),
            TEXT("instant ADS during inspect cancellation preserves sight alignment"));
        W->AimOff(); const_cast<UFPSWeaponDefinition*>(W->Definition())->AimTransitionTime = Run.SavedAimDuration;
    });
    At(175, [&] { if (Settings) Settings->BobScale = 0; });
    if (T > 17.5 && T < 18.3) PC->SetControlRotation(Run.View + FRotator(0, (T - 17.5) * 40, 0));
    At(183, [&] { Check(Presentation->GetAmbientOffset().IsZero(), TEXT("decorative motion option suppresses inertia and breathing")); });
    At(185, [&] { if (Settings) Settings->BobScale = 1; PC->SetControlRotation(Run.View); });
    At(190, [&] {
        Run.Observer = C->GetWorld()->SpawnActor<ACameraActor>();
        if (Run.Observer.IsValid())
        {
            const FVector Position = C->GetActorLocation() + Run.View.RotateVector(FVector(200,160,35));
            Run.Observer->SetActorLocation(Position);
            Run.Observer->SetActorRotation((C->GetActorLocation() - Position).Rotation());
            PC->SetViewTarget(Run.Observer.Get());
        }
        W->Inspect();
    });
    At(205, [&] { Move->RequestCrouch(true); });
    At(215, [&] { Move->RequestCrouch(false); });
    At(230, [&] { W->StartFire(); });
    At(232, [&] { W->StopFire(); });
    At(235, [&] { W->Reload(); });
    At(270, [&] { PC->SetViewTarget(C); W->Secondary(); });
    At(280, [&] { W->StartFire(); });
    At(282, [&] { W->StopFire(); });
    At(290, [&] { W->Reload(); });
    if (const auto* Rig = UFPSWeaponReloadRig::Find(W->Definition()->FirstPerson.Mesh.Get()))
    {
        const bool Pistol = W->Definition()->Category == EFPSWeaponCategory::Pistol;
        TInlineComponentArray<UStaticMeshComponent*> Components(C);
        UStaticMeshComponent *FPLeft = nullptr, *FPRight = nullptr, *WorldLeft = nullptr;
        UStaticMeshComponent *FPMag = nullptr, *FPCycle = nullptr, *TPMag = nullptr, *TPCycle = nullptr;
        const UStaticMeshComponent *FPWeapon = nullptr, *TPWeapon = nullptr;
        for (const auto* Part : Components)
        {
            if (Part->GetFName() == TEXT("FirstPersonRifle")) FPWeapon = Part;
            if (Part->GetFName() == TEXT("WorldRifle")) TPWeapon = Part;
            const bool Mag = Part->GetStaticMesh() == Rig->Magazine && Part->GetRelativeLocation().Z < -15;
            const bool Cycle = Part->GetStaticMesh() == Rig->CyclingPart && Part->GetRelativeLocation().X < -1.5;
            if (Part->bOnlyOwnerSee)
            {
                if (Pistol) { Run.PistolMag |= Mag; Run.PistolCycle |= Cycle; }
                else { Run.RifleMag |= Mag; Run.RifleCycle |= Cycle; }
            }
            else if (T > 23) { Run.WorldMag |= Mag; Run.WorldCycle |= Cycle; }
        }
        for (auto* Part : Components)
        {
            if (Part->GetFName() == TEXT("Avatar_FPLGlove")) FPLeft = Part;
            if (Part->GetFName() == TEXT("Avatar_FPRGlove")) FPRight = Part;
            if (Part->GetFName() == TEXT("Avatar_LGlove")) WorldLeft = Part;
            if (FPWeapon && Part->GetAttachParent() == FPWeapon)
            {
                if (Part->GetStaticMesh() == Rig->Magazine) FPMag = Part;
                if (Part->GetStaticMesh() == Rig->CyclingPart) FPCycle = Part;
            }
            if (TPWeapon && Part->GetAttachParent() == TPWeapon)
            {
                if (Part->GetStaticMesh() == Rig->Magazine) TPMag = Part;
                if (Part->GetStaticMesh() == Rig->CyclingPart) TPCycle = Part;
            }
        }
        const auto& State = W->GetLoadout();
        if (State.Action == EFPSWeaponAction::Reloading)
        {
            const float Phase = FPSWeaponVisualTiming::Progress(W->ServerTime() - State.ActionStartedAt, State.ActionEndsAt - State.ActionStartedAt);
            Run.RightGripError = FMath::Max(Run.RightGripError, FPRight && FPWeapon ? FVector::Dist(
                FPWeapon->GetComponentTransform().InverseTransformPosition(FPRight->GetComponentLocation()), FVector(-2,0,-9)) : 1000.);
            const bool HoldingMag = (Phase > .24f && Phase < .38f) || (Phase > .51f && Phase < .63f);
            const bool HoldingCycle = Phase > .78f && Phase < .88f;
            if (HoldingMag || HoldingCycle)
            {
                const auto* FPPart = HoldingMag ? FPMag : FPCycle;
                const auto* TPPart = HoldingMag ? TPMag : TPCycle;
                const FVector Contact = HoldingMag ? Rig->MagazineGrip : Rig->CyclingGrip;
                if (FPLeft && WorldLeft && FPPart && TPPart)
                {
                    Run.HandContactError = FMath::Max(Run.HandContactError, FVector::Dist(FPLeft->GetComponentLocation(), FPPart->GetComponentTransform().TransformPosition(Contact)));
                    Run.HandContactError = FMath::Max(Run.HandContactError, FVector::Dist(WorldLeft->GetComponentLocation(), TPPart->GetComponentTransform().TransformPosition(Contact)));
                }
                else Run.HandContactError = 1000;
                if (Pistol) { Run.PistolHandMag |= HoldingMag; Run.PistolHandCycle |= HoldingCycle; }
                else { Run.RifleHandMag |= HoldingMag; Run.RifleHandCycle |= HoldingCycle; }
            }
        }
    }
    Run.Walk |= C->GetVelocity().Size2D() > 100;
    Run.Sprint |= Move->IsSprinting(); Run.Air |= Move->IsFalling();
    Run.Reload |= W->GetLoadout().Action == EFPSWeaponAction::Reloading;
    Run.Inspect |= W->GetLoadout().Action == EFPSWeaponAction::Inspecting;
    if (T >= Run.NextFrame && T < 34)
    {
        FString Dir; FParse::Value(FCommandLine::Get(), TEXT("FPSAnimationOutput="), Dir);
        if (Dir.IsEmpty()) Dir = FPaths::ProjectSavedDir() / TEXT("FPSAnimationTests/Capture");
        IFileManager::Get().MakeDirectory(*Dir, true);
        FScreenshotRequest::RequestScreenshot(Dir / FString::Printf(TEXT("Frame%04d.png"), Run.Frames), true, false);
        UE_LOG(LogTemp, Display, TEXT("FPS_ANIMATION_FRAME frame=%d time=%.3f speed=%.1f action=%d gun=%s"),
            Run.Frames++, T, C->GetVelocity().Size2D(), int32(W->GetLoadout().Action), *Gun->GetRelativeLocation().ToString());
        Run.NextFrame = T + 1.0 / 12;
    }
    At(342, [&] {
        Check(Run.Walk && Run.Sprint && Run.Air, TEXT("native movement covers walk sprint jump landing"));
        Check(Run.Reload && Run.Inspect, TEXT("real weapon abilities enter reload and inspect"));
        Check(Run.RifleMag && Run.RifleCycle, TEXT("first-person rifle removes magazine and pulls charging handle"));
        Check(Run.PistolMag, TEXT("first-person pistol removes magazine"));
        Check(Run.PistolCycle, TEXT("first-person pistol racks slide"));
        Check(Run.WorldMag && Run.WorldCycle, TEXT("third-person reload animates independent parts"));
        Check(Run.RifleHandMag && Run.RifleHandCycle && Run.PistolHandMag && Run.PistolHandCycle && Run.HandContactError < .1,
            TEXT("first and third person hands stay attached to rifle and pistol magazines and cycling parts"));
        Check(Run.RightGripError < .1, TEXT("right hand remains fixed to the pistol grip during reload"));
        Check(!Gun->GetRelativeLocation().ContainsNaN() && !Gun->GetRelativeRotation().ContainsNaN(), TEXT("rendered transforms remain finite"));
        if (Settings) Settings->BobScale = Run.SavedBob;
        PC->SetViewTarget(C);
        if (Run.Observer.IsValid()) Run.Observer->Destroy();
        Check(!Run.Failed, TEXT("COMPLETE"));
        FPlatformMisc::RequestExit(false);
    });
#endif
}
