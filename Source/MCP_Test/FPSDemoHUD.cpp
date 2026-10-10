#include "FPSDemoHUD.h"

#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerController.h"
#include "FPSDemoPlayerState.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponPickup.h"
#include "FPSModeRules.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

void AFPSDemoHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas || !GetWorld() || !GetOwningPlayerController()) return;
	const AFPSDemoCharacter* Pawn = Cast<AFPSDemoCharacter>(GetOwningPlayerController()->GetPawn());
	const AFPSDemoPlayerController* PC = Cast<AFPSDemoPlayerController>(GetOwningPlayerController());
	const AFPSDemoGameState* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
	if (GS && GS->bStructuredMatch)
	{
        const auto* Catalog = GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
        const FString Error = !GS->WeaponDataError.IsEmpty() ? GS->WeaponDataError : Catalog->ValidationError;
        if (!Error.IsEmpty())
        {
            DrawRect(FLinearColor(.05f,0,0,.95f), 10,10,Canvas->SizeX-20,Canvas->SizeY-20);
            DrawText(TEXT("WEAPON DATA INVALID - MATCH BLOCKED"), FLinearColor::Red, 30,30);
            DrawText(Error, FLinearColor::White, 30,65);
            return;
        }
		if (GS->bWaiting || GS->bFinished) return;
		const auto* PS = GetOwningPlayerController()->GetPlayerState<AFPSDemoPlayerState>();
		const bool bAlive = Pawn && !Pawn->IsEliminated();
		const float HealthRatio = Pawn ? FMath::Clamp(Pawn->GetHealth() / FMath::Max(Pawn->GetMaxHealth(), 1.f), 0.f, 1.f) : 0;
		const float Hurt = PC ? PC->GetDamageFlash() : 0;
		if (Hurt > 0) DrawRect(FLinearColor(.65f, .015f, .01f, Hurt * .16f), 0, 0, Canvas->SizeX, Canvas->SizeY);
		if (bAlive && HealthRatio <= .25f)
		{
			const FLinearColor Edge(.8f, .02f, .01f, .22f + .08f * FMath::Sin(GetWorld()->GetTimeSeconds() * 5));
			DrawRect(Edge, 0, 0, Canvas->SizeX, 9);
			DrawRect(Edge, 0, Canvas->SizeY - 9, Canvas->SizeX, 9);
			DrawRect(Edge, 0, 0, 9, Canvas->SizeY);
			DrawRect(Edge, Canvas->SizeX - 9, 0, 9, Canvas->SizeY);
		}
		const int32 Seconds = FMath::Max(0, FMath::CeilToInt(GS->RoundEndsAt - GS->GetServerWorldTimeSeconds()));
		const FString Timer = GS->bTraining ? TEXT("TRAINING / SOLO") : FString::Printf(TEXT("%02d:%02d"), Seconds / 60, Seconds % 60);
		DrawRect(FLinearColor(0.01f, .02f, .04f, .8f), 20, 20, 320, 110);
		DrawText(Timer, !GS->bTraining && Seconds <= 30 ? FLinearColor::Yellow : FLinearColor::White, 35, 30, GEngine->GetMediumFont(), 1.3f);
		DrawText(FString::Printf(TEXT("%s TEAM   HP %.0f   SCORE %d"), PS && PS->TeamId == 0 ? TEXT("BLUE") : TEXT("RED"), Pawn ? Pawn->GetHealth() : 0, PS ? PS->GetEliminationCount() : 0), FLinearColor::White, 35, 68);
		DrawRect(FLinearColor(.12f, .14f, .18f), 35, 90, 285, 6);
		DrawRect(HealthRatio <= .25f ? FLinearColor::Red : FLinearColor(.15f,.8f,.55f), 35, 90, 285 * HealthRatio, 6);
		DrawText(TEXT("WASD / FIRE: LMB / JUMP: SPACE / MENU: ESC"), FLinearColor::White, 35, 105);
		const float X = Canvas->SizeX * .5f, Y0 = Canvas->SizeY * .5f;
		if (bAlive && (!Pawn->GetWeapons() || !Pawn->GetWeapons()->GetLoadout().bAiming))
		{
			DrawLine(X - 10, Y0, X - 4, Y0, FLinearColor::White);
			DrawLine(X + 4, Y0, X + 10, Y0, FLinearColor::White);
			DrawLine(X, Y0 - 10, X, Y0 - 4, FLinearColor::White);
			DrawLine(X, Y0 + 4, X, Y0 + 10, FLinearColor::White);
		}
		if (PC && GetWorld()->GetTimeSeconds() < PC->GetHitMarkerUntil())
		{
			DrawLine(X - 12, Y0 - 12, X - 5, Y0 - 5, FLinearColor::Red, 2);
			DrawLine(X + 12, Y0 + 12, X + 5, Y0 + 5, FLinearColor::Red, 2);
			DrawLine(X - 12, Y0 + 12, X - 5, Y0 + 5, FLinearColor::Red, 2);
			DrawLine(X + 12, Y0 - 12, X + 5, Y0 - 5, FLinearColor::Red, 2);
		}
		if (PC && GetWorld()->GetTimeSeconds() < PC->GetKillMarkerUntil())
		{
			DrawRect(FLinearColor(.01f, .02f, .04f, .85f), X - 100, Canvas->SizeY * .78f, 200, 42);
			DrawText(TEXT("[ X ]  ELIMINATION +1"), FLinearColor::Yellow, X - 90, Canvas->SizeY * .78f + 12);
		}
		if (Pawn && Pawn->IsEliminated())
		{
			DrawRect(FLinearColor(.02f,.025f,.04f,.9f), X - 230, Y0 - 45, 460, 115);
			DrawText(TEXT("ELIMINATED"), FLinearColor::Red, X - 210, Y0 - 33, GEngine->GetMediumFont(), 1.35f);
			DrawText(FString::Printf(TEXT("BY %s"), Pawn->GetKillerName().IsEmpty() ? TEXT("...") : *Pawn->GetKillerName()), FLinearColor::White, X - 210, Y0);
			const float Remaining = Pawn->GetRespawnSeconds();
			DrawText(Remaining > 0 ? FString::Printf(TEXT("RESPAWN IN %.1f s"), Remaining) : TEXT("RESPAWNING..."), FLinearColor::Yellow, X - 210, Y0 + 27);
			DrawRect(FLinearColor(.15f,.7f,1.f), X - 230, Y0 + 65, 460 * FMath::Clamp(1.f - Remaining / AFPSDemoCharacter::RespawnDelay, 0.f, 1.f), 5);
		}
		if (bAlive && Pawn->GetWeapons())
		{
			const auto* Weapons = Pawn->GetWeapons();
			const auto* Def = Weapons->Definition();
			const auto& Gun = Weapons->Equipped();
			const auto& State = Weapons->GetLoadout();
			const TCHAR* Action = State.Action == EFPSWeaponAction::Reloading ? TEXT("RELOADING") : State.Action == EFPSWeaponAction::Switching ? TEXT("SWITCHING") : State.Action == EFPSWeaponAction::Inspecting ? TEXT("INSPECTING") : TEXT("");
			DrawRect(FLinearColor(.01f, .02f, .04f, .85f), Canvas->SizeX - 325, Canvas->SizeY - 125, 305, 105);
			DrawText(FString::Printf(TEXT("%s   %d / %d"), Def ? *Def->DisplayName.ToString() : TEXT("UNARMED"), Gun.Magazine, Gun.Reserve), FLinearColor::White, Canvas->SizeX - 310, Canvas->SizeY - 112, GEngine->GetMediumFont(), 1.2f);
			const bool bLowAmmo = Def && Gun.Magazine <= FMath::Max(1, Def->MagazineSize / 5);
			DrawText(State.Action == EFPSWeaponAction::Idle && bLowAmmo ? (Gun.Reserve > 0 ? TEXT("LOW AMMO - PRESS R") : TEXT("LOW AMMO - SWITCH / PICK UP")) : Action, FLinearColor::Yellow, Canvas->SizeX - 310, Canvas->SizeY - 78);
			if (Def && State.Action == EFPSWeaponAction::Reloading)
			{
				const float Progress = FMath::Clamp(1.0 - (State.ActionEndsAt - GS->GetServerWorldTimeSeconds()) / FMath::Max(Def->ReloadDuration, .01f), 0., 1.);
				DrawRect(FLinearColor(.1f,.2f,.3f), Canvas->SizeX - 310, Canvas->SizeY - 62, 275, 3);
				DrawRect(FLinearColor(.15f,.7f,1.f), Canvas->SizeX - 310, Canvas->SizeY - 62, 275 * Progress, 3);
			}
			DrawText(TEXT("R reload | RMB aim | Q inspect"), FLinearColor::White, Canvas->SizeX - 310, Canvas->SizeY - 55);
			DrawText(TEXT("1/2 switch | G drop | F pickup"), FLinearColor::White, Canvas->SizeX - 310, Canvas->SizeY - 35);
			DrawText(TEXT("SHIFT sprint | CTRL crouch | P loadout"), FLinearColor::White, 35, Canvas->SizeY - 40);
            if (const auto* Rules = UFPSModeRules::ForWorld(GetWorld()))
                DrawText(FString::Printf(TEXT("Drops: max %d | %.0fs | oldest recycled"), Rules->DynamicDropLimit, Rules->DynamicDropLifetime), FLinearColor(.7f,.8f,.9f), 35, Canvas->SizeY - 60);
			if (const auto* Pickup = Weapons->FindPickup())
                if (const auto* PickupDef = Weapons->Catalog()->Find(Pickup->GetInstance().WeaponId))
					DrawText(FString::Printf(TEXT("[F] %s  %d / %d"), *PickupDef->DisplayName.ToString(), Pickup->GetInstance().Magazine, Pickup->GetInstance().Reserve), FLinearColor::Yellow, X - 100, Y0 + 60);
		}
		return;
	}

	DrawRect(FLinearColor(0.005f, 0.015f, 0.035f, 0.80f), 10, 10, 231, 184);
	float Y = 17;
	auto Line = [this, &Y](const FString& Value, const FLinearColor& Color)
	{
		DrawText(Value, Color, 18, Y, GEngine->GetSmallFont(), 0.76f, false);
		Y += 17;
	};
	Line(FString::Printf(TEXT("NET FPS  |  %s"), Pawn && Pawn->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT")), FLinearColor::White);
	Line(TEXT("WASD / Mouse fire / E heal / Space"), FLinearColor::White);
	Line(TEXT("LOCAL: instant recoil"), FLinearColor(0.25f, 0.9f, 1.0f));
	Line(TEXT("SERVER: trace + GAS effect"), FLinearColor::Yellow);
	Line(TEXT("ALL: Health / scores / respawn"), FLinearColor::Green);
	const float Health = Pawn ? Pawn->GetHealth() : 0.0f;
	const float MaxHealth = Pawn ? Pawn->GetMaxHealth() : 100.0f;
	Line(FString::Printf(TEXT("GAS HEALTH: %.0f / %.0f"), Health, MaxHealth), FLinearColor::White);
	DrawRect(FLinearColor(0.2f, 0.2f, 0.2f), 18, Y, 190, 10);
	DrawRect(FLinearColor::Green, 18, Y, 190 * FMath::Clamp(Health / FMath::Max(MaxHealth, 1.0f), 0.0f, 1.0f), 10);
	Y += 13;
	Line(FString::Printf(TEXT("GAME STATE: %d eliminations"), GS ? GS->GetEliminations() : 0), FLinearColor::White);
	if (GS)
	{
		FString Scores = TEXT("PLAYER STATES: ");
		for (const APlayerState* Base : GS->PlayerArray)
		{
			if (const AFPSDemoPlayerState* PS = Cast<AFPSDemoPlayerState>(Base))
				Scores += FString::Printf(TEXT("P%d %d/%d  "), PS->GetPlayerId(), PS->GetEliminationCount(), PS->GetDeathCount());
		}
		Line(Scores, FLinearColor::White);
		if (!GS->GetLastEvent().IsEmpty()) Line(GS->GetLastEvent(), FLinearColor::Yellow);
	}
	if (Pawn && Pawn->IsEliminated()) Line(TEXT("ELIMINATED - server respawning..."), FLinearColor::Red);

	const float CX = Canvas->SizeX * 0.5f;
	const float CY = Canvas->SizeY * 0.5f;
	const bool bHitMarker = PC && GetWorld()->GetTimeSeconds() < PC->GetHitMarkerUntil();
	const FLinearColor CrossColor = bHitMarker ? FLinearColor::Red : FLinearColor::White;
	DrawLine(CX - 15, CY, CX - 5, CY, CrossColor, 2);
	DrawLine(CX + 5, CY, CX + 15, CY, CrossColor, 2);
	DrawLine(CX, CY - 15, CX, CY - 5, CrossColor, 2);
	DrawLine(CX, CY + 5, CX, CY + 15, CrossColor, 2);
	if (bHitMarker)
		DrawText(PC->WasElimination() ? TEXT("ELIMINATION") : TEXT("SERVER CONFIRMED HIT"),
			FLinearColor::Red, CX - 90, CY + 28, GEngine->GetSmallFont(), 1.2f, false);
}
