// A 2D side-view platformer character: capsule collision body + a
// capsule-shaped visual built from engine BasicShapes (cylinder + two
// spheres). Movement is horizontal only (world X), jump along Z; the follow
// camera sits at the character's side. All input is created in code, so no
// content assets are required.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "PlatformerCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UStaticMeshComponent;
class UCameraComponent;
class USpringArmComponent;

UCLASS()
class MCP_TEST_API APlatformerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APlatformerCharacter();

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	// Called when the player binds input to this pawn.
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Adds the default mapping context for the local player, if any. */
	void AddDefaultMappingContext();

	// APawn interface
	virtual void PawnClientRestart() override;

	// Input handlers --------------------------------------------------------
	/** Horizontal (world X) movement for the 2D side view (A/D, arrows, left stick X). */
	void MoveHorizontal(const FInputActionValue& Value);

	/** Jump (Space / gamepad south button). */
	void StartJump();
	void StopJump();

	// Falling / checkpoint ------------------------------------------------
	/** If the capsule drops below this Z, teleport back to the checkpoint. */
	UPROPERTY(EditAnywhere, Category = "Checkpoint")
	float RespawnZ = -300.0f;

	/** Teleports the character back to the last checkpoint (spawn point). */
	void RespawnAtCheckpoint();

	/** Progresses the checkpoint when touching a tagged Checkpoint volume. */
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	/** Checkpoint location/rotation, captured where the pawn first spawns. */
	FVector RespawnLocation = FVector::ZeroVector;
	FRotator RespawnRotation = FRotator::ZeroRotator;

	// Visual capsule body ---------------------------------------------------
	/** Middle cylinder of the capsule visual. */
	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> BodyCylinder;

	/** Spherical top of the capsule visual. */
	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> TopSphere;

	/** Spherical bottom of the capsule visual. */
	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> BottomSphere;

	// Third-person follow camera -------------------------------------------
	/** Boom arm holding the camera behind the capsule. */
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArmComp;

	/** Follow camera. */
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComp;

	// Input (created in code, no assets needed) ----------------------------
	UPROPERTY()
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> JumpAction;
};
