#pragma once
#include "CoreMinimal.h"
#include "FPSWeaponData.h"
#include "FPSWeaponVisualTiming.h"

/** Cosmetic offsets only. These helpers never write movement, aim or weapon state. */
namespace FPSProceduralAnimation
{
    struct FSpring
    {
        FVector Position = FVector::ZeroVector;
        FVector Velocity = FVector::ZeroVector;

        // Exact critically damped solution for a held target: stable even on a long frame.
        FVector Update(const FVector& Target, float Seconds, float Frequency = 22.f)
        {
            if (Seconds <= 0) return Position;
            const FVector Error = Position - Target;
            const FVector C = Velocity + Error * Frequency;
            const float Decay = FMath::Exp(-Frequency * Seconds);
            Position = Target + (Error + C * Seconds) * Decay;
            Velocity = (Velocity - C * (Frequency * Seconds)) * Decay;
            return Position;
        }
    };

    struct FOffset
    {
        FVector Location = FVector::ZeroVector;
        FRotator Rotation = FRotator::ZeroRotator;
    };

    struct FLocomotion
    {
        FSpring Translation, Angles, Weight, Impact;
        FVector PreviousVelocity = FVector::ZeroVector, PreviousLocation = FVector::ZeroVector;
        FRotator PreviousView = FRotator::ZeroRotator;
        float Gait = 0, Breath = 0;
        bool bInitialized = false, bWasGrounded = true, bWasCrouched = false;

        FOffset Update(float Seconds, const FVector& Location, const FVector& Velocity,
            const FRotator& View, bool bGrounded, bool bCrouched)
        {
            FOffset Result;
            if (Seconds <= 0) return Result;
            // A teleport, respawn or suspended viewport is not a foot impact or mouse flick.
            if (!bInitialized || Seconds > .25f || FVector::DistSquared(Location, PreviousLocation) > FMath::Square(450.f))
            {
                *this = FLocomotion();
                PreviousVelocity = Velocity; PreviousView = View;
                bWasGrounded = bGrounded; bWasCrouched = bCrouched; bInitialized = true;
            }
            const float Dt = FMath::Min(Seconds, .1f);
            const FRotator Yaw(0, View.Yaw, 0);
            const FVector LocalVelocity = Yaw.UnrotateVector(Velocity);
            const FVector Acceleration = Yaw.UnrotateVector((Velocity - PreviousVelocity) / Seconds).GetClampedToMaxSize(3500.f);
            const FRotator ViewDelta = (View - PreviousView).GetNormalized();
            const float PitchRate = FMath::Clamp(ViewDelta.Pitch / Seconds, -240.f, 240.f);
            const float YawRate = FMath::Clamp(ViewDelta.Yaw / Seconds, -300.f, 300.f);
            if (bGrounded && !bWasGrounded)
                Impact.Velocity.Z -= FMath::Clamp(-PreviousVelocity.Z * .075f, 0.f, 48.f);
            else if (!bGrounded && bWasGrounded && Velocity.Z > 0)
                Impact.Velocity.Z -= 12.f;
            if (bCrouched != bWasCrouched) Impact.Velocity.Z += bCrouched ? 10.f : -8.f;

            const float Speed = Velocity.Size2D();
            const float GroundWeight = Weight.Update(FVector(bGrounded ? FMath::Clamp(Speed / 480.f, 0.f, 1.35f) : 0, 0, 0), Dt, 20.f).X;
            Gait = FMath::Fmod(Gait + (bGrounded ? Speed : 0) * Dt * .021f, 2.f * PI);
            Breath = FMath::Fmod(Breath + Dt * 1.8f, 2.f * PI);
            const float CrouchScale = bCrouched ? .55f : 1.f;
            const FVector Inertia(-Acceleration.X * .0003f,
                -YawRate * .0025f - Acceleration.Y * .00025f, -PitchRate * .002f);
            const FVector AngleTarget(PitchRate * -.006f + Acceleration.X * .00025f,
                -YawRate * .008f, -LocalVelocity.Y * .0025f - YawRate * .004f);
            const FVector Sway = Translation.Update(Inertia, Dt);
            const FVector Tilt = Angles.Update(AngleTarget, Dt, 25.f);
            const float Landing = Impact.Update(FVector::ZeroVector, Dt, 24.f).Z;
            Result.Location = Sway + FVector(0, FMath::Sin(Gait) * .48f,
                FMath::Cos(Gait * 2.f) * .42f) * GroundWeight * CrouchScale;
            Result.Location.Z += Landing + FMath::Sin(Breath) * .10f * (1.f - FMath::Min(GroundWeight, 1.f));
            Result.Rotation = FRotator(Tilt.X + Landing * -.65f + FMath::Sin(Gait * 2.f) * .22f * GroundWeight,
                Tilt.Y, Tilt.Z + FMath::Sin(Gait) * .55f * GroundWeight * CrouchScale);
            PreviousLocation = Location; PreviousVelocity = Velocity; PreviousView = View;
            bWasGrounded = bGrounded; bWasCrouched = bCrouched;
            return Result;
        }
    };

