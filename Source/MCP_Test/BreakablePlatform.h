// Breakable platform for the platformer: the first time a character steps
// onto it a countdown starts; a few seconds later the platform crumbles into
// physics shards and rebuilds itself in place after a delay. The countdown
// keeps running even if the character jumps off. While the countdown runs,
// only a separate visual mesh trembles - the collision body stays put, so a
// standing character is not shaken. All meshes use engine BasicShapes, no
// content assets required.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BreakablePlatform.generated.h"

class UStaticMeshComponent;
class AStaticMeshActor;

UCLASS()
class MCP_TEST_API ABreakablePlatform : public AActor
{
	GENERATED_BODY()

public:
	ABreakablePlatform();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaSeconds) override;

	/** Seconds after a character first steps on before the platform breaks (default 1.25). The countdown keeps running even if the character leaves. */
	UPROPERTY(EditAnywhere, Category = "Breakable")
	float BreakAfterStandSeconds = 1.25f;

	/** Seconds until the platform rebuilds at its original spot (default 3.5). */
	UPROPERTY(EditAnywhere, Category = "Breakable")
	float RespawnAfterBreakSeconds = 3.5f;

	/** Maximum shake offset in cm, reached right before the platform breaks. The tremor ramps up from zero over the countdown. */
	UPROPERTY(EditAnywhere, Category = "Breakable")
	float ShakeAmplitude = 10.0f;

	/** How many cube shards are spawned when the platform breaks. */
	UPROPERTY(EditAnywhere, Category = "Breakable")
	int32 ShardCount = 8;

	/** Horizontal burst speed range (cm/s) given to each shard when the platform breaks. The vertical velocity starts at zero, so every shard follows a clean parabolic arc. */
	UPROPERTY(EditAnywhere, Category = "Breakable", meta = (ClampMin = "0.0"))
	float ShardBurstSpeedMin = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Breakable", meta = (ClampMin = "0.0"))
	float ShardBurstSpeedMax = 650.0f;

	/** True while the platform is broken (readable for debugging). */
	UPROPERTY(VisibleAnywhere, Category = "Breakable")
	bool bBroken = false;

private:
	/** Breaks the platform: hide/disable collision and spawn falling shards. */
	void BreakPlatform();

	/** Rebuilds the platform and destroys the shards. */
	void RestorePlatform();

	/** Returns true when at least one character is standing on the top face. */
	bool HasStandingCharacter() const;

	/** The collision body of the platform: hidden in game, never moves. */
	UPROPERTY(VisibleAnywhere, Category = "Breakable")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;

	/** The visible mesh that trembles during the countdown (no collision). */
	UPROPERTY(VisibleAnywhere, Category = "Breakable")
	TObjectPtr<UStaticMeshComponent> ShakeVisual;

	/** Cached world-space box of the platform top surface. */
	FBox PlatformBox;

	/** Rest pose of the shake visual, cached so the tremor offset can be reset. */
	FVector BaseRelativeLocation = FVector::ZeroVector;
	FRotator BaseRelativeRotation = FRotator::ZeroRotator;

	/** True once a character has stepped on and the break countdown has started. */
	bool bTriggered = false;

	/** Countdown elapsed since the trigger (seconds). */
	float TriggerElapsed = 0.0f;

	/** Accumulated broken time (seconds). */
	float BrokenElapsed = 0.0f;

	/** Falling shard actors (destroyed when the platform rebuilds). */
	UPROPERTY()
	TArray<TObjectPtr<AStaticMeshActor>> Shards;
};
