#include "FPSDemoPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "FPSScoreSet.h"
#include "FPSModeRules.h"

AFPSDemoPlayerState::AFPSDemoPlayerState()
{
	ScoreAbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ScoreAbilitySystem"));
	ScoreAbilitySystem->SetIsReplicated(true);
	ScoreAbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	ScoreAttributes = CreateDefaultSubobject<UFPSScoreSet>(TEXT("ScoreAttributes"));
	SetNetUpdateFrequency(10);
}

void AFPSDemoPlayerState::BeginPlay()
{
	Super::BeginPlay();
	ScoreAbilitySystem->InitAbilityActorInfo(this, this);
}

UAbilitySystemComponent* AFPSDemoPlayerState::GetAbilitySystemComponent() const { return ScoreAbilitySystem; }
int32 AFPSDemoPlayerState::GetEliminationCount() const { return FMath::RoundToInt(ScoreAttributes->GetPoints()); }

void AFPSDemoPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFPSDemoPlayerState, TeamId);
	DOREPLIFETIME(AFPSDemoPlayerState, DeathCount);
	DOREPLIFETIME_CONDITION(AFPSDemoPlayerState, SelectedPrimary, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AFPSDemoPlayerState, SelectedSecondary, COND_OwnerOnly);
}

void AFPSDemoPlayerState::AddElimination()
{
	if (!HasAuthority()) return;
	const auto Spec = ScoreAbilitySystem->MakeOutgoingSpec(UFPSKillScoreEffect::StaticClass(), 1, ScoreAbilitySystem->MakeEffectContext());
	if (Spec.IsValid()) ScoreAbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	ForceNetUpdate();
}

void AFPSDemoPlayerState::AddDeath()
{
	if (HasAuthority()) { ++DeathCount; ForceNetUpdate(); }
}

void AFPSDemoPlayerState::EnsureLoadout(const UFPSModeRules* Rules)
{
    if (!HasAuthority() || bLoadoutInitialized || !Rules || !Rules->DefaultLoadout) return;
    const auto* Primary = Rules->DefaultLoadout->Primary.LoadSynchronous();
    const auto* Secondary = Rules->DefaultLoadout->Secondary.LoadSynchronous();
    if (!Primary || !Secondary) return;
    SelectedPrimary = Primary->WeaponId; SelectedSecondary = Secondary->WeaponId;
    bLoadoutInitialized = true; ForceNetUpdate();
}
