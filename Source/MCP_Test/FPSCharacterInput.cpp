#include "FPSCharacterInput.h"
#include "FPSDemoCharacter.h"
#include "FPSWeaponComponent.h"
#include "FPSMovementComponent.h"
#include "FPSInputSettings.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
UFPSCharacterInput::UFPSCharacterInput()
{
	InputMap = CreateDefaultSubobject<UInputMappingContext>(TEXT("FPSInputMap"));
	ForwardAction = CreateDefaultSubobject<UInputAction>(TEXT("ForwardAction"));
	RightAction = CreateDefaultSubobject<UInputAction>(TEXT("RightAction"));
	YawAction = CreateDefaultSubobject<UInputAction>(TEXT("YawAction"));
	PitchAction = CreateDefaultSubobject<UInputAction>(TEXT("PitchAction"));
	FireAction = CreateDefaultSubobject<UInputAction>(TEXT("FireAction"));
	JumpAction = CreateDefaultSubobject<UInputAction>(TEXT("FPSJumpAction"));
	HealAction = CreateDefaultSubobject<UInputAction>(TEXT("HealAction"));
	for (UInputAction* Action : {ForwardAction.Get(), RightAction.Get(), YawAction.Get(), PitchAction.Get()})
		Action->ValueType = EInputActionValueType::Axis1D;
	for (UInputAction* Action : {FireAction.Get(), JumpAction.Get(), HealAction.Get()})
		Action->ValueType = EInputActionValueType::Boolean;
	UInputModifierNegate* Negate = CreateDefaultSubobject<UInputModifierNegate>(TEXT("ReverseAxis"));
	InputMap->MapKey(ForwardAction, EKeys::W);
	InputMap->MapKey(ForwardAction, EKeys::S).Modifiers.Add(Negate);
	InputMap->MapKey(RightAction, EKeys::D);
	InputMap->MapKey(RightAction, EKeys::A).Modifiers.Add(Negate);
	InputMap->MapKey(YawAction, EKeys::MouseX);
	InputMap->MapKey(PitchAction, EKeys::MouseY);
	InputMap->MapKey(FireAction, EKeys::LeftMouseButton);
	InputMap->MapKey(JumpAction, EKeys::SpaceBar);
	InputMap->MapKey(HealAction, EKeys::E);
	auto Action = [this](const TCHAR* Name, const FKey& Key)
	{
		auto* Value = CreateDefaultSubobject<UInputAction>(Name);
		Value->ValueType = EInputActionValueType::Boolean;
		InputMap->MapKey(Value, Key);
		return Value;
	};
	ReloadAction = Action(TEXT("Reload"), EKeys::R);
	AimAction = Action(TEXT("Aim"), EKeys::RightMouseButton);
	InspectAction = Action(TEXT("Inspect"), EKeys::Q);
	PrimaryAction = Action(TEXT("PrimaryWeapon"), EKeys::One);
	SecondaryAction = Action(TEXT("SecondaryWeapon"), EKeys::Two);
	DropAction = Action(TEXT("DropWeapon"), EKeys::G);
	PickupAction = Action(TEXT("PickupWeapon"), EKeys::F);
	SprintAction = Action(TEXT("Sprint"), EKeys::LeftShift);
	CrouchAction = Action(TEXT("Crouch"), EKeys::LeftControl);
}
void UFPSCharacterInput::Bind(AFPSDemoCharacter* Character, UInputComponent* PlayerInputComponent)
{
    InputCharacter = Character;
    auto* Weapons = Character->GetWeapons();
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(ForwardAction, ETriggerEvent::Triggered, Character, &AFPSDemoCharacter::MoveForward);
		Input->BindAction(RightAction, ETriggerEvent::Triggered, Character, &AFPSDemoCharacter::MoveRight);
		Input->BindAction(YawAction, ETriggerEvent::Triggered, Character, &AFPSDemoCharacter::LookYaw);
		Input->BindAction(PitchAction, ETriggerEvent::Triggered, Character, &AFPSDemoCharacter::LookPitch);
		Input->BindAction(FireAction, ETriggerEvent::Started, Character, &AFPSDemoCharacter::PressFire);
		Input->BindAction(FireAction, ETriggerEvent::Completed, Character, &AFPSDemoCharacter::ReleaseFire);
		Input->BindAction(FireAction, ETriggerEvent::Canceled, Character, &AFPSDemoCharacter::ReleaseFire);
		Input->BindAction(ReloadAction, ETriggerEvent::Started, Weapons, &UFPSWeaponComponent::Reload);
		Input->BindAction(AimAction, ETriggerEvent::Started, Weapons, &UFPSWeaponComponent::AimOn);
		Input->BindAction(AimAction, ETriggerEvent::Completed, Weapons, &UFPSWeaponComponent::AimOff);
		Input->BindAction(AimAction, ETriggerEvent::Canceled, Weapons, &UFPSWeaponComponent::AimOff);
		Input->BindAction(InspectAction, ETriggerEvent::Started, Weapons, &UFPSWeaponComponent::Inspect);
		Input->BindAction(PrimaryAction, ETriggerEvent::Started, Weapons, &UFPSWeaponComponent::Primary);
		Input->BindAction(SecondaryAction, ETriggerEvent::Started, Weapons, &UFPSWeaponComponent::Secondary);
		Input->BindAction(DropAction, ETriggerEvent::Started, Weapons, &UFPSWeaponComponent::Drop);
		Input->BindAction(PickupAction, ETriggerEvent::Started, Weapons, &UFPSWeaponComponent::Pickup);
		Input->BindAction(SprintAction, ETriggerEvent::Started, this, &UFPSCharacterInput::SprintPressed);
		Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &UFPSCharacterInput::SprintReleased);
		Input->BindAction(SprintAction, ETriggerEvent::Canceled, this, &UFPSCharacterInput::SprintCanceled);
		Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &UFPSCharacterInput::CrouchPressed);
		Input->BindAction(CrouchAction, ETriggerEvent::Completed, this, &UFPSCharacterInput::CrouchReleased);
		Input->BindAction(CrouchAction, ETriggerEvent::Canceled, this, &UFPSCharacterInput::CrouchCanceled);
		Input->BindAction(HealAction, ETriggerEvent::Started, Character, &AFPSDemoCharacter::PressHeal);
		Input->BindAction(JumpAction, ETriggerEvent::Started, Character, &AFPSDemoCharacter::PressJump);
		Input->BindAction(JumpAction, ETriggerEvent::Completed, Character, &AFPSDemoCharacter::ReleaseJump);
		Input->BindAction(JumpAction, ETriggerEvent::Canceled, Character, &AFPSDemoCharacter::ReleaseJump);
	}
}
void UFPSCharacterInput::Register(AFPSDemoCharacter* Character)
{
    InputCharacter = Character;
	if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (RegisteredInputSubsystem.Get() == Subsystem) return;
			if (UEnhancedInputLocalPlayerSubsystem* PreviousSubsystem = RegisteredInputSubsystem.Get())
				PreviousSubsystem->RemoveMappingContext(InputMap);
			Subsystem->AddMappingContext(InputMap, 1);
			RegisteredInputSubsystem = Subsystem;
		}
	}
}


