// Compatibility-only fixed shot/heal RPCs for LV_NetSyncFPS. Structured weapons remain GAS Abilities.
#include "FPSDemoCharacter.h"
#include "FPSWeaponComponent.h"
#include "FPSDemoEffects.h"
#include "FPSDemoHealStation.h"
#include "FPSMatchFlow.h"
#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
void AFPSDemoCharacter::ServerFire_Implementation()
{
	if (Weapons->IsEnabled()) return; // Structured matches use GAS only; this RPC belongs to the teaching map.
	if (AFPSMatchMode* Rules = GetWorld()->GetAuthGameMode<AFPSMatchMode>())
		if (!Rules->IsCombatActive()) return;
	const float Now = GetWorld()->GetTimeSeconds();
	if (!HasAuthority() || bEliminated || Now - LastServerFire < 0.27f || !Controller) return;
	LastServerFire = Now;
	const auto* Def = Weapons->Catalog()->Find(Weapons->Catalog()->StartingPrimary);
	ResolveShot(6000.f, Def ? Def->FireSound.Get() : nullptr, nullptr);
}

void AFPSDemoCharacter::ApplyTeachingDamageEffectTo(AFPSDemoCharacter* Target)
{
	if (!HasAuthority() || !Target || Target->IsEliminated() || !Target->AbilitySystem) return;
	if (const auto* Rules = GetWorld()->GetAuthGameMode<AFPSMatchMode>())
		if (!Rules->CanDamage(this, Target)) return;
	const auto Spec = AbilitySystem->MakeOutgoingSpec(UFPSDemoDamageEffect::StaticClass(), 1.0f, AbilitySystem->MakeEffectContext());
	if (Spec.IsValid()) Target->AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void AFPSDemoCharacter::ServerRequestHeal_Implementation()
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (!HasAuthority() || bEliminated || Now - LastServerHeal < 3.0f) return;
	bool bNearStation = false;
	for (TActorIterator<AFPSDemoHealStation> It(GetWorld()); It; ++It)
	{
		if (It->IsWithinUseRange(GetActorLocation())) { bNearStation = true; break; }
	}
	if (!bNearStation) return;
	LastServerHeal = Now;
	FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(UFPSDemoHealEffect::StaticClass(), 1.0f, AbilitySystem->MakeEffectContext());
	if (Spec.IsValid()) AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

