#include "NetLessonPlayerController.h"

void ANetLessonPlayerController::AcknowledgeCounterRequest(int32 NewValue)
{
	if (HasAuthority()) ClientCounterAcknowledged(NewValue);
}

void ANetLessonPlayerController::ClientCounterAcknowledged_Implementation(int32 NewValue)
{
	LastAckValue = NewValue;
	AckUntil = GetWorld()->GetTimeSeconds() + 3.0f;
}
