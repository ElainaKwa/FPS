#include "FPSDemoHealStation.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

AFPSDemoHealStation::AFPSDemoHealStation()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("StationRoot"));
	RootComponent = SceneRoot;
	StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ModeledStation"));
	StationMesh->SetupAttachment(SceneRoot);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Game/FPSDemo/Meshes/SM_FPS_HealStation.SM_FPS_HealStation"));
	StationMesh->SetStaticMesh(Mesh.Object);
	StationMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoreLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PulsingHealLight"));
	CoreLight->SetupAttachment(SceneRoot);
	CoreLight->SetRelativeLocation(FVector(0, 0, 119));
	CoreLight->SetLightColor(FLinearColor(.08f, 1.0f, .42f));
	CoreLight->SetAttenuationRadius(510.0f);
	CoreLight->SetIntensity(3700.0f);
	UsePrompt = CreateDefaultSubobject<UTextRenderComponent>(TEXT("UsePrompt"));
	UsePrompt->SetupAttachment(SceneRoot);
	UsePrompt->SetRelativeLocation(FVector(0, -110, 225));
	UsePrompt->SetRelativeRotation(FRotator(0, 90, 0));
	UsePrompt->SetText(FText::FromString(TEXT("HEAL  [E]")));
	UsePrompt->SetTextRenderColor(FColor(90, 255, 165));
	UsePrompt->SetWorldSize(55);
	UsePrompt->SetHorizontalAlignment(EHTA_Center);
}

void AFPSDemoHealStation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Pulse = 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 2.5f);
	CoreLight->SetIntensity(3000.0f + 1200.0f * Pulse);
}

bool AFPSDemoHealStation::IsWithinUseRange(const FVector& PlayerLocation) const
{
	return FVector::Dist2D(PlayerLocation, GetActorLocation()) <= UseRadius;
}
