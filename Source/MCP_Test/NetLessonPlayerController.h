#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NetLessonPlayerController.generated.h"

/** Exists on the server and its owning client, never on another player's client. */
UCLASS()
class MCP_TEST_API ANetLessonPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void AcknowledgeCounterRequest(int32 NewValue);
	float GetAckUntil() const { return AckUntil; }
	int32 GetLastAckValue() const { return LastAckValue; }

private:
	UFUNCTION(Client, Reliable)
	void ClientCounterAcknowledged(int32 NewValue);

	float AckUntil = 0.0f;
	int32 LastAckValue = 0;
};
