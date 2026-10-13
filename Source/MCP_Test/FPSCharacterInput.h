#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSCharacterInput.generated.h"
class AFPSDemoCharacter;
class UInputMappingContext;
class UInputAction;
class UEnhancedInputLocalPlayerSubsystem;
/** Owns player mappings and binds input to verbs; AI invokes those verbs without this component. */
UCLASS()
class MCP_TEST_API UFPSCharacterInput : public UActorComponent
{
    GENERATED_BODY()
public:
    UFPSCharacterInput();
    void Bind(AFPSDemoCharacter* Character, UInputComponent* InputComponent);
    void Register(AFPSDemoCharacter* Character);
    void Unregister();
    void SuspendForMenu();
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
private:
    void Forward();
    void Backward();
    void Right();
    void Left();
    UPROPERTY() TObjectPtr<UInputAction> BackwardAction;
    UPROPERTY() TObjectPtr<UInputAction> LeftAction;
    void RebuildMappings();
    void AimPressed();
    void AimReleased();
    void AimCanceled();
    void NextWeapon();
    void Interact();
    uint32 SettingsRevision=MAX_uint32;
    bool bSuspended=false;
    UPROPERTY() TObjectPtr<UInputAction> NextWeaponAction;
    friend struct FFPSMovementSmoke;
    void SprintPressed();
    void SprintReleased();
    void SprintCanceled();
    void CrouchPressed();
    void CrouchReleased();
    void CrouchCanceled();
    TWeakObjectPtr<AFPSDemoCharacter> InputCharacter;
	UPROPERTY()
	TObjectPtr<UInputMappingContext> InputMap;
	UPROPERTY()
	TObjectPtr<UInputAction> ForwardAction;
	UPROPERTY()
	TObjectPtr<UInputAction> RightAction;
	UPROPERTY()
	TObjectPtr<UInputAction> YawAction;
	UPROPERTY()
	TObjectPtr<UInputAction> PitchAction;
	UPROPERTY()
	TObjectPtr<UInputAction> FireAction;
	UPROPERTY()
	TObjectPtr<UInputAction> JumpAction;
	UPROPERTY()
	TObjectPtr<UInputAction> HealAction;

	UPROPERTY() TObjectPtr<UInputAction> ReloadAction;
	UPROPERTY() TObjectPtr<UInputAction> AimAction;
	UPROPERTY() TObjectPtr<UInputAction> InspectAction;
	UPROPERTY() TObjectPtr<UInputAction> PrimaryAction;
	UPROPERTY() TObjectPtr<UInputAction> SecondaryAction;
	UPROPERTY() TObjectPtr<UInputAction> DropAction;
	UPROPERTY() TObjectPtr<UInputAction> PickupAction;
	UPROPERTY() TObjectPtr<UInputAction> SprintAction;
	UPROPERTY() TObjectPtr<UInputAction> CrouchAction;
	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> RegisteredInputSubsystem;
};
