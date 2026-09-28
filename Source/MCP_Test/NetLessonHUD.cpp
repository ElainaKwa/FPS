#include "NetLessonHUD.h"

#include "NetLessonCharacter.h"
#include "NetLessonGameState.h"
#include "NetLessonPlayerController.h"
#include "NetLessonPlayerState.h"
#include "NetLessonStage.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

void ANetLessonHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas || !GetWorld()) return;
	DrawRect(FLinearColor(0.01f, 0.02f, 0.04f, 0.78f), 16.0f, 16.0f, 485.0f, 330.0f);
	const float X = 32.0f;
	float Y = 28.0f;
	auto Line = [this, X, &Y](const FString& Text, const FLinearColor& Color)
	{
		DrawText(Text, Color, X, Y, GEngine->GetSmallFont(), 1.1f, false);
		Y += 24.0f;
	};
	const ANetLessonCharacter* Pawn = Cast<ANetLessonCharacter>(GetOwningPlayerController()->GetPawn());
	const FString NetworkRole = Pawn && Pawn->HasAuthority() ? TEXT("SERVER / HOST") : TEXT("CLIENT");
	Line(TEXT("UE NETWORK BASICS  |  ") + NetworkRole, FLinearColor::White);
	Line(TEXT("A/D move, Space jump. Run PIE with 2 players."), FLinearColor::White);
	Line(TEXT("1: Client -> Server RPC -> replicated counter"), FLinearColor(0.0f, 1.0f, 1.0f));
	Line(TEXT("2: Client -> Server RPC -> multicast pulse"), FLinearColor::Yellow);
	Line(TEXT("3: Local-only effect (other window stays unchanged)"), FLinearColor::Green);
	const ANetLessonGameState* GameState = GetWorld()->GetGameState<ANetLessonGameState>();
	Line(FString::Printf(TEXT("GameMode: %s | GameState shared: %d"),
		GetWorld()->GetAuthGameMode() ? TEXT("server") : TEXT("absent"),
		GameState ? GameState->GetSharedRequestCount() : -1), FLinearColor::White);
	FString Players = TEXT("PlayerStates (all): ");
	if (GameState)
	{
		for (const APlayerState* BaseState : GameState->PlayerArray)
		{
			if (const ANetLessonPlayerState* State = Cast<ANetLessonPlayerState>(BaseState))
				Players += FString::Printf(TEXT("P%d=%d  "), State->GetPlayerId(), State->GetPersonalRequestCount());
		}
	}
	Line(Players, FLinearColor::White);
	Line(TEXT("Controller: server + owner | HUD: local"), FLinearColor::White);
	Line(TEXT("Pawn: all | GameInstance: local"), FLinearColor::White);
	for (TActorIterator<ANetLessonStage> It(GetWorld()); It; ++It)
	{
		Line(FString::Printf(TEXT("Replicated counter: %d"), It->GetCounter()), FLinearColor(0.0f, 1.0f, 1.0f));
		if (GetWorld()->GetTimeSeconds() < It->GetPulseUntil())
			Line(TEXT("MULTICAST EVENT RECEIVED"), FLinearColor::Red);
		break;
	}
	if (Pawn && GetWorld()->GetTimeSeconds() < Pawn->GetLocalEffectUntil())
		Line(TEXT("LOCAL ONLY: visible in this window"), FLinearColor::Green);
	if (const ANetLessonPlayerController* PC = Cast<ANetLessonPlayerController>(GetOwningPlayerController()))
	{
		if (GetWorld()->GetTimeSeconds() < PC->GetAckUntil())
			Line(FString::Printf(TEXT("OWNER-ONLY Client RPC ACK: %d"), PC->GetLastAckValue()), FLinearColor::Yellow);
	}
}
