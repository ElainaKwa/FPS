#include "NetLessonGameState.h"
#include "Net/UnrealNetwork.h"

void ANetLessonGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANetLessonGameState, SharedRequestCount);
}

void ANetLessonGameState::RecordSharedRequest()
{
	if (!HasAuthority()) return;
	++SharedRequestCount;
	ForceNetUpdate();
}
