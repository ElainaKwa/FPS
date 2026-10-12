#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "FPSWeaponVisualTiming.h"
#include "FPSDemoCharacter.generated.h"

class UFPSLifeComponent;
class UFPSActionPolicy;
class UFPSCharacterInput;
class UFPSCharacterPresentation;
struct FFPSDeathContext;
class USoundBase;
struct FFPSWeaponShotContext;

class UAbilitySystemComponent;
class UCameraComponent;
class UFPSDemoAttributeSet;
class UInputAction;
class UInputMappingContext;
class UEnhancedInputLocalPlayerSubsystem;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

/** C++ modeled FPS avatar and gun. Recoil/bob are procedural local animation. */
UCLASS()
class MCP_TEST_API AFPSDemoCharacter : public ACharacter, public IAbilitySystemInterface
{
	friend struct FFPSLoadoutSmoke;
    friend struct FFPSDropSmoke;
	GENERATED_BODY()

public:
	AFPSDemoCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual FVector GetPawnViewLocation() const override;

	UFPSLifeComponent* GetLife() const { return Life; }
	UFPSActionPolicy* GetActionPolicy() const { return ActionPolicy; }
	float GetHealth() const;
	float GetMaxHealth() const;
	bool IsEliminated() const { return bEliminated; }
	static constexpr float RespawnDelay = 2.5f;
	float GetRespawnSeconds() const;
	const FString& GetKillerName() const { return EliminatedBy; }
	void ApplyDamageEffectTo(AFPSDemoCharacter* Target, const FFPSWeaponShotContext& Shot);
	void FireBotShot();
	class UFPSWeaponComponent* GetWeapons() const { return Weapons; }
	void ResolveWeaponShot(const FFPSWeaponShotContext& Shot);
	void PlayWeaponShot(const FVector& Kick);
	void RefreshWeaponPresentation();
	void StopCombatInput();

protected:
	UPROPERTY(VisibleAnywhere, Category="FPS") TObjectPtr<UFPSLifeComponent> Life;
	UPROPERTY(VisibleAnywhere, Category="FPS") TObjectPtr<class UFPSBaseMembership> BaseMembership;
	UPROPERTY(VisibleAnywhere, Category="FPS") TObjectPtr<UFPSActionPolicy> ActionPolicy;
	UPROPERTY(VisibleAnywhere, Category="FPS") TObjectPtr<UFPSCharacterInput> CharacterInput;
	UPROPERTY(VisibleAnywhere, Category="FPS") TObjectPtr<UFPSCharacterPresentation> Presentation;
    // Camera/avatar settings, independent of weapon equip or gameplay recoil recovery.
    UPROPERTY(EditAnywhere, Category="FPS|Camera", meta=(ClampMin="30", ClampMax="150"))
    float BaseFieldOfView = 92.f;
    UPROPERTY(EditAnywhere, Category="FPS|Presentation", meta=(ClampMin="0"))
    float VisualRecoilReturnTime = .25f;
	UPROPERTY(VisibleAnywhere, Category="FPS")
	TObjectPtr<class UFPSAvatarVisuals> AvatarVisuals;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="FPS")
	TObjectPtr<class UFPSWeaponComponent> Weapons;
	UPROPERTY(VisibleAnywhere, Category="GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY(VisibleAnywhere, Category="GAS")
	TObjectPtr<UFPSDemoAttributeSet> Attributes;

	UPROPERTY(VisibleAnywhere, Category="FPS")
	TObjectPtr<UCameraComponent> FirstPersonCamera;
	UPROPERTY(VisibleAnywhere, Category="FPS")
	TObjectPtr<USceneComponent> GunRoot;
	UPROPERTY(VisibleAnywhere, Category="FPS")
	TObjectPtr<UStaticMeshComponent> FirstPersonRifle;
	UPROPERTY(VisibleAnywhere, Category="FPS")
	TObjectPtr<UStaticMeshComponent> WorldRifle;
	UPROPERTY(VisibleAnywhere, Category="FPS")
	TObjectPtr<UPointLightComponent> MuzzleLight;
	UPROPERTY(VisibleAnywhere, Category="FPS")
	TObjectPtr<UPointLightComponent> RemoteMuzzleLight;
	UPROPERTY(VisibleAnywhere, Category="FPS")
	TObjectPtr<UStaticMeshComponent> LeftLeg;
	UPROPERTY(VisibleAnywhere, Category="FPS")
	TObjectPtr<UStaticMeshComponent> RightLeg;
	UPROPERTY(VisibleAnywhere, Category="FPS")
	TObjectPtr<UStaticMeshComponent> LeftArm;
	UPROPERTY(VisibleAnywhere, Category="FPS")
	TObjectPtr<UStaticMeshComponent> RightArm;
	UPROPERTY(VisibleAnywhere, Category="FPS")
	TObjectPtr<class UTextRenderComponent> TeamMarker;

	UPROPERTY(ReplicatedUsing=OnRep_Eliminated)
	bool bEliminated = false;
	UPROPERTY(Replicated)
	double RespawnsAt = 0;
	UPROPERTY(Replicated)
	FString EliminatedBy;
	UFUNCTION()
	void OnRep_Eliminated();
	UFUNCTION(Server, Reliable)
	void ServerFire();
	UFUNCTION(Server, Reliable)
	void ServerRequestHeal();
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastShotFX(FVector_NetQuantize Start, USoundBase* ShotSound);

private:
    friend struct FFPSCharacterSmoke;
    friend struct FFPSMovementSmoke;
	void MarkEliminated(const FString& KillerName);
	friend class UFPSCharacterInput;
	friend class UFPSCharacterPresentation;
	void OnLifeEnded(UFPSLifeComponent* Component, const FFPSDeathContext& Context);
	void ResolveShot(float Range, USoundBase* FireSound, const FFPSWeaponShotContext* WeaponShot);
	void ApplyTeachingDamageEffectTo(AFPSDemoCharacter* Target);
    friend struct FFPSTeachingSmoke;
    friend struct FFPSWeaponAbilitySmoke;
    friend struct FFPSWeaponPresentationSmoke;
	void ReleaseFire();
	void SprintOn();
	void SprintOff();
	void CrouchOn();
	void CrouchOff();
	void AddInputMap();
	void MoveForward(const FInputActionValue& Value);
	void MoveRight(const FInputActionValue& Value);
	void LookYaw(const FInputActionValue& Value);
	void LookPitch(const FInputActionValue& Value);
	void PressFire();
	void PressHeal();
	void PressJump();
	void ReleaseJump();
	void TriggerLocalShotAnimation();

	float LastLocalFire = -10.0f;
	float LastServerFire = -10.0f;
	float LastServerHeal = -10.0f;
};