    struct FActionPose : FOffset
    {
        FVector SupportHand = FVector::ZeroVector;
        FVector SupportClearance = FVector::ZeroVector;
        FRotator SupportRotation = FRotator::ZeroRotator;
        FVector MagazineOffset = FVector::ZeroVector;
        float MagazineHand = 0, CyclingHand = 0, Cycle = 0;
        bool MagazineVisible = true;
        FVector SupportGrip(const FVector& Rest, const FVector& Magazine, const FVector& Cycling, float Travel) const
        {
            // Transfer directly between contacts, without detouring through the foregrip.
            const float Sum = MagazineHand + CyclingHand;
            const float Scale = 1.f / FMath::Max(1.f, Sum);
            return Rest * (1.f - FMath::Min(1.f, Sum)) +
                (Magazine + MagazineOffset) * (MagazineHand * Scale) +
                (Cycling - FVector(Cycle * Travel, 0, 0)) * (CyclingHand * Scale) + SupportClearance;
        }
        static FActionPose Blend(const FActionPose& A, const FActionPose& B, float Alpha)
        {
            FActionPose Result;
            Result.Location = FMath::Lerp(A.Location, B.Location, Alpha);
            Result.Rotation = A.Rotation + (B.Rotation - A.Rotation) * Alpha;
            Result.SupportHand = FMath::Lerp(A.SupportHand, B.SupportHand, Alpha);
            Result.SupportClearance = FMath::Lerp(A.SupportClearance, B.SupportClearance, Alpha);
            Result.SupportRotation = A.SupportRotation + (B.SupportRotation - A.SupportRotation) * Alpha;
            Result.MagazineOffset = FMath::Lerp(A.MagazineOffset, B.MagazineOffset, Alpha);
            Result.MagazineHand = FMath::Lerp(A.MagazineHand, B.MagazineHand, Alpha);
            Result.CyclingHand = FMath::Lerp(A.CyclingHand, B.CyclingHand, Alpha);
            Result.Cycle = FMath::Lerp(A.Cycle, B.Cycle, Alpha);
            Result.MagazineVisible = Alpha > 0 ? B.MagazineVisible : A.MagazineVisible;
            return Result;
        }
    };

    struct FKey
    {
        float Time;
        FVector Location;
        FRotator Rotation;
        FVector Hand;
        FRotator HandRotation;
        FActionPose Pose() const
        {
            FActionPose Result;
            Result.Location = Location; Result.Rotation = Rotation;
            Result.SupportHand = Hand; Result.SupportRotation = HandRotation;
            return Result;
        }
    };

    template <uint32 N> FActionPose Sample(const FKey (&Keys)[N], float Phase)
    {
        for (uint32 I = 1; I < N; ++I)
            if (Phase < Keys[I].Time)
                return FActionPose::Blend(Keys[I-1].Pose(), Keys[I].Pose(),
                    FPSWeaponVisualTiming::Smooth(FMath::Clamp((Phase - Keys[I-1].Time) / (Keys[I].Time - Keys[I-1].Time), 0.f, 1.f)));
        return Keys[N-1].Pose();
    }

