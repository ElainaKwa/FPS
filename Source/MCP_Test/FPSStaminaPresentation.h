#pragma once
#include "CoreMinimal.h"

/** Local visual state only; never feeds a displayed value back into gameplay. */
struct FFPSStaminaPresentation
{
    float Fast = 1.f, Delayed = 1.f, Previous = 1.f, Hold = 0.f;
    int32 Direction = 0;
    bool Initialized = false;

    void Reset(float Ratio)
    {
        Fast = Delayed = Previous = FMath::Clamp(Ratio, 0.f, 1.f);
        Hold = 0.f; Direction = 0; Initialized = true;
    }
    void Update(float Ratio, float DeltaTime)
    {
        Ratio = FMath::Clamp(Ratio, 0.f, 1.f);
        if (!Initialized) { Reset(Ratio); return; }
        const float Delta = Ratio - Previous;
        const int32 NewDirection = Delta > KINDA_SMALL_NUMBER ? 1 : Delta < -KINDA_SMALL_NUMBER ? -1 : 0;
        // Continuous drain/recovery must not restart the delay every frame.
        if (NewDirection && NewDirection != Direction) { Direction = NewDirection; Hold = .16f; }
        const float Dt = FMath::Max(DeltaTime, 0.f);
        const float FollowTime = FMath::Max(0.f, Dt - Hold);
        Hold = FMath::Max(0.f, Hold - Dt);
        Fast = FMath::Lerp(Fast, Ratio, 1.f - FMath::Exp(-Dt / .055f));
        Delayed = FMath::Lerp(Delayed, Ratio, 1.f - FMath::Exp(-FollowTime / .20f));
        if (FMath::Abs(Fast - Ratio) < .001f) Fast = Ratio;
        if (Hold == 0.f && FMath::Abs(Delayed - Ratio) < .001f) { Delayed = Ratio; Direction = 0; }
        Previous = Ratio;
    }
    float White() const { return FMath::Min(Fast, Delayed); }
    float Ghost() const { return FMath::Max(Fast, Delayed); }
    bool IsSettled() const { return Fast == Previous && Delayed == Previous; }
};
