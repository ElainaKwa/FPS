#include "FPSDemoPlayerController.h"
#include "FPSSettingsUI.h"
#include "FPSSystemSettings.h"
#include "FPSSettingsSmoke.h"
#include "FPSAudioSubsystem.h"
#include "FPSDemoCharacter.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void AFPSDemoPlayerController::UseGameplayInput()
{
    bShowMouseCursor=false;
    SetInputMode(FInputModeGameOnly());
}
void AFPSDemoPlayerController::UseMenuInput(UUserWidget* Focus)
{
    bShowMouseCursor=true;
    FInputModeGameAndUI Mode;
    if(Focus)Mode.SetWidgetToFocus(Focus->TakeWidget());
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(LockMenuMouseToViewport()?EMouseLockMode::LockAlways:EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
}

void AFPSDemoPlayerController::OpenSettings(UUserWidget* Source)
{
    if(!IsLocalController()||SettingsUI)return;
    SettingsSource=Source;bSettingsWasBlocked=IsMoveInputIgnored();
    SettingsUI=UFPSSettingsUI::Open(this);if(!SettingsUI)return;
    if(auto* Body=Cast<AFPSDemoCharacter>(GetPawn()))Body->StopCombatInput();
    ResetIgnoreMoveInput();ResetIgnoreLookInput();SetIgnoreMoveInput(true);SetIgnoreLookInput(true);bShowMouseCursor=true;
    SettingsUI->OnClosed.BindWeakLambda(this,[this]()
    {
        SettingsUI=nullptr;ResetIgnoreMoveInput();ResetIgnoreLookInput();
        SetIgnoreMoveInput(bSettingsWasBlocked);SetIgnoreLookInput(bSettingsWasBlocked);bShowMouseCursor=bSettingsWasBlocked;
        if(bSettingsWasBlocked)UseMenuInput(SettingsSource.Get());
        else UseGameplayInput();SettingsSource.Reset();
    });
    UseMenuInput(SettingsUI);
}
void AFPSDemoPlayerController::SettingsBack(){if(SettingsUI)SettingsUI->RequestBack();}
void AFPSDemoPlayerController::CloseSettingsForTravel()
{
    if(!SettingsUI)return;if(auto* S=UFPSSystemSettings::Get(this)){S->CancelCapture();S->DiscardGraphics();}
    SettingsUI->OnClosed.Unbind();SettingsUI->RemoveFromParent();SettingsUI=nullptr;SettingsSource.Reset();
}
void AFPSDemoPlayerController::EndPlay(const EEndPlayReason::Type Reason){CloseSettingsForTravel();Super::EndPlay(Reason);}

void AFPSDemoPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
    FFPSSettingsSmoke::Tick(this);
	if (!IsLocalController()) return;
	auto* Avatar = Cast<AFPSDemoCharacter>(GetPawn());
	if (ObservedPawn.Get() != Avatar)
	{
		ObservedPawn = Avatar;
		PreviousHealth = Avatar ? Avatar->GetHealth() : 0.f;
		DamageFlashUntil = 0;
		bReportedDeath = false;
		if (Avatar && !Avatar->IsEliminated() && bAwaitingRespawn)
		{
#if !UE_BUILD_SHIPPING
			if (FParse::Param(FCommandLine::Get(), TEXT("FPSFlowSmoke")))
				UE_LOG(LogTemp, Display, TEXT("FPS_FEEDBACK PASS respawn-reset %s"), HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
#endif
			bAwaitingRespawn = false;
		}
	}
	if (!Avatar) return;
	const float Health = Avatar->GetHealth();
	if (Health < PreviousHealth - .01f)
	{
		DamageFlashUntil = GetWorld()->GetTimeSeconds() + .55f;
#if !UE_BUILD_SHIPPING
		if (FParse::Param(FCommandLine::Get(), TEXT("FPSFlowSmoke")))
			UE_LOG(LogTemp, Display, TEXT("FPS_FEEDBACK PASS damage-flash %s health=%.0f"), HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), Health);
		if (FParse::Param(FCommandLine::Get(), TEXT("FPSFeedbackCapture")) && Health > 0)
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/WindowsEditor/FeedbackDamage.png"), false, false);
#endif
	}
	PreviousHealth = Health;
	if (Avatar->IsEliminated()) bAwaitingRespawn = true;
#if !UE_BUILD_SHIPPING
	if (!bCapturedDeath && Avatar->IsEliminated() && Avatar->GetRespawnSeconds() > 0 && Avatar->GetRespawnSeconds() < 1.8f && FParse::Param(FCommandLine::Get(), TEXT("FPSFeedbackCapture")))
	{
		bCapturedDeath = true;
		FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/WindowsEditor/FeedbackDeath.png"), false, false);
	}
	if (!bReportedDeath && Avatar->IsEliminated() && !Avatar->GetKillerName().IsEmpty() && Avatar->GetRespawnSeconds() > 0)
	{
		bReportedDeath = true;
		if (FParse::Param(FCommandLine::Get(), TEXT("FPSFlowSmoke")))
			UE_LOG(LogTemp, Display, TEXT("FPS_FEEDBACK PASS death-info %s killer=%s remaining=%.2f"), HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), *Avatar->GetKillerName(), Avatar->GetRespawnSeconds());
	}
#endif
}

float AFPSDemoPlayerController::GetDamageFlash() const
{
	return FMath::Clamp((DamageFlashUntil - GetWorld()->GetTimeSeconds()) / .55f, 0.f, 1.f);
}

void AFPSDemoPlayerController::ConfirmHit(bool bElimination, bool bHeadshot)
{
	if (HasAuthority()) ClientConfirmHit(bElimination, bHeadshot);
}

void AFPSDemoPlayerController::ClientConfirmHit_Implementation(bool bElimination, bool bHeadshot)
{
    bLastHitHeadshot = bHeadshot;
	bLastHitEliminated = bElimination;
	HitMarkerUntil = GetWorld()->GetTimeSeconds() + (bElimination ? 1.6f : 0.35f);
	if (bElimination) KillMarkerUntil = GetWorld()->GetTimeSeconds() + 1.6f;
	if (IsLocalController())
		if (auto* Audio = UFPSAudioSubsystem::Get(this)) Audio->Play(bElimination ? TEXT("Kill") : TEXT("Hit"));
}