    inline FActionPose Action(EFPSWeaponAction Type, float Phase)
    {
        const FVector Z = FVector::ZeroVector;
        const FRotator R = FRotator::ZeroRotator;
        // Roll the receiver toward the right to expose its side; do not yaw the muzzle.
        // Hand and part paths remain in weapon space through insertion and cycling.
        static const FKey Reload[] = {
            {0, Z, R, Z, R},
            {.12f, {0,1,3}, {3,0,5}, Z, {-6,-10,-10}},
            {.28f, {-2,3,12}, {7,0,22}, {-3,-3,-6}, {4,-12,-15}},
            {.48f, {-2,3,12}, {8,0,26}, {-14,-7,-22}, {4,-12,-15}},
            {.64f, {-1,3,12}, {7,0,24}, {-1,-3,-9}, {4,-6,-8}},
            {.72f, {0,2,13.3f}, {9,0,20}, {0,-1,-6}, {-12,-18,45}},
            {.82f, {-1,2,11.5f}, {6,0,16}, {1,-1,-4}, {-8,-16,50}},
            {1, Z, R, Z, R}
        };
        // Two deliberate viewing angles with a hold, rather than a continuous sine wave.
        static const FKey Inspect[] = {
            {0, Z, R, Z, R},
            {.18f, {-1,-1,1}, {10,16,38}, Z, R},
            {.46f, {-1,-1,1}, {12,18,40}, Z, R},
            {.66f, {0,1,0}, {4,-12,-22}, Z, R},
            {.80f, {0,1,0}, {4,-12,-22}, Z, R},
            {1, Z, R, Z, R}
        };
        if (Type == EFPSWeaponAction::Reloading)
        {
            auto Pose = Sample(Reload, Phase);
            auto Ramp = [Phase](float A, float B) { return FPSWeaponVisualTiming::Smooth(FMath::Clamp((Phase-A)/(B-A), 0.f, 1.f)); };
            auto Arc = [&Ramp](float A, float B) { const float T = Ramp(A, B); return 4.f * T * (1.f-T); };
            // Release and replacement happen below the frame; insertion completes before cycling.
            Pose.MagazineOffset = FVector(-5,-6,-28) * (Ramp(.23f,.40f) - Ramp(.48f,.63f));
            Pose.MagazineHand = Ramp(.08f,.20f) * (1.f-Ramp(.66f,.76f));
            Pose.CyclingHand = Ramp(.66f,.76f) * (1.f-Ramp(.90f,.99f));
            // Reach around the magazine, clear the receiver, then return around its left side.
            // Offsets vanish while gripping a moving part so the hand cannot slide off it.
            Pose.SupportClearance = FVector(-1,-3,-2) * Arc(.08f,.20f) +
                FVector(0,-6,18) * Arc(.66f,.76f) + FVector(0,-7,10) * Arc(.90f,.99f);
            Pose.Cycle = Ramp(.76f,.82f) * (1.f-Ramp(.86f,.90f));
            Pose.MagazineVisible = Phase < .40f || Phase >= .48f;
            return Pose;
        }
        if (Type == EFPSWeaponAction::Inspecting) return Sample(Inspect, Phase);
        return {};
    }

    struct FActionBlend
    {
        FActionPose Current, Correction;
        EFPSWeaponAction Previous = EFPSWeaponAction::Idle;
        double PreviousStart = 0, ChangedAt = -1;
        bool bInitialized = false;

        FActionPose Update(EFPSWeaponAction Type, double StartedAt, double EndsAt, double Now)
        {
            const FActionPose Target = Action(Type, FPSWeaponVisualTiming::Progress(Now - StartedAt, EndsAt - StartedAt));
            if (!bInitialized) { bInitialized = true; Previous = Type; PreviousStart = StartedAt; }
            else if (Type != Previous || (Type != EFPSWeaponAction::Idle && StartedAt != PreviousStart))
            {
                Correction = Current;
                Correction.Location -= Target.Location; Correction.Rotation -= Target.Rotation;
                Correction.SupportHand -= Target.SupportHand; Correction.SupportRotation -= Target.SupportRotation;
                Correction.SupportClearance -= Target.SupportClearance;
                Correction.MagazineOffset -= Target.MagazineOffset;
                Correction.MagazineHand -= Target.MagazineHand; Correction.CyclingHand -= Target.CyclingHand;
                Correction.Cycle -= Target.Cycle;
                ChangedAt = Now; Previous = Type; PreviousStart = StartedAt;
            }
            const float Residual = 1.f - FPSWeaponVisualTiming::Smooth(FPSWeaponVisualTiming::Progress(Now - ChangedAt, .12f));
            Current = Target;
            Current.Location += Correction.Location * Residual; Current.Rotation += Correction.Rotation * Residual;
            Current.SupportHand += Correction.SupportHand * Residual; Current.SupportRotation += Correction.SupportRotation * Residual;
            Current.SupportClearance += Correction.SupportClearance * Residual;
            Current.MagazineOffset += Correction.MagazineOffset * Residual;
            Current.MagazineHand += Correction.MagazineHand * Residual; Current.CyclingHand += Correction.CyclingHand * Residual;
            Current.Cycle += Correction.Cycle * Residual;
            // A corrected server deadline can move across a short pull/release segment.
            // Keep grip weights and mechanical travel physical during that correction.
            Current.MagazineHand = FMath::Clamp(Current.MagazineHand, 0.f, 1.f);
            Current.CyclingHand = FMath::Clamp(Current.CyclingHand, 0.f, 1.f);
            Current.Cycle = FMath::Clamp(Current.Cycle, 0.f, 1.f);
            Current.MagazineOffset.X = FMath::Clamp(Current.MagazineOffset.X, -5., 0.);
            Current.MagazineOffset.Y = FMath::Clamp(Current.MagazineOffset.Y, -6., 0.);
            Current.MagazineOffset.Z = FMath::Clamp(Current.MagazineOffset.Z, -28., 0.);
            return Current;
        }
    };
}
