#include "FPSWeaponComponent.h"
#include "FPSExtractionMode.h"
#include "FPSDemoCharacter.h"
#include "FPSWeaponAbilities.h"
#include "AbilitySystemComponent.h"

bool UFPSWeaponComponent::RestoreExtractionLoadout(const FFPSWeaponLoadout& Saved, FString& Failure)
{
    const auto* Mode = GetWorld()->GetAuthGameMode<AFPSExtractionMode>();
    if (!Mode || !Mode->IsRaid() || GetWorld()->GetNetMode() != NM_Standalone || !GetOwner()->HasAuthority() ||
        !Character() || !Character()->GetController() || !Character()->GetController()->IsPlayerController() || !Catalog() || !Catalog()->IsReady())
    { Failure = TEXT("无法恢复出征装备。" ); return false; }
    for (const auto* Gun : {&Saved.Primary, &Saved.Secondary})
    {
        FFPSResolvedWeaponStats Stats; TArray<FString> Errors;
        if (!Gun->IsValid() || !Catalog()->Resolve(Gun->WeaponId, Gun->AttachmentIds, Stats, Errors) ||
            Gun->Magazine < 0 || Gun->Magazine > Stats.MagazineSize || Gun->Reserve < 0)
        { Failure = TEXT("出征装备内容无效。" ); return false; }
    }
    StopAllActions(); Loadout = Saved;
    Loadout.Action = EFPSWeaponAction::Idle; Loadout.ActionToken.Invalidate(); Loadout.bAiming = Loadout.bFireActive = false;
    ++Loadout.PrimaryInventoryRevision; ++Loadout.SecondaryInventoryRevision;
    Changed(); Character()->GetAbilitySystemComponent()->TryActivateAbilityByClass(UFPSEquipAbility::StaticClass());
    return true;
}
