#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NetLessonGameState.generated.h"

/** One copy per world, spawned by GameMode on the server and replicated to every client. */
UCLASS()
class MCP_TEST_API ANetLessonGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void RecordSharedRequest();
	int32 GetSharedRequestCount() const { return SharedRequestCount; }

private:
	UPROPERTY(Replicated)
	int32 SharedRequestCount = 0;
};
