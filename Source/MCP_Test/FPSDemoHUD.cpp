#include "FPSDemoHUD.h"
#include "FPSMovementComponent.h"
#include "FPSUIStyle.h"
#include "FPSInterface.h"
#include "FPSBaseMembership.h"
#include "CanvasItem.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "UObject/ConstructorHelpers.h"

#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerController.h"
#include "FPSDemoPlayerState.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponPickup.h"
#include "FPSModeRules.h"
#include "FPSMatchFlow.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

AFPSDemoHUD::AFPSDemoHUD()
{
    static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    RuntimeFont=Font.Object;
    static ConstructorHelpers::FObjectFinder<UFont> MovementText(TEXT("/Game/FPSDemo/Art/Industrial/UI/Fonts/F_UI_Text.F_UI_Text"));
    MovementFont=MovementText.Object;
}

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
        if(!HUDAsset){HUDAsset=UFPSInterface::Open(GetOwningPlayerController(),TEXT("HUD"));if(HUDAsset)HUDAsset->AddToPlayerScreen(0);}
        if (GS->bWaiting || GS->bFinished) return;
        const bool Alive=Pawn && !Pawn->IsEliminated();
        const float Hurt=PC ? PC->GetDamageFlash() : 0;
        if(Hurt>0)DrawRect(FLinearColor(.65f,.015f,.01f,Hurt*.16f),0,0,Canvas->SizeX,Canvas->SizeY);
        if(Alive && Pawn->GetHealth()/FMath::Max(Pawn->GetMaxHealth(),1.f)<=.25f)
        {
            const FLinearColor Edge(.8f,.02f,.01f,.22f+.08f*FMath::Sin(GetWorld()->GetTimeSeconds()*5));
            DrawRect(Edge,0,0,Canvas->SizeX,9);DrawRect(Edge,0,Canvas->SizeY-9,Canvas->SizeX,9);DrawRect(Edge,0,0,9,Canvas->SizeY);DrawRect(Edge,Canvas->SizeX-9,0,9,Canvas->SizeY);
        }
        const float X=Canvas->SizeX*.5f,Y=Canvas->SizeY*.5f;
        if(Alive && !Pawn->GetWeapons()->GetLoadout().bAiming)
        {DrawLine(X-12,Y,X-5,Y,FPSUI::Paper,1.5f);DrawLine(X+5,Y,X+12,Y,FPSUI::Paper,1.5f);DrawLine(X,Y-12,X,Y-5,FPSUI::Paper,1.5f);DrawLine(X,Y+5,X,Y+12,FPSUI::Paper,1.5f);}
        if(PC && GetWorld()->GetTimeSeconds()<PC->GetHitMarkerUntil())
            for(int32 DX:{-1,1})for(int32 DY:{-1,1})DrawLine(X+DX*6,Y+DY*6,X+DX*13,Y+DY*13,FLinearColor::Red,2);
        if(!HUDAsset||!HUDAsset->HasControl(TEXT("StaminaPanel")))DrawMovementHUD(Pawn);

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

void AFPSDemoHUD::DrawMovementHUD(const AFPSDemoCharacter* Pawn)
{
    const auto* GS=GetWorld()->GetGameState<AFPSDemoGameState>();
    const auto* Front=Cast<AFPSFrontController>(GetOwningPlayerController());
    if(!GS||!GS->bStructuredMatch||GS->bWaiting||GS->bFinished||(Front&&Front->IsLoadoutOpen())){StaminaPresentation.Initialized=false;return;}
    const auto View=UFPSInterface::ReadStaminaHUD(Pawn);
    const auto* Movement=Pawn?Cast<UFPSMovementComponent>(Pawn->GetCharacterMovement()):nullptr;
    if(!Pawn||Pawn->IsEliminated()||!Movement||!Movement->UsesStamina()){StaminaPresentation.Initialized=false;return;}
    if(StaminaOwner.Get()!=Pawn){StaminaPresentation.Reset(View.Ratio);StaminaOwner=Pawn;}
    StaminaPresentation.Update(View.Ratio,GetWorld()->GetDeltaSeconds());
    if(!View.Visible&&StaminaPresentation.IsSettled())return;
    const float Scale=FMath::Min(Canvas->SizeX/1600.f,Canvas->SizeY/900.f);
    const FVector2D Offset((Canvas->SizeX-1600.f*Scale)*.5f,(Canvas->SizeY-900.f*Scale)*.5f);
    // Match the saved HUD's 1600x900 design layout.
    const float X=Offset.X+672.5f*Scale,Y=Offset.Y+740.f*Scale;
    DrawRect(FLinearColor(0,0,0,.28f),X,Y,255*Scale,9*Scale);
    DrawRect(FLinearColor(.45f,.45f,.45f,1),X,Y,255*Scale*StaminaPresentation.Ghost(),9*Scale);
    DrawRect(FLinearColor::White,X,Y,255*Scale*StaminaPresentation.White(),9*Scale);
}
