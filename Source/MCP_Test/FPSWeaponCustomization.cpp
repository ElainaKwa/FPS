#include "FPSWeaponCustomization.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponAbilities.h"
#include "FPSDemoCharacter.h"
#include "FPSModeRules.h"
#include "AbilitySystemComponent.h"

bool UFPSWeaponComponent::IsTrainingCustomizationAllowed() const
{
    const auto* Rules = UFPSModeRules::ForWorld(GetWorld());
    return CanAct() && Rules && Rules->Kind == EFPSGameplayKind::Training;
}

void UFPSWeaponComponent::StopForCustomization()
{
    CancelForMenu();
}

bool UFPSWeaponComponent::SetCustomizationOpen(bool bOpen)
{
    if (IsShuttingDown() || (bOpen && !IsTrainingCustomizationAllowed())) return false;
    bCustomizationOpen = bOpen;
    if (bOpen) StopForCustomization();
    if (!GetOwner()->HasAuthority()) ServerSetCustomizationOpen(bOpen);
    else GetOwner()->ForceNetUpdate();
    return true;
}

void UFPSWeaponComponent::ServerSetCustomizationOpen_Implementation(bool bOpen)
{
    if (!SetCustomizationOpen(bOpen)) { bCustomizationOpen = false; GetOwner()->ForceNetUpdate(); }
}

FFPSWeaponCustomizationRequest UFPSWeaponComponent::NewCustomizationRequest()
{
    FFPSWeaponCustomizationRequest Request;
    Request.Operation = FGuid::NewGuid(); Request.Instance = Equipped().InstanceId;
    Request.Revision = Equipped().ConfigurationRevision;
    if (NextModificationSequence != MAX_uint32) Request.Sequence = ++NextModificationSequence;
    return Request;
}

void UFPSWeaponComponent::ModifyAttachment(EFPSAttachmentCategory Category, FName Attachment)
{
    if (!bCustomizationOpen || !IsTrainingCustomizationAllowed()) return;
    if (Loadout.Action != EFPSWeaponAction::Idle) { ModificationFeedback = TEXT("请等待当前换弹或切枪完成，或点击换弹中的武器取消。"); return; }
    auto Request = NewCustomizationRequest(); Request.Category = Category; Request.Attachment = Attachment;
    FGameplayEventData Event;
    Event.TargetData.Add(new FFPSWeaponCustomizationRequest(Request));
    const bool Magazine = Category == EFPSAttachmentCategory::Magazine;
    ModificationFeedback = Magazine ? TEXT("正在请求更换弹匣…") : TEXT("正在请求改装…");
    Character()->GetAbilitySystemComponent()->HandleGameplayEvent(FGameplayTag::RequestGameplayTag(Magazine ? TEXT("Input.Weapon.ModifyMagazine") : TEXT("Input.Weapon.Modify")), &Event);
}

void UFPSWeaponComponent::CancelCustomizationReload()
{
    if (!bCustomizationOpen || Loadout.Action != EFPSWeaponAction::Reloading) return;
    auto Request = NewCustomizationRequest(); Request.CancelToken = Loadout.ActionToken;
    FGameplayEventData Event; Event.TargetData.Add(new FFPSWeaponCustomizationRequest(Request));
    Character()->GetAbilitySystemComponent()->HandleGameplayEvent(FGameplayTag::RequestGameplayTag(TEXT("Input.Weapon.CancelReload")), &Event);
}

bool UFPSWeaponComponent::ValidateCustomizationRequest(const FFPSWeaponCustomizationRequest* Request)
{
    if (!Request || !Request->Operation.IsValid() || !Request->Sequence) return false;
    if (GetOwner()->HasAuthority())
    {
        if (Request->Sequence <= LastModificationSequence) return false;
        LastModificationSequence = Request->Sequence;
    }
    return bCustomizationOpen && IsTrainingCustomizationAllowed() && Request->Instance == Equipped().InstanceId &&
        Request->Revision == Equipped().ConfigurationRevision;
}

