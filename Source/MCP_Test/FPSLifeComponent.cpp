#include "FPSLifeComponent.h"
#include "FPSDemoAttributeSet.h"
#include "FPSDemoGameMode.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

FFPSDeathContext FFPSDeathContext::FromEffect(const FGameplayEffectContextHandle& Effect)
{
    FFPSDeathContext Result;
    Result.Instigator = Effect.GetOriginalInstigator();
    Result.Causer = Effect.GetEffectCauser();
    AController* Controller = Cast<AController>(Result.Instigator.Get());
    if (!Controller)
        if (const APawn* Pawn = Cast<APawn>(Result.Instigator.Get())) Controller = Pawn->GetController();
    if (!Controller && Result.Causer.IsValid()) Controller = Result.Causer->GetInstigatorController();
    Result.Attacker = Controller;
    Result.AttackerState = Controller ? Controller->PlayerState.Get() : Cast<APlayerState>(Result.Instigator.Get());
    if (Result.AttackerState.IsValid()) Result.SourceName = Result.AttackerState->GetPlayerName();
    else if (Result.Causer.IsValid()) Result.SourceName = Result.Causer->GetName();
    else if (Result.Instigator.IsValid()) Result.SourceName = Result.Instigator->GetName();
    return Result;
}

UFPSLifeComponent::UFPSLifeComponent() { SetIsReplicatedByDefault(true); }
void UFPSLifeComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner()->HasAuthority())
        if (auto* Rules = GetWorld()->GetAuthGameMode<AFPSDemoGameMode>())
            OnLifeEnded.AddUObject(Rules, &AFPSDemoGameMode::HandleLifeEnded);
}
void UFPSLifeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UFPSLifeComponent, bDead);
}
void UFPSLifeComponent::NotifyHealthExhausted(const FGameplayEffectContextHandle& Effect)
{
    const auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
    if (!GetOwner()->HasAuthority() || bDead || !ASC || ASC->GetNumericAttribute(UFPSDemoAttributeSet::GetHealthAttribute()) > 0) return;
    bDead = true; // Commit before notifying: recursive effects and duplicate notifications are harmless.
    GetOwner()->ForceNetUpdate();
    const auto Context = FFPSDeathContext::FromEffect(Effect);
    OnLifeEnded.Broadcast(this, Context);
}
