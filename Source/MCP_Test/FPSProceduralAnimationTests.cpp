#if WITH_DEV_AUTOMATION_TESTS
#include "FPSProceduralAnimation.h"
#include "Misc/AutomationTest.h"
#include "FPSWeaponReloadRig.h"
#include "Engine/StaticMesh.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPSProceduralAnimationTest, "FPS.Animation.MotionAndActions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFPSProceduralAnimationTest::RunTest(const FString& Parameters)
{
    using namespace FPSProceduralAnimation;
    FSpring Slow, Fast;
    Slow.Velocity = Fast.Velocity = FVector(0, 0, -40);
    for (int I = 0; I < 15; ++I) Slow.Update(FVector(1,-2,0), 1.f/30);
    for (int I = 0; I < 60; ++I) Fast.Update(FVector(1,-2,0), 1.f/120);
    TestTrue(TEXT("30/120 FPS identical spring response"), Slow.Position.Equals(Fast.Position, .0001f));
    Slow.Update(FVector::ZeroVector, 5.f);
    TestTrue(TEXT("Long frame remains finite and settles"), Slow.Position.IsNearlyZero() && !Slow.Position.ContainsNaN());

    FLocomotion Motion;
    const FVector Z = FVector::ZeroVector;
    Motion.Update(1.f/60, Z, Z, FRotator(0,179,0), true, false);
    const auto Wrapped = Motion.Update(1.f/60, Z, Z, FRotator(0,-179,0), true, false);
    TestTrue(TEXT("Yaw wraps without a full-turn impulse"), FMath::Abs(Wrapped.Rotation.Yaw) < .2f);
    for (int I = 0; I < 30; ++I) Motion.Update(1.f/60, Z, FVector(480,0,0), FRotator::ZeroRotator, true, false);
    const float WalkPhase = Motion.Gait;
    for (int I = 0; I < 90; ++I) Motion.Update(1.f/60, Z, Z, FRotator::ZeroRotator, true, false);
    TestEqual(TEXT("Stopped feet do not keep cycling"), Motion.Gait, WalkPhase);
    TestTrue(TEXT("Stop inertia and gait amplitude recover"), Motion.Weight.Position.Size() < .001f && Motion.Translation.Position.Size() < .001f);
    Motion.Update(1.f/60, Z, FVector(0,0,-500), FRotator::ZeroRotator, false, false);
    const auto Land = Motion.Update(1.f/60, Z, Z, FRotator::ZeroRotator, true, false);
    TestTrue(TEXT("Landing creates a bounded downward impulse"), Land.Location.Z < -.1f && Land.Location.Z > -2.f);
    const auto Teleport = Motion.Update(1.f/60, FVector(2000,0,0), Z, FRotator::ZeroRotator, true, false);
    TestTrue(TEXT("Teleport resets stale impact"), FMath::Abs(Teleport.Location.Z) < .02f);

    for (const auto Type : {EFPSWeaponAction::Reloading, EFPSWeaponAction::Inspecting})
    {
        TestTrue(TEXT("Action starts neutral"), Action(Type, 0).Rotation.IsZero() && Action(Type, 0).SupportHand.IsZero());
        TestTrue(TEXT("Action ends at exact neutral"), Action(Type, 1).Location.IsZero() && Action(Type, 1).Rotation.IsZero() && Action(Type, 1).SupportHand.IsZero());
        FActionPose Previous = Action(Type, 0);
        for (int I = 1; I <= 1000; ++I)
        {
            const auto Pose = Action(Type, I / 1000.f);
            if ((Pose.Location - Previous.Location).Size() > .3f || (Pose.SupportHand - Previous.SupportHand).Size() > .5f ||
                (Pose.SupportClearance - Previous.SupportClearance).Size() > 1.f ||
                (Pose.Rotation - Previous.Rotation).GetManhattanDistance(FRotator::ZeroRotator) > 2.f)
                AddError(TEXT("Discontinuity inside staged action"));
            Previous = Pose;
        }
    }
    FActionBlend ActionState;
    const auto Mid = ActionState.Update(EFPSWeaponAction::Reloading, 10, 12, 11);
    TestTrue(TEXT("Late join restores the current action phase immediately"), Mid.Location.Equals(Action(EFPSWeaponAction::Reloading, .5f).Location));
    const auto Cancel = ActionState.Update(EFPSWeaponAction::Idle, 0, 0, 11);
    TestTrue(TEXT("Cancellation preserves first-frame pose and hand continuity"), Cancel.Location.Equals(Mid.Location) && Cancel.SupportHand.Equals(Mid.SupportHand));
    const auto Settled = ActionState.Update(EFPSWeaponAction::Idle, 0, 0, 11.13);
    TestTrue(TEXT("Cancellation reaches exact neutral within 120 ms"), Settled.Location.IsZero() && Settled.Rotation.IsZero() && Settled.SupportHand.IsZero());
    const auto Instant = ActionState.Update(EFPSWeaponAction::Reloading, 20, 20, 20);
    TestTrue(TEXT("Zero duration stays finite and neutral"), Instant.Location.IsZero() && !Instant.Rotation.ContainsNaN());
    const auto Removed = Action(EFPSWeaponAction::Reloading, .39f);
    TestTrue(TEXT("Magazine is extracted before replacement"), Removed.MagazineOffset.Z < -27 && Removed.MagazineHand == 1 && Removed.Cycle == 0);
    TestFalse(TEXT("Old magazine disappears below frame"), Action(EFPSWeaponAction::Reloading, .44f).MagazineVisible);
    const auto Seated = Action(EFPSWeaponAction::Reloading, .65f);
    TestTrue(TEXT("New magazine is fully seated before cycling"), Seated.MagazineOffset.IsZero() && Seated.MagazineVisible && Seated.Cycle == 0);
    const auto Pulled = Action(EFPSWeaponAction::Reloading, .825f);
    TestTrue(TEXT("Hand holds fully pulled charging handle or slide"), Pulled.Cycle == 1 && Pulled.CyclingHand == 1 && Pulled.MagazineOffset.IsZero());
    const FVector Rest(25,-2,-5), Magazine(14,-2.8,-13), Cycling(7.7,4.45,2.4);
    TestTrue(TEXT("Extracting hand stays attached to the magazine"), Removed.SupportGrip(Rest, Magazine, Cycling, 4.5f).Equals(Magazine + Removed.MagazineOffset));
    TestTrue(TEXT("Cycling hand stays attached to the pulled handle"), Pulled.SupportGrip(Rest, Magazine, Cycling, 4.5f).Equals(Cycling - FVector(4.5,0,0)));
    const auto Transfer = Action(EFPSWeaponAction::Reloading, .71f);
    TestTrue(TEXT("Magazine-to-handle transfer does not detour through the foregrip"),
        Transfer.SupportGrip(Rest, Magazine, Cycling, 4.5f).Equals(Transfer.SupportGrip(Rest + FVector(100,100,100), Magazine, Cycling, 4.5f)));
    TestTrue(TEXT("Transfer clears the receiver from above"), Transfer.SupportGrip(Rest, Magazine, Cycling, 4.5f).Z > 12.f);
    TestEqual(TEXT("Cycling part releases before hand returns"), Action(EFPSWeaponAction::Reloading, .9f).Cycle, 0.f);
    for (float Phase : {.15f,.35f,.44f,.6f,.71f,.825f,.95f})
    {
        FActionBlend Cancelled;
        const auto Before = Cancelled.Update(EFPSWeaponAction::Reloading, 10, 12, 10+Phase*2);
        const auto First = Cancelled.Update(EFPSWeaponAction::Idle, 0, 0, 10+Phase*2);
        TestTrue(TEXT("Cancellation keeps part transform continuous"), First.MagazineOffset.Equals(Before.MagazineOffset) && First.Cycle == Before.Cycle);
        TestTrue(TEXT("Cancellation preserves the hand's contact or clearance arc"), First.SupportGrip(Rest, Magazine, Cycling, 4.5f).Equals(Before.SupportGrip(Rest, Magazine, Cycling, 4.5f)));
        const auto End = Cancelled.Update(EFPSWeaponAction::Idle, 0, 0, 10+Phase*2+.13);
        TestTrue(TEXT("Cancelled assembly is complete after recovery"), End.MagazineOffset.IsZero() && End.Cycle == 0 && End.MagazineVisible && End.MagazineHand == 0 && End.CyclingHand == 0 && End.SupportClearance.IsZero());
    }
    FActionBlend Retimed;
    Retimed.Update(EFPSWeaponAction::Reloading, 10, 11, 10.75);
    Retimed.Update(EFPSWeaponAction::Reloading, 9.925, 10.925, 10.75);
    for (int I=1; I<=15; ++I)
    {
        const auto Pose = Retimed.Update(EFPSWeaponAction::Reloading, 9.925, 10.925, 10.75+I*.01);
        TestTrue(TEXT("Authority timing correction cannot push bolt forward or extrapolate grips"),
            Pose.Cycle >= 0 && Pose.Cycle <= 1 && Pose.CyclingHand >= 0 && Pose.CyclingHand <= 1 && Pose.MagazineHand >= 0 && Pose.MagazineHand <= 1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPSReloadAssetsTest, "FPS.Animation.ReloadAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFPSReloadAssetsTest::RunTest(const FString& Parameters)
{
    for (const TCHAR* Name : {TEXT("M4"),TEXT("AK"),TEXT("BR3"),TEXT("MP5"),TEXT("UMP45"),TEXT("M9"),TEXT("HeavyPistol")})
    {
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_%s.SM_IND_%s"),Name,Name));
        const auto* Rig = UFPSWeaponReloadRig::Find(Mesh);
        if (!TestNotNull(*FString::Printf(TEXT("%s has saved reload assembly"),Name), Rig)) continue;
        TestTrue(TEXT("Assembly parts are independent and finite"), Rig->Body != Mesh && Rig->Magazine != Rig->Body && Rig->CyclingPart != Rig->Body &&
            !Rig->MagazineGrip.ContainsNaN() && !Rig->CyclingGrip.ContainsNaN() && Rig->CyclingTravel > 0);
        for (auto* Part : {Rig->Body.Get(),Rig->Magazine.Get(),Rig->CyclingPart.Get()})
            TestTrue(TEXT("Imported part has render geometry and materials"), Part->GetNumVertices(0) > 0 && Part->GetStaticMaterials().Num() > 0);
    }
    return true;
}
#endif
