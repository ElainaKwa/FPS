#include "FPSDynamicDrops.h"
#include "FPSWeaponPickup.h"
#include "FPSModeRules.h"
#include "FPSDemoGameState.h"
#include "Engine/World.h"

int32 UFPSDynamicDrops::Count() const
{
    int32 Result = 0;
    for (const auto& Entry : Ordered) if (Entry.IsValid() && Entry->IsAvailable()) ++Result;
    return Result;
}
bool UFPSDynamicDrops::Register(AFPSWeaponPickup* Drop)
{
    const auto* Rules = UFPSModeRules::ForWorld(GetWorld());
    if (!Drop || !Drop->HasAuthority() || !Drop->IsDynamic() || !Rules || Rules->DynamicDropLimit < 1) return false;
    Ordered.RemoveAll([](const auto& Entry) { return !Entry.IsValid(); });
    Ordered.AddUnique(Drop);
    while (Count() > Rules->DynamicDropLimit)
    {
        AFPSWeaponPickup* Oldest = nullptr;
        for (const auto& Entry : Ordered)
            if (Entry.IsValid() && Entry->IsAvailable()) { Oldest = Entry.Get(); break; }
        if (!Oldest) break;
        Ordered.Remove(Oldest);
        Oldest->Destroy();
        if(auto* GS=GetWorld()->GetGameState<AFPSDemoGameState>())
        {GS->DropNoticeUntil=GS->GetServerWorldTimeSeconds()+4;GS->ForceNetUpdate();}
    }
    return true;
}
void UFPSDynamicDrops::Unregister(AFPSWeaponPickup* Drop) { Ordered.Remove(Drop); }
void UFPSDynamicDrops::ClearRound()
{
    const auto Entries = MoveTemp(Ordered);
    for (const auto& Entry : Entries) if (Entry.IsValid()) Entry->Destroy();
}
