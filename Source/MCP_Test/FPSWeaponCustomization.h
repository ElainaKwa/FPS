#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "FPSAttachmentData.h"
#include "FPSWeaponCustomization.generated.h"

/** IDs and intent only. Never accepts resolved stats, ammo or a client deadline. */
USTRUCT()
struct FFPSWeaponCustomizationRequest : public FGameplayAbilityTargetData
{
    GENERATED_BODY()
    UPROPERTY() FGuid Operation;
    UPROPERTY() FGuid Instance;
    UPROPERTY() uint32 Revision = 0;
    UPROPERTY() uint32 Sequence = 0;
    UPROPERTY() EFPSAttachmentCategory Category = EFPSAttachmentCategory::Optic;
    UPROPERTY() FName Attachment;
    UPROPERTY() FGuid CancelToken;
    virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }
    bool NetSerialize(FArchive& Ar, UPackageMap*, bool& Success)
    {
        Ar << Operation << Instance << Revision << Sequence << Category << Attachment << CancelToken;
        Success = !Ar.IsError(); return Success;
    }
    static const FFPSWeaponCustomizationRequest* Read(const FGameplayAbilityTargetDataHandle& Handle)
    {
        const auto* Data = Handle.Num() == 1 ? Handle.Get(0) : nullptr;
        return Data && Data->GetScriptStruct() == StaticStruct() ? static_cast<const FFPSWeaponCustomizationRequest*>(Data) : nullptr;
    }
};
template<> struct TStructOpsTypeTraits<FFPSWeaponCustomizationRequest> : TStructOpsTypeTraitsBase2<FFPSWeaponCustomizationRequest> { enum { WithNetSerializer = true, WithCopy = true }; };
