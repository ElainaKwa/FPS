#pragma once
#include "CoreMinimal.h"

namespace FPSWeaponVisualTiming
{
    // Durations are seconds to the endpoint, including zero = immediate.
    inline float Progress(double Age, float Duration)
    {
        return Duration <= 0 ? 1.f : FMath::Clamp(float(Age / Duration), 0.f, 1.f);
    }
    inline float Smooth(float Alpha) { return Alpha * Alpha * (3.f - 2.f * Alpha); }
    struct FTransition
    {
        float From = 0, Target = 0, Duration = 0;
        double StartedAt = 0;
        explicit FTransition(float Initial = 0) : From(Initial), Target(Initial) {}
        float Value(double Now) const { return FMath::Lerp(From, Target, Smooth(Progress(Now - StartedAt, Duration))); }
        float Update(double Now, float NewTarget, float Seconds)
        {
            if (NewTarget != Target)
            {
                From = Value(Now); Target = NewTarget; StartedAt = Now; Duration = Seconds;
            }
            if (Seconds <= 0) { From = Target; Duration = 0; }
            return Value(Now);
        }
    };
}
