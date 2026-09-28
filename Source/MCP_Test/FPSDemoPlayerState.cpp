#include "FPSDemoPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "FPSScoreSet.h"

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
