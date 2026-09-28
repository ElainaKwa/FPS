#include "NetLessonPlayerState.h"
#include "Net/UnrealNetwork.h"

void ANetLessonPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANetLessonPlayerState, PersonalRequestCount);
}

void ANetLessonPlayerState::RecordPersonalRequest()
{
	if (!HasAuthority()) return;
	++PersonalRequestCount;
	ForceNetUpdate();
}
