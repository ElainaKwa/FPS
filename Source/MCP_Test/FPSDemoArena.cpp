#include "FPSDemoArena.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AFPSDemoArena::AFPSDemoArena()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ArenaRoot"));
	RootComponent = SceneRoot;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FloorMaterial(TEXT("/Game/FPSDemo/Materials/MI_FPS_Floor.MI_FPS_Floor"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallMaterial(TEXT("/Game/FPSDemo/Materials/MI_FPS_Wall.MI_FPS_Wall"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CoverMaterial(TEXT("/Game/FPSDemo/Materials/MI_FPS_Cover.MI_FPS_Cover"));
	auto Part = [this, SceneRoot](const TCHAR* Name, const FVector& Position,
		const FVector& Scale, bool bSolid = true)
	{
		UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Mesh->SetupAttachment(SceneRoot);
		Mesh->SetStaticMesh(Cube.Object);
		const FString PartName(Name);
		Mesh->SetMaterial(0, PartName.Contains(TEXT("Floor")) ? FloorMaterial.Object
			: PartName.Contains(TEXT("Wall")) ? WallMaterial.Object
			: CoverMaterial.Object);
		Mesh->SetRelativeLocation(Position);
		Mesh->SetRelativeScale3D(Scale);
		Mesh->SetCollisionProfileName(bSolid ? TEXT("BlockAll") : TEXT("NoCollision"));
		return Mesh;
	};
	auto Label = [this, SceneRoot](const TCHAR* Name, const TCHAR* Message, const FVector& Position,
		const FRotator& Rotation, const FColor& Color, float Size)
	{
		UTextRenderComponent* Text = CreateDefaultSubobject<UTextRenderComponent>(Name);
		Text->SetupAttachment(SceneRoot);
		Text->SetRelativeLocation(Position);
		Text->SetRelativeRotation(Rotation);
		Text->SetText(FText::FromString(Message));
		Text->SetTextRenderColor(Color);
		Text->SetWorldSize(Size);
		Text->SetHorizontalAlignment(EHTA_Center);
		return Text;
	};

	Part(TEXT("ConcreteFloor"), FVector(0, 0, -70), FVector(42, 32, 1));
	Part(TEXT("NorthWall"), FVector(0, 1610, 190), FVector(42, .35f, 4.2f));
	Part(TEXT("SouthWall"), FVector(0, -1610, 190), FVector(42, .35f, 4.2f));
	Part(TEXT("EastWall"), FVector(2110, 0, 190), FVector(.35f, 32, 4.2f));
	Part(TEXT("WestWall"), FVector(-2110, 0, 190), FVector(.35f, 32, 4.2f));
	// Short cover keeps the initial two spawn points in sight of each other.
	Part(TEXT("WestCoverNorth"), FVector(-710, 780, 65), FVector(3, .7f, 1.7f));
	Part(TEXT("WestCoverSouth"), FVector(-750, -930, 65), FVector(2.6f, .7f, 1.7f));
	Part(TEXT("EastCoverNorth"), FVector(810, 930, 65), FVector(2.7f, .7f, 1.7f));
	Part(TEXT("EastCoverSouth"), FVector(720, -750, 65), FVector(3.1f, .7f, 1.7f));
	Part(TEXT("WestMarker"), FVector(-1640, -580, 25), FVector(1.3f, .07f, .45f), false);
	Part(TEXT("EastMarker"), FVector(1640, 580, 25), FVector(1.3f, .07f, .45f), false);
	Label(TEXT("NorthBanner"), TEXT("SERVER AUTHORITY  /  RPC  /  REPLICATION"), FVector(0, 1544, 175),
		FRotator(0, -90, 0), FColor(110, 225, 255), 76);
	Label(TEXT("SouthBanner"), TEXT("GAS HEALTH  /  HIT CONFIRM  /  RESPAWN"), FVector(0, -1544, 175),
		FRotator(0, 90, 0), FColor(255, 180, 75), 76);
	Label(TEXT("SpawnA"), TEXT("CLIENT A"), FVector(-1500, -500, 5),
		FRotator(0, 0, 0), FColor(110, 225, 255), 46);
	Label(TEXT("SpawnB"), TEXT("CLIENT B"), FVector(1500, 500, 5),
		FRotator(0, 180, 0), FColor(255, 180, 75), 46);
}
