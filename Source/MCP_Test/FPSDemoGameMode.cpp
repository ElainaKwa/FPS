#include "FPSDemoGameMode.h"

#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoHUD.h"
#include "FPSDemoPlayerController.h"
#include "FPSDemoPlayerState.h"
#include "FPSMatchFlow.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AFPSDemoGameMode::AFPSDemoGameMode()
{
	DefaultPawnClass = AFPSDemoCharacter::StaticClass();
	static ConstructorHelpers::FClassFinder<AFPSDemoCharacter> SavedCharacter(TEXT("/Game/FPSDemo/Blueprints/BP_FPS_Character"));
	if (SavedCharacter.Succeeded()) DefaultPawnClass = SavedCharacter.Class;
	GameStateClass = AFPSDemoGameState::StaticClass();
	PlayerStateClass = AFPSDemoPlayerState::StaticClass();
	PlayerControllerClass = AFPSDemoPlayerController::StaticClass();
	HUDClass = AFPSDemoHUD::StaticClass();
}

void AFPSDemoGameMode::HandleElimination(AFPSDemoCharacter* Victim, AController* Attacker)
{
	if (!HasAuthority() || !IsValid(Victim) || Victim->IsEliminated()) return;
	if (const AFPSMatchMode* Rules = Cast<AFPSMatchMode>(this))
		if (!Rules->IsCombatActive()) return;
	AController* VictimController = Victim->GetController();
	AFPSDemoPlayerState* VictimState = VictimController ? VictimController->GetPlayerState<AFPSDemoPlayerState>() : nullptr;
	AFPSDemoPlayerState* AttackerState = Attacker ? Attacker->GetPlayerState<AFPSDemoPlayerState>() : nullptr;
	if (VictimState) VictimState->AddDeath();
	if (AttackerState && Attacker != VictimController) AttackerState->AddElimination();
	if (AFPSDemoGameState* State = GetGameState<AFPSDemoGameState>())
		State->RecordElimination(AttackerState ? FString::Printf(TEXT("P%d"), AttackerState->GetPlayerId()) : TEXT("World"),
			VictimState ? FString::Printf(TEXT("P%d"), VictimState->GetPlayerId()) : TEXT("Player"));
	Victim->MarkEliminated(AttackerState ? AttackerState->GetPlayerName() : TEXT("World"));

	TWeakObjectPtr<AFPSDemoCharacter> WeakVictim(Victim);
	TWeakObjectPtr<AController> WeakController(VictimController);
	FTimerHandle RespawnTimer;
	GetWorldTimerManager().SetTimer(RespawnTimer, [this, WeakVictim, WeakController]()
	{
		// Keep the final view stable if the round ended during the respawn wait.
		if (const AFPSMatchMode* Rules = Cast<AFPSMatchMode>(this))
			if (!Rules->IsCombatActive()) return;
		if (WeakVictim.IsValid()) WeakVictim->Destroy();
		if (WeakController.IsValid()) RestartPlayer(WeakController.Get());
	}, AFPSDemoCharacter::RespawnDelay, false);
}
