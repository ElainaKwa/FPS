#include "NetLessonCharacter.h"

#include "NetLessonStage.h"
#include "NetLessonGameState.h"
#include "NetLessonPlayerController.h"
#include "NetLessonPlayerState.h"
#include "EnhancedInputComponent.h"
#include "EngineUtils.h"
#include "InputAction.h"
#include "InputMappingContext.h"

ANetLessonCharacter::ANetLessonCharacter()
{
	bReplicates = true;
	CounterAction = CreateDefaultSubobject<UInputAction>(TEXT("CounterAction"));
	PulseAction = CreateDefaultSubobject<UInputAction>(TEXT("PulseAction"));
	LocalAction = CreateDefaultSubobject<UInputAction>(TEXT("LocalAction"));
	CounterAction->ValueType = EInputActionValueType::Boolean;
	PulseAction->ValueType = EInputActionValueType::Boolean;
	LocalAction->ValueType = EInputActionValueType::Boolean;
	DefaultMappingContext->MapKey(CounterAction, EKeys::One);
	DefaultMappingContext->MapKey(PulseAction, EKeys::Two);
	DefaultMappingContext->MapKey(LocalAction, EKeys::Three);
}

void ANetLessonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(CounterAction, ETriggerEvent::Started, this, &ANetLessonCharacter::PressCounter);
		Input->BindAction(PulseAction, ETriggerEvent::Started, this, &ANetLessonCharacter::PressPulse);
		Input->BindAction(LocalAction, ETriggerEvent::Started, this, &ANetLessonCharacter::PressLocal);
	}
}

ANetLessonStage* ANetLessonCharacter::FindStage() const
{
	for (TActorIterator<ANetLessonStage> It(GetWorld()); It; ++It) return *It;
	return nullptr;
}

void ANetLessonCharacter::PressCounter()
{
	ServerRequestCounter();
}

void ANetLessonCharacter::PressPulse()
{
	ServerRequestPulse();
}

void ANetLessonCharacter::PressLocal()
{
	LocalEffectUntil = GetWorld()->GetTimeSeconds() + 1.5f;
}

void ANetLessonCharacter::ServerRequestCounter_Implementation()
{
	if (ANetLessonStage* Stage = FindStage())
	{
		Stage->ServerIncrementCounter();
		if (ANetLessonGameState* State = GetWorld()->GetGameState<ANetLessonGameState>())
			State->RecordSharedRequest();
		if (ANetLessonPlayerState* State = Cast<ANetLessonPlayerState>(GetPlayerState()))
			State->RecordPersonalRequest();
		if (ANetLessonPlayerController* PC = Cast<ANetLessonPlayerController>(GetController()))
			PC->AcknowledgeCounterRequest(Stage->GetCounter());
	}
}

void ANetLessonCharacter::ServerRequestPulse_Implementation()
{
	if (ANetLessonStage* Stage = FindStage()) Stage->ServerBroadcastPulse();
}
