#include "NetLessonStage.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ANetLessonStage::ANetLessonStage()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	auto MakeBox = [this](const TCHAR* Name, const FVector& Position, const FVector& Scale)
	{
		UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Mesh->SetupAttachment(Root);
		if (Cube.Succeeded()) Mesh->SetStaticMesh(Cube.Object);
		Mesh->SetRelativeLocation(Position);
		Mesh->SetRelativeScale3D(Scale);
		return Mesh;
	};
	Floor = MakeBox(TEXT("LessonFloor"), FVector(1000, 0, -80), FVector(38, 4, 1));
	ReplicationStation = MakeBox(TEXT("ReplicationStation"), FVector(-420, 0, 10), FVector(2.1, 1.2, 0.8));
	MulticastStation = MakeBox(TEXT("MulticastStation"), FVector(0, 0, 10), FVector(2.1, 1.2, 0.8));
	LocalStation = MakeBox(TEXT("LocalStation"), FVector(420, 0, 10), FVector(2.1, 1.2, 0.8));
	for (UStaticMeshComponent* Station : {ReplicationStation.Get(), MulticastStation.Get(), LocalStation.Get()})
	{
		Station->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	auto MakeText = [this](const TCHAR* Name, const TCHAR* Text, const FVector& Position, const FColor& Color)
	{
		UTextRenderComponent* Label = CreateDefaultSubobject<UTextRenderComponent>(Name);
		Label->SetupAttachment(Root);
		Label->SetRelativeLocation(Position);
		Label->SetRelativeRotation(FRotator(0, 90, 0));
		Label->SetHorizontalAlignment(EHTA_Center);
		Label->SetWorldSize(42.0f);
		Label->SetTextRenderColor(Color);
		Label->SetText(FText::FromString(Text));
		return Label;
	};
	Title = MakeText(TEXT("Title"), TEXT("UE NETWORK BASICS  |  2-PLAYER PIE"), FVector(0, 0, 360), FColor::White);
	CounterLabel = MakeText(TEXT("CounterLabel"), TEXT("1  REPLICATED COUNTER: 0"), FVector(-420, 0, 165), FColor::Cyan);
	MulticastLabel = MakeText(TEXT("MulticastLabel"), TEXT("2  MULTICAST PULSE"), FVector(0, 0, 165), FColor::Yellow);
	LocalLabel = MakeText(TEXT("LocalLabel"), TEXT("3  LOCAL ONLY"), FVector(420, 0, 165), FColor::Green);
	UTextRenderComponent* FrameworkTitle = MakeText(TEXT("FrameworkTitle"),
		TEXT("GAMEPLAY FRAMEWORK"), FVector(1750, 0, 350), FColor::White);
	FrameworkTitle->SetWorldSize(35.0f);
	const TCHAR* ClassNames[] = {
		TEXT("GAME INSTANCE\nENGINE START\nLOCAL"),
		TEXT("GAME MODE\nMAP LOAD\nSERVER"),
		TEXT("GAME STATE\nGAME MODE\nALL"),
		TEXT("CONTROLLER\nLOGIN\nSERVER + OWNER"),
		TEXT("PLAYER STATE\nONE / PLAYER\nALL"),
		TEXT("PAWN\nSPAWN + POSSESS\nALL"),
		TEXT("HUD\nPLAYER CTRL\nLOCAL")};
	const FColor ClassColors[] = {FColor::Silver, FColor::Orange, FColor::Cyan,
		FColor::Yellow, FColor::Cyan, FColor::Green, FColor::Silver};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ClassNames); ++Index)
	{
		const float X = 850.0f + 300.0f * Index;
		UStaticMeshComponent* Plinth = MakeBox(*FString::Printf(TEXT("FrameworkPlinth%d"), Index),
			FVector(X, 0, 10), FVector(2.5, 1.2, 0.8));
		Plinth->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UTextRenderComponent* Label = MakeText(*FString::Printf(TEXT("FrameworkLabel%d"), Index),
			ClassNames[Index], FVector(X, 0, 145), ClassColors[Index]);
		Label->SetWorldSize(25.0f);
	}
}

void ANetLessonStage::BeginPlay()
{
	Super::BeginPlay();
	OnRep_Counter();
}

void ANetLessonStage::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANetLessonStage, Counter);
}

void ANetLessonStage::ServerIncrementCounter()
{
	if (!HasAuthority()) return;
	++Counter;
	OnRep_Counter(); // RepNotify runs on clients; update the listen server explicitly.
	ForceNetUpdate();
}

void ANetLessonStage::ServerBroadcastPulse()
{
	if (HasAuthority()) MulticastPulse();
}

void ANetLessonStage::OnRep_Counter()
{
	if (CounterLabel)
	{
		CounterLabel->SetText(FText::FromString(FString::Printf(TEXT("1  REPLICATED COUNTER: %d"), Counter)));
	}
}

void ANetLessonStage::MulticastPulse_Implementation()
{
	PulseUntil = GetWorld()->GetTimeSeconds() + 1.5f;
	if (MulticastLabel) MulticastLabel->SetTextRenderColor(FColor::Red);
	GetWorldTimerManager().SetTimer(PulseTimer, this, &ANetLessonStage::ResetPulseColor, 1.5f, false);
}

void ANetLessonStage::ResetPulseColor()
{
	if (MulticastLabel) MulticastLabel->SetTextRenderColor(FColor::Yellow);
}
