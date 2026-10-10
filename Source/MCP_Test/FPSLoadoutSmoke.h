#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FPSLoadoutSmoke.generated.h"
class AFPSMatchMode;
class AFPSFrontController;
struct FFPSLoadoutSmoke
{
    static void Start(AFPSMatchMode* Mode);
    static void Tick(AFPSFrontController* Controller);
};
UCLASS()
class UFPSHostilePeriodicSmokeEffect : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UFPSHostilePeriodicSmokeEffect();
};
