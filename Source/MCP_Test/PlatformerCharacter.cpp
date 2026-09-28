#include "PlatformerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "UObject/ConstructorHelpers.h"

APlatformerCharacter::APlatformerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Capsule collision body (ACharacter default: radius 34, half height 88).
	GetCapsuleComponent()->InitCapsuleSize(34.0f, 88.0f);

	// 2D platformer feel: horizontal movement only, rotation locked, snappy
	// jump with variable height (releasing Space early cuts the jump short).
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = false;
	MoveComp->bAllowPhysicsRotationDuringAnimRootMotion = false;
	MoveComp->JumpZVelocity = 850.0f;
	MoveComp->AirControl = 0.5f;
	MoveComp->MaxWalkSpeed = 650.0f;
	MoveComp->GravityScale = 1.6f;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// -----------------------------------------------------------------------
	// Visual capsule body, built from engine BasicShapes (100-unit primitives):
	//   radius 34 -> scale 0.68 on the 50-unit-radius sphere/cylinder meshes.
	//   total visual height 176 (2x half height 88); cylinder z-scale 1.08
	//   stretches the 100-unit cylinder to the 108-unit straight middle part.
	// -----------------------------------------------------------------------
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	BodyCylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CapsuleBodyCylinder"));
	TopSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CapsuleVisualTop"));
	BottomSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CapsuleVisualBottom"));

	BodyCylinder->SetupAttachment(GetCapsuleComponent());
	TopSphere->SetupAttachment(GetCapsuleComponent());
	BottomSphere->SetupAttachment(GetCapsuleComponent());

	const FVector CylinderScale(0.68f, 0.68f, 1.08f);
	const float SphereScale = 0.68f;
	const float SphereCenterZ = 54.0f; // 88 (half height) - 34 (radius)

	if (CylinderMesh.Succeeded())
	{
		BodyCylinder->SetStaticMesh(CylinderMesh.Object);
	}
	if (SphereMesh.Succeeded())
	{
		TopSphere->SetStaticMesh(SphereMesh.Object);
		BottomSphere->SetStaticMesh(SphereMesh.Object);
	}

	BodyCylinder->SetRelativeScale3D(CylinderScale);
	TopSphere->SetRelativeScale3D(FVector(SphereScale));
	BottomSphere->SetRelativeScale3D(FVector(SphereScale));

	TopSphere->SetRelativeLocation(FVector(0.0f, 0.0f, SphereCenterZ));
	BottomSphere->SetRelativeLocation(FVector(0.0f, 0.0f, -SphereCenterZ));

	// The visual parts never collide; only the capsule collision body does.
	auto DisableVisualCollision = [](UStaticMeshComponent* VisualPart)
	{
		VisualPart->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		VisualPart->SetCollisionProfileName(TEXT("NoCollision"));
	};
	DisableVisualCollision(BodyCylinder);
	DisableVisualCollision(TopSphere);
	DisableVisualCollision(BottomSphere);

	// -----------------------------------------------------------------------
	// Side camera: exactly perpendicular to the X/Z play plane, straight at
	// the character's side profile (no vertical offset, no tilt, no lag so
	// the frame never trails the capsule).
	// -----------------------------------------------------------------------
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("FollowCameraBoom"));
	SpringArmComp->SetupAttachment(GetCapsuleComponent());
	SpringArmComp->TargetArmLength = 950.0f;
	SpringArmComp->SocketOffset = FVector::ZeroVector;
	SpringArmComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	SpringArmComp->bUsePawnControlRotation = false;
	SpringArmComp->bInheritPitch = false;
	SpringArmComp->bInheritYaw = false;
	SpringArmComp->bInheritRoll = false;
	// Smooth camera that trails the player: it chases the capsule at a fixed
	// catch-up rate, so fast runs/jumps leave it briefly behind and it glides
	// back once the player slows down. Rotation stays locked side-on.
	SpringArmComp->bEnableCameraLag = true;
	SpringArmComp->CameraLagSpeed = 5.0f;
	SpringArmComp->CameraLagMaxDistance = 450.0f;
	SpringArmComp->bEnableCameraRotationLag = false;

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;
	CameraComp->FieldOfView = 90.0f;

	// -----------------------------------------------------------------------
	// Input assets created entirely in code (Enhanced Input).
	// -----------------------------------------------------------------------
	DefaultMappingContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("DefaultMappingContext"));

	MoveAction = CreateDefaultSubobject<UInputAction>(TEXT("MoveAction"));
	MoveAction->ValueType = EInputActionValueType::Axis1D;

	JumpAction = CreateDefaultSubobject<UInputAction>(TEXT("JumpAction"));
	JumpAction->ValueType = EInputActionValueType::Boolean;

	UInputModifierNegate* NegateModifier = CreateDefaultSubobject<UInputModifierNegate>(TEXT("InputNegateModifier"));

	// Horizontal movement: D / Right arrow positive, A / Left arrow negated,
	// gamepad stick X.
	DefaultMappingContext->MapKey(MoveAction, EKeys::D);
	DefaultMappingContext->MapKey(MoveAction, EKeys::Right);
	FEnhancedActionKeyMapping& LeftMapping = DefaultMappingContext->MapKey(MoveAction, EKeys::A);
	LeftMapping.Modifiers.Add(NegateModifier);
	FEnhancedActionKeyMapping& LeftArrowMapping = DefaultMappingContext->MapKey(MoveAction, EKeys::Left);
	LeftArrowMapping.Modifiers.Add(NegateModifier);
	DefaultMappingContext->MapKey(MoveAction, EKeys::Gamepad_LeftX);

	// Jump.
	DefaultMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	DefaultMappingContext->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);
}

void APlatformerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// The first checkpoint is where the pawn spawned (the level PlayerStart).
	RespawnLocation = GetActorLocation();
	RespawnRotation = GetActorRotation();

	if (IsLocallyControlled())
	{
		AddDefaultMappingContext();
	}
}

void APlatformerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Fell into the void below the course -> back to the checkpoint.
	if (GetActorLocation().Z < RespawnZ)
	{
		RespawnAtCheckpoint();
	}
}

void APlatformerCharacter::RespawnAtCheckpoint()
{
	GetCharacterMovement()->StopMovementImmediately();
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetPhysicsLinearVelocity(FVector::ZeroVector);
		GetCapsuleComponent()->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
	}
	SetActorLocationAndRotation(RespawnLocation, RespawnRotation, false, nullptr, ETeleportType::TeleportPhysics);
}

void APlatformerCharacter::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (OtherActor != nullptr && OtherActor->ActorHasTag(TEXT("Checkpoint")))
	{
		const FVector Candidate = OtherActor->GetActorLocation();

		// Checkpoints only move forward along the course: overlapping a
		// checkpoint volume stores its center as the new respawn point.
		if (Candidate.X > RespawnLocation.X)
		{
			RespawnLocation = Candidate;
			RespawnRotation = FRotator::ZeroRotator;
		}
	}
}

void APlatformerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlatformerCharacter::MoveHorizontal);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &APlatformerCharacter::MoveHorizontal);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &APlatformerCharacter::StartJump);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlatformerCharacter::StopJump);
	}
}

void APlatformerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	AddDefaultMappingContext();
}

void APlatformerCharacter::AddDefaultMappingContext()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void APlatformerCharacter::MoveHorizontal(const FInputActionValue& Value)
{
	if (Controller != nullptr)
	{
		// Actor rotation is locked, so world X == actor forward: positive
		// input walks right (+X), negative walks left (-X).
		AddMovementInput(FVector::ForwardVector, Value.Get<float>());
	}
}

void APlatformerCharacter::StartJump()
{
	Jump();
}

void APlatformerCharacter::StopJump()
{
	StopJumping();
}
