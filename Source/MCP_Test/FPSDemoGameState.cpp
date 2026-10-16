#include "FPSDemoGameState.h"
#include "Net/UnrealNetwork.h"

void AFPSDemoGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFPSDemoGameState, bStructuredMatch);
	DOREPLIFETIME(AFPSDemoGameState, bTraining);
    DOREPLIFETIME(AFPSDemoGameState, bBaseRegionsEnabled);
	DOREPLIFETIME(AFPSDemoGameState, bWaiting);
	DOREPLIFETIME(AFPSDemoGameState, bFinished);
	DOREPLIFETIME(AFPSDemoGameState, RoundEndsAt);
	DOREPLIFETIME(AFPSDemoGameState, RoomName);
	DOREPLIFETIME(AFPSDemoGameState, WeaponDataError);
    DOREPLIFETIME(AFPSDemoGameState, DropNoticeUntil);
    DOREPLIFETIME(AFPSDemoGameState, HostPlayerId);
	DOREPLIFETIME(AFPSDemoGameState, Podium);
	DOREPLIFETIME(AFPSDemoGameState, Eliminations);
	DOREPLIFETIME(AFPSDemoGameState, LastEvent);
}

void AFPSDemoGameState::RecordElimination(const FString& AttackerName, const FString& VictimName)
{
	if (!HasAuthority()) return;
	++Eliminations;
	LastEvent = FString::Printf(TEXT("%s eliminated %s"), *AttackerName, *VictimName);
	ForceNetUpdate();
}

bool AFPSDemoGameState::IsCombatActive() const
{
    return bStructuredMatch && WeaponDataError.IsEmpty() && !bWaiting && !bFinished &&
        (bTraining || RoundEndsAt <= 0 || GetServerWorldTimeSeconds() < RoundEndsAt);
}