void UFPSCharacterInput::Unregister()
{
    if (auto* Character = InputCharacter.Get())
        if (auto* Movement = Cast<UFPSMovementComponent>(Character->GetCharacterMovement()); Movement && Movement->UsesMovementRules())
            Movement->ClearMovementIntents();
    if (auto* Subsystem = RegisteredInputSubsystem.Get()) Subsystem->RemoveMappingContext(InputMap);
    RegisteredInputSubsystem.Reset();
    InputCharacter.Reset();
}

void UFPSCharacterInput::SprintPressed()
{
    auto* Character = InputCharacter.Get();
    if (!Character) return;
    if (auto* PC = Cast<APlayerController>(Character->GetController()); PC && PC->IsMoveInputIgnored()) return;
    if (auto* Movement = Cast<UFPSMovementComponent>(Character->GetCharacterMovement()); Movement && Movement->UsesMovementRules())
        Movement->RequestSprint(UFPSInputSettings::Get()->bSprintToggle ? !Movement->HasSprintIntent() : true);
    else Character->SprintOn();
}
void UFPSCharacterInput::SprintReleased()
{
    auto* Character = InputCharacter.Get();
    if (!Character) return;
    if (auto* Movement = Cast<UFPSMovementComponent>(Character->GetCharacterMovement()); Movement && Movement->UsesMovementRules())
    {
        if (!UFPSInputSettings::Get()->bSprintToggle) Movement->RequestSprint(false);
    }
    else Character->SprintOff();
}
void UFPSCharacterInput::SprintCanceled()
{
    if (auto* Character = InputCharacter.Get())
    {
        if (auto* Movement = Cast<UFPSMovementComponent>(Character->GetCharacterMovement()); Movement && Movement->UsesMovementRules()) Movement->RequestSprint(false);
        else Character->SprintOff();
    }
}
void UFPSCharacterInput::CrouchPressed()
{
    auto* Character = InputCharacter.Get();
    if (!Character) return;
    if (auto* PC = Cast<APlayerController>(Character->GetController()); PC && PC->IsMoveInputIgnored()) return;
    if (auto* Movement = Cast<UFPSMovementComponent>(Character->GetCharacterMovement()); Movement && Movement->UsesMovementRules())
        Movement->RequestCrouch(UFPSInputSettings::Get()->bCrouchToggle ? !Character->bIsCrouched : true);
    else Character->CrouchOn();
}
void UFPSCharacterInput::CrouchReleased()
{
    auto* Character = InputCharacter.Get();
    if (!Character) return;
    if (auto* Movement = Cast<UFPSMovementComponent>(Character->GetCharacterMovement()); Movement && Movement->UsesMovementRules())
    {
        if (!UFPSInputSettings::Get()->bCrouchToggle) Movement->RequestCrouch(false);
    }
    else Character->CrouchOff();
}
void UFPSCharacterInput::CrouchCanceled()
{
    if (auto* Character = InputCharacter.Get())
    {
        if (auto* Movement = Cast<UFPSMovementComponent>(Character->GetCharacterMovement()); Movement && Movement->UsesMovementRules()) Movement->RequestCrouch(false);
        else Character->CrouchOff();
    }
}
