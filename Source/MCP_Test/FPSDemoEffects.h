#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FPSDemoEffects.generated.h"

/** Fixed instant GameplayEffects keep damage and healing visible in C++. */
UCLASS()
class MCP_TEST_API UFPSDemoDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UFPSDemoDamageEffect();
};

UCLASS()
class MCP_TEST_API UFPSWeaponDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UFPSWeaponDamageEffect();
};

UCLASS()
class MCP_TEST_API UFPSDemoHealEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UFPSDemoHealEffect();
};
