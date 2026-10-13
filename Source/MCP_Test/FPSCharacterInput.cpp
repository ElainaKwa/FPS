#include "FPSCharacterInput.h"
#include "FPSDemoCharacter.h"
#include "FPSWeaponComponent.h"
#include "FPSMovementComponent.h"
#include "FPSInputSettings.h"
#include "FPSSystemSettings.h"
#include "FPSExtractionMode.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
UFPSCharacterInput::UFPSCharacterInput()
{
	PrimaryComponentTick.bCanEverTick=true;
	InputMap = CreateDefaultSubobject<UInputMappingContext>(TEXT("FPSInputMap"));
	ForwardAction = CreateDefaultSubobject<UInputAction>(TEXT("ForwardAction"));
	RightAction = CreateDefaultSubobject<UInputAction>(TEXT("RightAction"));
	YawAction = CreateDefaultSubobject<UInputAction>(TEXT("YawAction"));
	PitchAction = CreateDefaultSubobject<UInputAction>(TEXT("PitchAction"));
	FireAction = CreateDefaultSubobject<UInputAction>(TEXT("FireAction"));
	JumpAction = CreateDefaultSubobject<UInputAction>(TEXT("FPSJumpAction"));
	HealAction = CreateDefaultSubobject<UInputAction>(TEXT("HealAction"));
	for (UInputAction* Action : {YawAction.Get(), PitchAction.Get()})
		Action->ValueType = EInputActionValueType::Axis1D;
	for (UInputAction* Action : {ForwardAction.Get(), RightAction.Get(), FireAction.Get(), JumpAction.Get(), HealAction.Get()})
		Action->ValueType = EInputActionValueType::Boolean;
	BackwardAction=CreateDefaultSubobject<UInputAction>(TEXT("BackwardAction"));
	LeftAction=CreateDefaultSubobject<UInputAction>(TEXT("LeftAction"));
	InputMap->MapKey(ForwardAction, EKeys::W);
	InputMap->MapKey(BackwardAction, EKeys::S);
	InputMap->MapKey(RightAction, EKeys::D);
	InputMap->MapKey(LeftAction, EKeys::A);
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
	NextWeaponAction = Action(TEXT("NextWeapon"), EKeys::MouseScrollUp);
}
void UFPSCharacterInput::Bind(AFPSDemoCharacter* Character, UInputComponent* PlayerInputComponent)
{
    InputCharacter = Character;
    auto* Weapons = Character->GetWeapons();
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(ForwardAction, ETriggerEvent::Triggered, this, &UFPSCharacterInput::Forward);
		Input->BindAction(BackwardAction, ETriggerEvent::Triggered, this, &UFPSCharacterInput::Backward);
		Input->BindAction(RightAction, ETriggerEvent::Triggered, this, &UFPSCharacterInput::Right);
		Input->BindAction(LeftAction, ETriggerEvent::Triggered, this, &UFPSCharacterInput::Left);
		Input->BindAction(YawAction, ETriggerEvent::Triggered, Character, &AFPSDemoCharacter::LookYaw);
		Input->BindAction(PitchAction, ETriggerEvent::Triggered, Character, &AFPSDemoCharacter::LookPitch);
		Input->BindAction(FireAction, ETriggerEvent::Started, Character, &AFPSDemoCharacter::PressFire);
		Input->BindAction(FireAction, ETriggerEvent::Completed, Character, &AFPSDemoCharacter::ReleaseFire);
		Input->BindAction(FireAction, ETriggerEvent::Canceled, Character, &AFPSDemoCharacter::ReleaseFire);
		Input->BindAction(ReloadAction, ETriggerEvent::Started, Weapons, &UFPSWeaponComponent::Reload);
		Input->BindAction(AimAction, ETriggerEvent::Started, this, &UFPSCharacterInput::AimPressed);
		Input->BindAction(AimAction, ETriggerEvent::Completed, this, &UFPSCharacterInput::AimReleased);
		Input->BindAction(AimAction, ETriggerEvent::Canceled, this, &UFPSCharacterInput::AimCanceled);
		Input->BindAction(InspectAction, ETriggerEvent::Started, Weapons, &UFPSWeaponComponent::Inspect);
		Input->BindAction(PrimaryAction, ETriggerEvent::Started, Weapons, &UFPSWeaponComponent::Primary);
		Input->BindAction(SecondaryAction, ETriggerEvent::Started, Weapons, &UFPSWeaponComponent::Secondary);
		Input->BindAction(DropAction, ETriggerEvent::Started, Weapons, &UFPSWeaponComponent::Drop);
		Input->BindAction(PickupAction, ETriggerEvent::Started, this, &UFPSCharacterInput::Interact);
		Input->BindAction(NextWeaponAction, ETriggerEvent::Started, this, &UFPSCharacterInput::NextWeapon);
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
			RegisteredInputSubsystem = Subsystem;
			RebuildMappings();
		}
	}
}

