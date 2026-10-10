#include "FPSMatchFlow.h"
#include "FPSWeaponData.h"
#include "FPSDemoGameState.h"
#include "FPSFrontGameInstance.h"
#include "GameFramework/GameSession.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

void AFPSFrontController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AFPSFrontController, bWeaponContentVerified, COND_OwnerOnly);
}

void AFPSFrontController::BeginWeaponContentCheck()
{
    if (!HasAuthority()) return;
    const auto* Catalog = GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    if (!Catalog->IsReady()) { RejectWeaponContent(Catalog->ValidationError); return; }
    bWeaponContentVerified = false;
    WeaponContentChallenge = FGuid::NewGuid();
    GetWorldTimerManager().SetTimer(WeaponContentTimeout, FTimerDelegate::CreateWeakLambda(this, [this]() { RejectWeaponContent(TEXT("Weapon content handshake timed out")); }), 30.f, false);
    ClientCheckWeaponContent(WeaponContentChallenge, UFPSWeaponCatalog::ManifestVersion, Catalog->RequiredIds(), Catalog->Digest());
}

void AFPSFrontController::ClientCheckWeaponContent_Implementation(FGuid Challenge, int32 Version, const TArray<FName>& Ids, const FString& Digest)
{
    auto* Catalog = GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    Catalog->PrepareWorld(GetWorld(), true);
    FString Failure = Catalog->ValidationError;
    if (Failure.IsEmpty() && (Version != UFPSWeaponCatalog::ManifestVersion || Ids != Catalog->RequiredIds() || Digest != Catalog->Digest()))
        Failure = TEXT("Weapon content mismatch: required definitions or resource revision differs");
    UE_LOG(LogTemp, Display, TEXT("FPS_WEAPON_CONTENT CLIENT %s definitions=%d digest=%s reason=%s"),
        Failure.IsEmpty() ? TEXT("MATCH") : TEXT("MISMATCH"), Catalog->RequiredIds().Num(), *Catalog->Digest(), *Failure);
    if (auto* GI = Cast<UFPSFrontGameInstance>(GetGameInstance()))
    {
        GI->PendingWeaponContentError = Failure;
        if (!Failure.IsEmpty()) GI->ConnectionError = Failure;
    }
    ServerConfirmWeaponContent(Challenge, Catalog->Digest(), Failure.Left(512));
}

void AFPSFrontController::ServerConfirmWeaponContent_Implementation(FGuid Challenge, const FString& Digest, const FString& ErrorMessage)
{
    if (bWeaponContentVerified || Challenge != WeaponContentChallenge) return;
    const auto* Catalog = GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    if (!Catalog->IsReady() || !ErrorMessage.IsEmpty() || Digest != Catalog->Digest())
    {
        RejectWeaponContent(ErrorMessage.IsEmpty() ? TEXT("Weapon content mismatch") : ErrorMessage.Left(512));
        return;
    }
    GetWorldTimerManager().ClearTimer(WeaponContentTimeout);
    bWeaponContentVerified = true;
    ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("FPS_WEAPON_CONTENT SERVER ACCEPT definitions=%d"), Catalog->RequiredIds().Num());
    if (auto* Mode = GetWorld()->GetAuthGameMode<AFPSMatchMode>())
    {
        // Restart before trying to begin: BeginRound respawns all admitted players once.
        if (!Mode->GetGameState<AFPSDemoGameState>()->bWaiting) Mode->RestartPlayer(this);
        Mode->TryStartDedicatedRound();
    }
}

void AFPSFrontController::RejectWeaponContent(const FString& Reason)
{
    GetWorldTimerManager().ClearTimer(WeaponContentTimeout);
    UE_LOG(LogTemp, Error, TEXT("FPS_WEAPON_CONTENT SERVER REJECT pawn=%d reason=%s"), GetPawn() != nullptr, *Reason);
    if (auto* Mode = GetWorld()->GetAuthGameMode<AFPSMatchMode>(); Mode && Mode->GameSession)
        Mode->GameSession->KickPlayer(this, FText::FromString(Reason));
}

void AFPSFrontController::ClientWasKicked_Implementation(const FText& ReturnReason)
{
    if (auto* GI = Cast<UFPSFrontGameInstance>(GetGameInstance())) { GI->PendingWeaponContentError = ReturnReason.ToString(); GI->ConnectionError = ReturnReason.ToString(); }
    UE_LOG(LogTemp, Warning, TEXT("FPS_WEAPON_CONTENT RETURN reason=%s"), *ReturnReason.ToString());
    Super::ClientWasKicked_Implementation(ReturnReason);
}
