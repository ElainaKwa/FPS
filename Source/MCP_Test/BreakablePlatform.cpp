#include "BreakablePlatform.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "UObject/ConstructorHelpers.h"

ABreakablePlatform::ABreakablePlatform()
{
	PrimaryActorTick.bCanEverTick = true;

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	SetRootComponent(PlatformMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PlatformMesh->SetStaticMesh(CubeMesh.Object);
	}

	PlatformMesh->SetCollisionProfileName(TEXT("BlockAll"));
	// The collision body stays put so a standing character is not shaken;
	// only the visual copy below trembles. It renders in the editor but is
	// hidden in game (the visual copy takes over there).
	PlatformMesh->SetMobility(EComponentMobility::Static);
	PlatformMesh->SetHiddenInGame(true);

	// Separate visible mesh: trembles during the countdown, never collides.
	ShakeVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShakeVisual"));
	ShakeVisual->SetupAttachment(PlatformMesh);
	if (CubeMesh.Succeeded())
	{
		ShakeVisual->SetStaticMesh(CubeMesh.Object);
	}
	ShakeVisual->SetCollisionProfileName(TEXT("NoCollision"));
	ShakeVisual->SetMobility(EComponentMobility::Movable);
	// Hidden in the editor viewport so the two cubes don't overlap; shown
	// when the game starts (the root is HiddenInGame there).
	ShakeVisual->SetVisibility(false);
}

void ABreakablePlatform::BeginPlay()
{
	Super::BeginPlay();

	PlatformBox = PlatformMesh->Bounds.GetBox();

	// Show the visual copy (the root is HiddenInGame) and cache its rest pose.
	ShakeVisual->SetVisibility(true);
	BaseRelativeLocation = ShakeVisual->GetRelativeLocation();
	BaseRelativeRotation = ShakeVisual->GetRelativeRotation();
}

void ABreakablePlatform::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bBroken)
	{
		BrokenElapsed += DeltaSeconds;
		if (BrokenElapsed >= RespawnAfterBreakSeconds)
		{
			RestorePlatform();
		}
		return;
	}

	// Arm the countdown the first time a character stands on the platform.
	// Once armed it keeps counting down even if the character leaves, so the
	// platform is guaranteed to break BreakAfterStandSeconds after first contact.
	if (!bTriggered)
	{
		if (HasStandingCharacter())
		{
			bTriggered = true;
			TriggerElapsed = 0.0f;
		}
		return;
	}

	// Tremble more and more as the break approaches: the offset ramps from
	// zero to ShakeAmplitude, strongest along the horizontal play plane.
	// Only the visual mesh shakes; the collision body (and anyone standing
	// on it) stays perfectly still.
	const float Progress = FMath::Clamp(TriggerElapsed / BreakAfterStandSeconds, 0.0f, 1.0f);
	const float Amplitude = ShakeAmplitude * Progress;
	ShakeVisual->SetRelativeLocation(BaseRelativeLocation + FVector(
		FMath::FRandRange(-Amplitude, Amplitude),
		FMath::FRandRange(-Amplitude * 0.3f, Amplitude * 0.3f),
		FMath::FRandRange(-Amplitude * 0.6f, Amplitude * 0.6f)));
	ShakeVisual->SetRelativeRotation(BaseRelativeRotation + FRotator(
		FMath::FRandRange(-1.0f, 1.0f) * Progress,
		FMath::FRandRange(-1.0f, 1.0f) * Progress,
		FMath::FRandRange(-1.0f, 1.0f) * Progress));

	TriggerElapsed += DeltaSeconds;
	if (TriggerElapsed >= BreakAfterStandSeconds)
	{
		BreakPlatform();
	}
}

bool ABreakablePlatform::HasStandingCharacter() const
{
	if (PlatformBox.Max.X <= PlatformBox.Min.X || PlatformBox.Max.Z <= PlatformBox.Min.Z)
	{
		return false;
	}

	const float TopZ = PlatformBox.Max.Z;
	bool bSomeone = false;

	for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
	{
		const ACharacter* Character = *It;
		const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
		if (Capsule == nullptr)
		{
			continue;
		}

		const FVector Location = Capsule->GetComponentLocation();
		const float BottomZ = Location.Z - Capsule->GetScaledCapsuleHalfHeight();

		// Feet must rest on the top face (small tolerance) inside the footprint.
		const bool bInsideX = Location.X >= PlatformBox.Min.X - 5.0f && Location.X <= PlatformBox.Max.X + 5.0f;
		const bool bInsideY = Location.Y >= PlatformBox.Min.Y - 5.0f && Location.Y <= PlatformBox.Max.Y + 5.0f;
		const bool bOnTop = BottomZ >= TopZ - 15.0f && BottomZ <= TopZ + 10.0f;

		if (bInsideX && bInsideY && bOnTop)
		{
			bSomeone = true;
			break;
		}
	}

	return bSomeone;
}