bool UFPSWeaponComponent::BeginModification(FFPSWeaponActionTicket& Ticket, UFPSWeaponAbility* Ability,
    const FFPSWeaponCustomizationRequest* Request, float& Duration)
{
    const bool Magazine = Ticket.Command == EFPSWeaponRequest::ModifyMagazine;
    auto Fail = [&](const FString& Reason)
    {
        ModificationFeedback = Reason;
        if (GetOwner()->HasAuthority() && Request) ClientModificationResult(Request->Sequence, Reason);
        return false;
    };
    if ((!Magazine && Ticket.Command != EFPSWeaponRequest::Modify) || !ValidOpening(Ticket, Ability) || !ValidateCustomizationRequest(Request)) return Fail(TEXT("改装请求已过时，或当前无法改装。"));
    if ((Request->Category == EFPSAttachmentCategory::Magazine) != Magazine ||
        (Request->Category != EFPSAttachmentCategory::Optic && Request->Category != EFPSAttachmentCategory::Muzzle && Request->Category != EFPSAttachmentCategory::Magazine)) return Fail(TEXT("配件类别无效。"));
    TArray<FName> Target = Equipped().AttachmentIds;
    Target.RemoveAll([&](FName Id) { const auto* Part = Catalog()->FindAttachment(Id); return Part && Part->Category == Request->Category; });
    if (!Request->Attachment.IsNone())
    {
        const auto* Part = Catalog()->FindAttachment(Request->Attachment);
        if (!Part || Part->Category != Request->Category) return Fail(TEXT("配件未准入或类别不匹配。"));
        Target.Add(Request->Attachment);
    }
    Target.Sort(FNameLexicalLess());
    TArray<FName> Current = Equipped().AttachmentIds; Current.Sort(FNameLexicalLess());
    if (Target == Current) return Fail(TEXT("已经使用此配置。"));
    if (Equipped().ConfigurationRevision == MAX_uint32) return Fail(TEXT("配置版本已耗尽。"));
    TArray<FString> Errors; FFPSResolvedWeaponStats Stats;
    if (!Catalog()->Resolve(Equipped().WeaponId, Target, Stats, Errors)) return Fail(FString::Join(Errors, TEXT("\n")));
    const int64 Total = int64(Equipped().Magazine) + Equipped().Reserve;
    if (Magazine && Total - FMath::Min<int64>(Total, Stats.MagazineSize) > MAX_int32) return Fail(TEXT("改装后的备弹数量超出可保存范围。"));
    OpeningTicket = {};
    ActionTicket = Ticket; ActionAbility = Ability;
    PendingAttachmentIds = MoveTemp(Target); PendingAttachmentTicket = Ticket; PendingModificationSequence = Request->Sequence;
    Duration = Magazine ? Stats.ReloadDuration : 0.f;
    Loadout.Action = Magazine ? EFPSWeaponAction::Reloading : EFPSWeaponAction::Idle;
    Loadout.ActionToken = Request->Operation; Loadout.ActionStartedAt = ServerTime();
    ActionDeadline = ServerTime() + Duration; Loadout.ActionEndsAt = ActionDeadline;
    Changed();
    if (Magazine) EmitActionResult(Ticket, EFPSWeaponResultPhase::Started);
    ModificationFeedback = Magazine ? TEXT("换弹中，点击武器可取消。") : TEXT("等待服务器确认改装。");
    return true;
}

bool UFPSWeaponComponent::CancelMatchingReload(const FFPSWeaponActionTicket& Ticket, const FFPSWeaponCustomizationRequest* Request)
{
    if (Ticket.Command != EFPSWeaponRequest::CancelReload || !ValidOpening(Ticket, OpeningAbility.Get()) || !ValidateCustomizationRequest(Request) ||
        !Request->CancelToken.IsValid() || Request->CancelToken != Loadout.ActionToken || Loadout.Action != EFPSWeaponAction::Reloading || !ActionAbility) return false;
    OpeningTicket = {};
    ActionAbility->CancelWeaponAbility();
    if (GetOwner()->HasAuthority()) ClientModificationResult(Request->Sequence, TEXT("已取消换弹。"));
    return true;
}

void UFPSWeaponComponent::ClientModificationResult_Implementation(uint32 Sequence, const FString& Message)
{
    if (Sequence < LastModificationReply || Sequence < NextModificationSequence) return;
    LastModificationReply = Sequence; ModificationFeedback = Message;
}
