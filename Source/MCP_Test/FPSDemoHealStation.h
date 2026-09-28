#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSDemoHealStation.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Reusable modeled station. The server checks range before applying the heal GameplayEffect. */
UCLASS()
class MCP_TEST_API AFPSDemoHealStation : public AActor
{
	GENERATED_BODY()

public:
	AFPSDemoHealStation();
	virtual void Tick(float DeltaSeconds) override;
	bool IsWithinUseRange(const FVector& PlayerLocation) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<UStaticMeshComponent> StationMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<UPointLightComponent> CoreLight;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<UTextRenderComponent> UsePrompt;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gameplay", meta=(ClampMin="50.0"))
	float UseRadius = 290.0f;
};