void ABreakablePlatform::BreakPlatform()
{
	bBroken = true;
	BrokenElapsed = 0.0f;

	ShakeVisual->SetVisibility(false, true);
	PlatformMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Crumble the platform into falling cube shards.
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const FVector Center = PlatformBox.GetCenter();
	const FVector Extents = PlatformBox.GetExtent();

	for (int32 i = 0; i < ShardCount; ++i)
	{
		const FVector SpawnLocation(
			Center.X + FMath::FRandRange(-Extents.X * 0.8f, Extents.X * 0.8f),
			Center.Y + FMath::FRandRange(-Extents.Y * 0.8f, Extents.Y * 0.8f),
			PlatformBox.Max.Z - FMath::FRandRange(5.0f, 25.0f));

		AStaticMeshActor* Shard = World->SpawnActor<AStaticMeshActor>(
			AStaticMeshActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator);
		if (Shard == nullptr)
		{
			continue;
		}

		UStaticMeshComponent* ShardMesh = Shard->GetStaticMeshComponent();
		// Make the shard movable before assigning the mesh: calling SetStaticMesh
		// on a Static-mobility component logs a warning.
		ShardMesh->SetMobility(EComponentMobility::Movable);
		ShardMesh->SetStaticMesh(PlatformMesh->GetStaticMesh());
		// "PhysicsActor" keeps collision enabled so the engine actually creates
		// a simulating body, while every channel ignores it: shards pass through
		// everything. ("NoCollision" prevents the physics state from existing at
		// all, which would leave the shards frozen in mid-air.)
		ShardMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
		ShardMesh->SetSimulatePhysics(true);
		// Belt and braces: make sure the body exists before velocities are set.
		ShardMesh->RecreatePhysicsState();

		// Burst the shard outward from the platform center with a purely
		// horizontal initial velocity (zero vertical pop): gravity then pulls
		// it down in a clean parabolic arc - projectile motion.
		FVector HorizontalDir = SpawnLocation - Center;
		HorizontalDir.Z = 0.0f;
		if (HorizontalDir.IsNearlyZero())
		{
			HorizontalDir = FVector(1.0f, 0.0f, 0.0f);
		}
		HorizontalDir.Normalize();
		ShardMesh->SetPhysicsLinearVelocity(HorizontalDir *
			FMath::FRandRange(ShardBurstSpeedMin, ShardBurstSpeedMax));
		ShardMesh->SetPhysicsAngularVelocityInRadians(FVector(
			FMath::FRandRange(-6.0f, 6.0f),
			FMath::FRandRange(-6.0f, 6.0f),
			FMath::FRandRange(-6.0f, 6.0f)));

		Shard->SetActorScale3D(FVector(
			FMath::FRandRange(0.3f, 0.7f),
			FMath::FRandRange(0.3f, 0.7f),
			FMath::FRandRange(0.12f, 0.25f)));

		Shards.Add(Shard);
	}
}

void ABreakablePlatform::RestorePlatform()
{
	// Remove the shards.
	for (AStaticMeshActor* Shard : Shards)
	{
		if (Shard != nullptr)
		{
			if (UStaticMeshComponent* ShardMesh = Shard->GetStaticMeshComponent())
			{
				ShardMesh->SetSimulatePhysics(false);
			}
			Shard->Destroy();
		}
	}
	Shards.Reset();

	// Rebuild in place, clearing any leftover shake offset.
	ShakeVisual->SetRelativeLocation(BaseRelativeLocation);
	ShakeVisual->SetRelativeRotation(BaseRelativeRotation);
	ShakeVisual->SetVisibility(true, true);
	PlatformMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	bBroken = false;
	bTriggered = false;
	TriggerElapsed = 0.0f;
}