void UFPSCharacterInput::RebuildMappings()
{
    auto* S=UFPSSystemSettings::Get(this);auto* Subsystem=RegisteredInputSubsystem.Get();if(!Subsystem)return;
    Subsystem->RemoveMappingContext(InputMap);
    if(S)
    {
        InputMap->UnmapAll();SettingsRevision=S->BindingRevision();
        auto Map=[&](FName Id,UInputAction* Action,bool Negate=false)
        {if(const auto* B=S->Binding(Id))for(FKey Key:{B->Primary,B->Secondary})if(Key.IsValid())
            {auto& M=InputMap->MapKey(Action,Key);if(Negate)M.Modifiers.Add(NewObject<UInputModifierNegate>(InputMap));}};
        // Separate digital actions also get Enhanced Input's release-before-reuse guard (axis actions do not).
        Map("Forward",ForwardAction);Map("Backward",BackwardAction);Map("Right",RightAction);Map("Left",LeftAction);
        Map("Fire",FireAction);Map("Jump",JumpAction);Map("Reload",ReloadAction);Map("Aim",AimAction);Map("Inspect",InspectAction);
        Map("Primary",PrimaryAction);Map("Secondary",SecondaryAction);Map("NextWeapon",NextWeaponAction);Map("Drop",DropAction);Map("Interact",PickupAction);Map("Sprint",SprintAction);Map("Crouch",CrouchAction);
        InputMap->MapKey(YawAction,EKeys::MouseX);InputMap->MapKey(PitchAction,EKeys::MouseY);
        // Teaching-only heal verb is not a hidden extra gameplay binding in structured FPS modes.
        if(auto* C=InputCharacter.Get();C&&!C->GetWeapons()->IsEnabled())InputMap->MapKey(HealAction,EKeys::E);
    }
    if(!bSuspended){FModifyContextOptions O;O.bForceImmediately=true;O.bIgnoreAllPressedKeysUntilRelease=true;Subsystem->AddMappingContext(InputMap,1,O);}
}
void UFPSCharacterInput::SuspendForMenu()
{
    bSuspended=true;
    if(auto* S=RegisteredInputSubsystem.Get()){FModifyContextOptions O;O.bForceImmediately=true;S->RemoveMappingContext(InputMap,O);}
}
void UFPSCharacterInput::TickComponent(float DT,ELevelTick Type,FActorComponentTickFunction* TF)
{
    Super::TickComponent(DT,Type,TF);auto* C=InputCharacter.Get();if(!C||!C->IsLocallyControlled())return;
    auto* PC=Cast<APlayerController>(C->GetController());const bool Block=PC&&PC->IsMoveInputIgnored();
    C->GetWeapons()->SetMenuInputBlocked(Block);
    if(Block&&!bSuspended){C->StopCombatInput();SuspendForMenu();}
    if(!Block&&bSuspended){bSuspended=false;RebuildMappings();}
    if(const auto* S=UFPSSystemSettings::Get(this);S&&SettingsRevision!=S->BindingRevision())RebuildMappings();
}
void UFPSCharacterInput::AimPressed()
{if(auto* C=InputCharacter.Get()){auto* W=C->GetWeapons();const auto* S=UFPSSystemSettings::Get(this);if(S&&S->AimToggle&&W->GetLoadout().bAiming)W->AimOff();else W->AimOn();}}
void UFPSCharacterInput::Forward(){if(auto* C=InputCharacter.Get())C->MoveForward(FInputActionValue(1.f));}
void UFPSCharacterInput::Backward(){if(auto* C=InputCharacter.Get())C->MoveForward(FInputActionValue(-1.f));}
void UFPSCharacterInput::Right(){if(auto* C=InputCharacter.Get())C->MoveRight(FInputActionValue(1.f));}
void UFPSCharacterInput::Left(){if(auto* C=InputCharacter.Get())C->MoveRight(FInputActionValue(-1.f));}
void UFPSCharacterInput::AimReleased(){if(const auto* S=UFPSSystemSettings::Get(this);!S||!S->AimToggle)AimCanceled();}
void UFPSCharacterInput::AimCanceled(){if(auto* C=InputCharacter.Get())C->GetWeapons()->AimOff();}
void UFPSCharacterInput::NextWeapon(){if(auto* C=InputCharacter.Get()){auto* W=C->GetWeapons();if(W->GetLoadout().ActiveSlot==EFPSWeaponSlot::Primary)W->Secondary();else W->Primary();}}
void UFPSCharacterInput::Interact(){if(auto* C=InputCharacter.Get()){if(auto* PC=Cast<AFPSExtractionController>(C->GetController()))PC->Interact();else C->GetWeapons()->Pickup();}}


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
