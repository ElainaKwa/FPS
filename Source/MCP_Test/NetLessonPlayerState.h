#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NetLessonPlayerState.generated.h"

/** One per player, created on the server and replicated to all clients. */
UCLASS()
class MCP_TEST_API ANetLessonPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void RecordPersonalRequest();
	int32 GetPersonalRequestCount() const { return PersonalRequestCount; }

private:
	UPROPERTY(Replicated)
	int32 PersonalRequestCount = 0;
};
