#include "FPSWeaponComponent.h"
#include "FPSDemoCharacter.h"
#include "FPSAudioSubsystem.h"
#include "GameFramework/Controller.h"

void UFPSWeaponComponent::EmitActionResult(const FFPSWeaponActionTicket& Ticket, EFPSWeaponResultPhase Phase)
{
    FFPSWeaponActionResult Result;
    Result.ActionId = Ticket.Sequence; Result.Instance = Ticket.Instance;
    Result.WeaponId = Equipped().WeaponId; Result.Command = Ticket.Command;
    Result.PredictionKey = Ticket.PredictionKey; Result.Phase = Phase;
    if (GetOwner()->HasAuthority())
    {
        Result.EventSerial = ++NextResultSerial;
        MulticastActionResult(Result);
    }
    else if (Phase == EFPSWeaponResultPhase::Started && Ticket.PredictionKey != 0 && Ticket.Command != EFPSWeaponRequest::Equip)
    {
        // Only the start is predicted. A local timer or rollback cannot announce a reload commit.
        // GAS prediction keys are connection-local; this journal exists only on the owning component.
        if (PredictedStarts.Num() >= 256) PredictedStarts.RemoveAt(0);
        PredictedStarts.Add(Result);
        PresentActionResult(Result);
    }
}

void UFPSWeaponComponent::MulticastActionResult_Implementation(const FFPSWeaponActionResult& Result)
{
    if (!Result.EventSerial || Result.EventSerial <= LastResultSerial) return;
    LastResultSerial = Result.EventSerial;
    const int32 Predicted = PredictedStarts.IndexOfByPredicate([&](const FFPSWeaponActionResult& Start)
    {
        return Start.Instance == Result.Instance && Start.Command == Result.Command && Start.PredictionKey == Result.PredictionKey;
    });
    if (Predicted != INDEX_NONE)
    {
        PredictedStarts.RemoveAt(Predicted);
        if (Result.Phase == EFPSWeaponResultPhase::Started)
        {
#if !UE_BUILD_SHIPPING
            ++ConfirmedStartDuplicates;
#endif
            return;
        }
    }
    PresentActionResult(Result);
}

void UFPSWeaponComponent::PresentActionResult(const FFPSWeaponActionResult& Result)
{
    // Results never mutate inventory/poses: an old weapon can finish while a newer snapshot is visible.
    const auto* Def = Catalog()->Find(Result.WeaponId);
    auto* Pawn = Character();
    if (!Def || !Pawn || GetNetMode() == NM_DedicatedServer) return;
    USoundBase* Sound = nullptr;
    if (Result.Phase == EFPSWeaponResultPhase::Started)
    {
        if (Result.Command == EFPSWeaponRequest::Reload) Sound = Def->ReloadStartSound.Get();
        else if (Result.Command == EFPSWeaponRequest::Inspect) Sound = Def->InspectSound.Get();
        else Sound = Def->EquipSound.Get();
#if !UE_BUILD_SHIPPING
        if (Result.Command == EFPSWeaponRequest::Reload) ++ReloadStartPresentations;
#endif
    }
    else if (Result.Phase == EFPSWeaponResultPhase::Committed && Result.Command == EFPSWeaponRequest::Reload)
    {
        Sound = Def->ReloadEndSound.Get();
#if !UE_BUILD_SHIPPING
        ++ReloadCommitPresentations;
#endif
    }
    const bool bLocalPlayer = Pawn->IsLocallyControlled() && Pawn->GetController() && Pawn->GetController()->IsPlayerController();
    if (Sound) if (auto* Audio = UFPSAudioSubsystem::Get(this)) Audio->PlaySound(Sound, Pawn->GetActorLocation(), bLocalPlayer);
}
