#include "FPSAttachmentSmoke.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponCustomization.h"
#include "FPSWeaponAbilities.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSMatchFlow.h"
#include "FPSModeRules.h"
#include "FPSCustomizationMenu.h"
#include "FPSWeaponPickup.h"
#include "FPSDemoEffects.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UnrealClient.h"

void FFPSAttachmentSmoke::Tick(UFPSWeaponComponent* W)
{
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("FPSModularPartsNetwork"))) { ModularTick(W); return; }
    const bool Observer = FParse::Param(FCommandLine::Get(), TEXT("FPSAttachmentObserver"));
    if ((!Observer && !FParse::Param(FCommandLine::Get(), TEXT("FPSAttachmentSmoke"))) || !W->IsEnabled()) return;
    auto* Pawn = W->Character(); const bool Server = Pawn->HasAuthority();
    if (Observer)
    {
        if (Server || Pawn->IsLocallyControlled() || W->Equipped().ConfigurationRevision != 7 || !W->Definition()) return;
        static TSet<TWeakObjectPtr<UFPSWeaponComponent>> Checked;
        if (Checked.Contains(W)) return;
        Checked.Add(W);
        const_cast<UFPSAttachmentDefinition*>(W->Catalog()->FindAttachment(TEXT("Attachment.M4_MagazineStandard")))->CapacityDelta = -15;
        const auto Before = W->Equipped(); const auto Starts = W->ReloadStartPresentations, Commits = W->ReloadCommitPresentations;
        const auto Snapshot = W->VisibleState;
        W->Loadout = {}; W->OnRep_VisibleState();
        const bool Pass = Before.InstanceId.IsValid() && Before.AttachmentIds.Num() == 3 && W->Equipped().InstanceId == Before.InstanceId && W->Equipped().AttachmentIds == Before.AttachmentIds &&
            W->EffectiveStats().MagazineSize == 15 && W->EffectiveStats().AimFOV == 48 && FMath::IsNearlyEqual(W->EffectiveStats().VerticalRecoilMultiplier, .8f) &&
            W->Equipped().Magazine == 0 && W->Equipped().Reserve == 0 && !W->Loadout.Secondary.IsValid() &&
            W->Loadout.Action == Snapshot.Action && W->Loadout.ActionStartedAt == Snapshot.ActionStartedAt && W->Loadout.ActionEndsAt == Snapshot.ActionEndsAt &&
            Starts == W->ReloadStartPresentations && Commits == W->ReloadCommitPresentations && Pawn->FindComponentByClass<UStaticMeshComponent>() != nullptr;
        UE_LOG(LogTemp, Display, TEXT("FPS_ATTACHMENT_OBSERVER %s CLIENT COMPLETE late join restores committed IDs, effective stats and persistent state without replaying cues or exposing ammo"), Pass ? TEXT("PASS") : TEXT("FAIL"));
        return;
    }
    if (Server == Pawn->IsLocallyControlled() || !Cast<APlayerController>(Pawn->GetController())) return;
    {
        static TMap<TWeakObjectPtr<UWorld>, TWeakObjectPtr<UFPSWeaponComponent>> Drivers;
        auto& Driver = Drivers.FindOrAdd(W->GetWorld(), W);
        if (Driver != W) return; // late join and respawn must not become another fixture driver
    }
    auto* GS = W->GetWorld()->GetGameState<AFPSDemoGameState>();
    if (!GS || !GS->bTraining) return;
    struct FProgress { TSet<int32> Done; bool Failed = false; FGuid Gun; double Cooldown = 0; FFPSWeaponActionTicket OldAction; FGuid OldToken; FFPSWeaponCustomizationRequest OldRequest; uint32 Shots = 0; };
    static TMap<TWeakObjectPtr<UFPSWeaponComponent>, FProgress> Progress;
    auto& P = Progress.FindOrAdd(W);
    auto Check = [&](bool Pass, const TCHAR* Label)
    {
        P.Failed |= !Pass;
        UE_LOG(LogTemp, Display, TEXT("FPS_ATTACHMENT_SMOKE %s %s %s mag=%d reserve=%d revision=%u action=%d"), Pass ? TEXT("PASS") : TEXT("FAIL"), Server ? TEXT("SERVER") : TEXT("CLIENT"), Label,
            W->Equipped().Magazine, W->Equipped().Reserve, W->Equipped().ConfigurationRevision, int32(W->Loadout.Action));
    };
    if (Server && !P.Done.Contains(-1))
    {
        if (!W->CanAct()) return;
        P.Done.Add(-1); GS->RoundEndsAt = W->ServerTime() + 300; GS->ForceNetUpdate();
        W->StopAllActions(); W->Loadout.Primary = W->Catalog()->CreateInstance(TEXT("Weapon.M4")); W->Loadout.ActiveSlot = EFPSWeaponSlot::Primary;
        W->EquippedMutable().Magazine = 20; W->EquippedMutable().Reserve = 90; W->EquippedMutable().RecoilIndex = 4;
        W->EquippedMutable().NextShotAt = W->ServerTime() + .3;
        Pawn->SetActorLocation(FVector(0,0,1800)); W->Changed();
    }
    if (GS->RoundEndsAt <= 0) return;
    const double Time = W->ServerTime() - (GS->RoundEndsAt - 300);
    // Keep the committed gun alive while a separate cold Unreal process loads and joins.
    auto At = [&](int32 Tenth, auto Fn) { if (Time >= Tenth * .1 + (Tenth >= 245 ? 40 : 0) && !P.Done.Contains(Tenth)) { P.Done.Add(Tenth); Fn(); } };
    auto Send = [&](const FFPSWeaponCustomizationRequest& Request, const TCHAR* Tag)
    {
        FGameplayEventData Event; Event.TargetData.Add(new FFPSWeaponCustomizationRequest(Request));
        Pawn->GetAbilitySystemComponent()->HandleGameplayEvent(FGameplayTag::RequestGameplayTag(Tag), &Event);
    };
    auto* PC = Cast<AFPSFrontController>(Pawn->GetController());
    FString CapturePath; const bool PointerTest = FParse::Value(FCommandLine::Get(), TEXT("FPSAttachmentCapture="), CapturePath);
    At(0, [&]()
    {
        Pawn->GetCharacterMovement()->DisableMovement();
        const_cast<UFPSWeaponDefinition*>(W->Catalog()->Find(TEXT("Weapon.M4")))->ReloadDuration = .9f;
        const_cast<UFPSAttachmentDefinition*>(W->Catalog()->FindAttachment(TEXT("Attachment.M4_MagazineStandard")))->CapacityDelta = -15;
    });
    At(8, [&]() { P.Gun = W->Equipped().InstanceId; P.Cooldown = W->Equipped().NextShotAt; });
    At(10, [&]() { if (!Server) PC->ToggleCustomization(); });
    At(12, [&]()
    {
        if (Server) return;
        Check(PC->IsCustomizationMenuOpen() && W->IsCustomizationOpen(), TEXT("training menu opens"));
        P.OldRequest = W->NewCustomizationRequest(); P.OldRequest.Attachment = TEXT("Attachment.M4_OpticFixed");
        Send(P.OldRequest, TEXT("Input.Weapon.Modify"));
    });
    At(20, [&]() { Check(W->Equipped().InstanceId == P.Gun && W->Equipped().ConfigurationRevision == 2 && W->EffectiveStats().AimFOV == 48 && W->Equipped().Magazine == 20 && W->Equipped().Reserve == 90 && W->Equipped().NextShotAt == P.Cooldown && W->Equipped().RecoilIndex == 4, TEXT("optic commits without replacing instance ammo cooldown recoil")); });
    At(22, [&]() { if (!Server) { if (PointerTest) Check(FPSCustomizationSmokePointer(W, TEXT("Attachment.M4_MagazineExtended"), 0), TEXT("Slate mouse down on extended magazine")); else W->ModifyAttachment(EFPSAttachmentCategory::Magazine, TEXT("Attachment.M4_MagazineExtended")); } });
    At(23, [&]() { if (!Server && PointerTest) Check(FPSCustomizationSmokePointer(W, TEXT("Attachment.M4_MagazineExtended"), 1), TEXT("Slate drag detected")); });
    At(24, [&]() { if (!Server && PointerTest) Check(FPSCustomizationSmokePointer(W, TEXT("Attachment.M4_MagazineExtended"), 2), TEXT("Slate magazine drop routed to weapon")); });
    // Allow the request to reach authority before checking, while the reload is still pending.
    At(28, [&]()
    {
        P.OldAction = W->ActionTicket; P.OldToken = W->Loadout.ActionToken;
        Check(W->Loadout.Action == EFPSWeaponAction::Reloading && W->Equipped().Magazine == 20 && W->Equipped().ConfigurationRevision == 2 && W->EffectiveStats().MagazineSize == 30, TEXT("magazine remains pending during reload"));
        Check(!W->CanStart(EFPSWeaponRequest::Modify) && !W->CanStart(EFPSWeaponRequest::ModifyMagazine), TEXT("same gun blocks overlapping assembly"));
    });
    At(29, [&]()
    {
        if (Server) return;
        if (PointerTest) Check(FPSCustomizationSmokePointer(W, NAME_None, 3) && FPSCustomizationSmokePointer(W, NAME_None, 4), TEXT("Slate click cancels reloading weapon"));
        else W->CancelCustomizationReload();
    });
    At(35, [&]() { Check(W->Loadout.Action == EFPSWeaponAction::Idle && W->Equipped().Magazine == 20 && W->Equipped().Reserve == 90 && W->Equipped().ConfigurationRevision == 2, TEXT("cancel discards only pending magazine")); });
    At(37, [&]() { if (!Server) W->ModifyAttachment(EFPSAttachmentCategory::Magazine, TEXT("Attachment.M4_MagazineExtended")); });
    At(40, [&]() { if (Server) { W->EquippedMutable().Reserve = 150; W->Changed(); } });
    At(53, [&]()
    {
        Check(W->Equipped().Magazine == 45 && W->Equipped().Reserve == 125 && W->Equipped().ConfigurationRevision == 3, TEXT("commit consumes current reserve after independent increase"));
        Check(!W->FinishAction(P.OldAction, false) && !W->FinishAction(P.OldAction, true), TEXT("stale callback and cancellation rejected"));
    });
    At(55, [&]() { if (!Server) Send(P.OldRequest, TEXT("Input.Weapon.Modify")); });
    At(59, [&]() { Check(W->Equipped().ConfigurationRevision == 3 && W->EffectiveStats().MagazineSize == 45, TEXT("duplicate stale request cannot replace assembly")); });
    At(60, [&]() { if (!Server) W->ModifyAttachment(EFPSAttachmentCategory::Magazine, TEXT("Attachment.M4_MagazineStandard")); });
    At(63, [&]() { if (!Server) { auto Request = W->NewCustomizationRequest(); Request.CancelToken = P.OldToken; Send(Request, TEXT("Input.Weapon.CancelReload")); } });
    At(76, [&]() { Check(W->Equipped().Magazine == 15 && W->Equipped().Reserve == 155 && W->Equipped().ConfigurationRevision == 4, TEXT("shrink conserves ammo and stale cancel leaves newer reload")); });
    At(80, [&]()
    {
        P.Shots = W->Equipped().ShotSerial;
        auto* ASC = Pawn->GetAbilitySystemComponent();
        if (Server)
        {
            const auto* Fire = ASC->FindAbilitySpecFromClass(UFPSFireAbility::StaticClass());
            const auto* Aim = ASC->FindAbilitySpecFromClass(UFPSAimAbility::StaticClass());
            Check(Fire && Aim && !Fire->Ability->CanActivateAbility(Fire->Handle, ASC->AbilityActorInfo.Get()) && !Aim->Ability->CanActivateAbility(Aim->Handle, ASC->AbilityActorInfo.Get()), TEXT("server GAS gates fire and ADS while menu open"));
        }
        else Check(!ASC->TryActivateAbilityByClass(UFPSFireAbility::StaticClass()) && !ASC->TryActivateAbilityByClass(UFPSAimAbility::StaticClass()), TEXT("open menu blocks actual GAS fire and ADS"));
    });
    At(83, [&]() { if (!Server) PC->HideCustomization(); });
    // Character input synchronizes the menu lock each frame. Send new gameplay input after that boundary.
    At(84, [&]() { if (!Server) { W->StartFire(); W->StopFire(); W->AimOn(); } });
    At(88, [&]() { Check(W->Loadout.bAiming && (Server || FMath::IsNearlyEqual(Pawn->FindComponentByClass<UCameraComponent>()->FieldOfView, 48.f)), TEXT("committed optic drives replicated ADS and actual camera FOV")); });
    At(90, [&]() { Check(W->Equipped().ShotSerial > P.Shots, TEXT("new fire input works after close")); if (!Server) W->Reload(); });
    At(92, [&]() { if (!Server) PC->ToggleCustomization(); });
    // Opening the menu must round-trip before the owner's replicated ADS state is checked.
    At(97, [&]() { Check(W->Loadout.Action == EFPSWeaponAction::Reloading && !W->Loadout.bAiming, TEXT("opening menu retains normal reload and exits ADS")); if (!Server) W->CancelCustomizationReload(); });
    At(103, [&]() { Check(W->Loadout.Action == EFPSWeaponAction::Idle && W->Equipped().Magazine < 15, TEXT("normal reload cancelled through same checked action path")); if (!Server) W->ModifyAttachment(EFPSAttachmentCategory::Magazine, TEXT("Attachment.M4_MagazineExtended")); });
    At(106, [&]() { if (!Server) PC->HideCustomization(); });
    At(120, [&]() { Check(!W->IsCustomizationOpen() && W->Equipped().Magazine == 45 && W->Equipped().ConfigurationRevision == 5 && W->Loadout.Action == EFPSWeaponAction::Idle, TEXT("close retains pending reload and atomic commit")); });
    At(124, [&]() { if (!Server) PC->ToggleCustomization(); });
    At(130, [&]() { if (Server) const_cast<UFPSModeRules*>(UFPSModeRules::ForWorld(W->GetWorld()))->Kind = EFPSGameplayKind::TeamMatch; });
    At(132, [&]() { if (!Server) W->ModifyAttachment(EFPSAttachmentCategory::Muzzle, TEXT("Attachment.M4_MuzzleBrake")); });
    At(142, [&]()
    {
        Check(W->Equipped().ConfigurationRevision == 5 && !W->Equipped().AttachmentIds.Contains(TEXT("Attachment.M4_MuzzleBrake")) && W->Loadout.Action == EFPSWeaponAction::Idle, TEXT("server-only mode restriction rejects locally predicted modification"));
        if (!Server) Check(W->CustomizationFeedback().Contains(TEXT("服务器拒绝")), TEXT("real GAS prediction rejection reaches owning client"));
        if (Server) const_cast<UFPSModeRules*>(UFPSModeRules::ForWorld(W->GetWorld()))->Kind = EFPSGameplayKind::Training;
    });
    At(146, [&]() { if (Server) { W->EquippedMutable().Magazine = 5; W->EquippedMutable().Reserve = 0; W->Changed(); } });
    At(153, [&]() { if (!Server) W->ModifyAttachment(EFPSAttachmentCategory::Magazine, TEXT("Attachment.M4_MagazineStandard")); });
    At(170, [&]() { Check(W->Equipped().Magazine == 5 && W->Equipped().Reserve == 0 && W->Equipped().ConfigurationRevision == 6 && W->EffectiveStats().MagazineSize == 15, TEXT("insufficient ammo still conserves total on magazine commit")); });
    At(180, [&]() { P.Shots = W->Equipped().ShotSerial; });
    At(182, [&]()
    {
        if (Server) return;
        // Bypass only the local UI flag. The real server flag stays open and must reject the GAS RPCs.
        W->bCustomizationOpen = false;
        Pawn->GetAbilitySystemComponent()->TryActivateAbilityByClass(UFPSFireAbility::StaticClass());
        Pawn->GetAbilitySystemComponent()->TryActivateAbilityByClass(UFPSAimAbility::StaticClass());
        W->bCustomizationOpen = true;
    });
    At(189, [&]() { Check(W->Equipped().ShotSerial == P.Shots && W->Equipped().Magazine == 5 && !W->Loadout.bAiming, TEXT("server rejects fire ADS RPCs even when client bypasses UI flag")); });
    At(193, [&]() { if (!Server) W->ModifyAttachment(EFPSAttachmentCategory::Muzzle, TEXT("Attachment.M4_MuzzleBrake")); });
    At(201, [&]() { Check(W->Equipped().ConfigurationRevision == 7 && FMath::IsNearlyEqual(W->EffectiveStats().VerticalRecoilMultiplier, .8f) && FMath::IsNearlyEqual(W->EffectiveStats().HorizontalRecoilMultiplier, .85f), TEXT("muzzle commits shared recoil multipliers")); if (!Server) PC->HideCustomization(); else UE_LOG(LogTemp, Display, TEXT("FPS_ATTACHMENT_LATE_WINDOW")); });
    At(206, [&]()
    {
        if (Server) return;
        Pawn->GetController()->SetControlRotation(FRotator::ZeroRotator);
        const FVector Kick = W->Definition()->RecoilAt(0);
        W->StartFire(); W->StopFire();
        const FRotator Actual = Pawn->GetController()->GetControlRotation();
        Check(FMath::IsNearlyEqual(Actual.Pitch, Kick.X * .8f, .001f) && FMath::IsNearlyEqual(Actual.Yaw, Kick.Y * .85f, .001f), TEXT("predicted shot applies muzzle multipliers to actual control rotation"));
    });
    At(218, [&]()
    {
        Check(W->Equipped().InstanceId == P.Gun && W->Equipped().Magazine == 4 && W->Equipped().ConfigurationRevision == 7, TEXT("post-rejection fire preserves committed assembly and identity"));
        if (!Server) PC->ToggleCustomization();
    });
    At(225, [&]()
    {
        FString Capture;
        if (!Server && FParse::Value(FCommandLine::Get(), TEXT("FPSAttachmentCapture="), Capture)) FScreenshotRequest::RequestScreenshot(Capture, true, false);
    });
    At(245, [&]() { if (!Server) W->ModifyAttachment(EFPSAttachmentCategory::Magazine, TEXT("Attachment.M4_MagazineExtended")); });
    At(248, [&]() { if (!Server) W->Secondary(); });
    At(265, [&]() { Check(W->Loadout.Primary.InstanceId == P.Gun && W->Loadout.Primary.ConfigurationRevision == 7 && W->Loadout.Primary.Magazine == 4 && W->Loadout.ActiveSlot == EFPSWeaponSlot::Secondary && !W->PendingAttachmentTicket.Sequence, TEXT("switch cancels pending magazine without modifying stored gun")); });
    At(270, [&]() { if (!Server) W->Primary(); });
    At(280, [&]() { if (!Server) W->ModifyAttachment(EFPSAttachmentCategory::Magazine, TEXT("Attachment.M4_MagazineExtended")); });
    At(283, [&]() { if (!Server) PC->HideCustomization(); });
    At(284, [&]() { if (!Server) W->Drop(); });
    At(291, [&]()
    {
        bool Found = false;
        for (TActorIterator<AFPSWeaponPickup> It(W->GetWorld()); It; ++It)
            if (It->GetInstance().InstanceId == P.Gun)
            {
                const auto& Gun = It->GetInstance();
                Found = Gun.ConfigurationRevision == 7 && Gun.Magazine == 4 && Gun.Reserve == 0 && Gun.AttachmentIds.Contains(TEXT("Attachment.M4_MuzzleBrake")) && Gun.AttachmentIds.Contains(TEXT("Attachment.M4_OpticFixed"));
            }
        Check(Found && !W->Loadout.Primary.IsValid() && !W->PendingAttachmentTicket.Sequence, TEXT("drop cancels pending action and replicates committed assembly on pickup"));
    });
    if (Server && Time >= 69.1 && Time < 70.4)
        for (TActorIterator<AFPSWeaponPickup> It(W->GetWorld()); It; ++It)
            if (It->GetInstance().InstanceId == P.Gun) Pawn->SetActorLocation(It->GetActorLocation() + FVector(0,0,90), false, nullptr, ETeleportType::TeleportPhysics);
    At(300, [&]() { if (!Server) W->Pickup(); });
    At(310, [&]() { if (!Server) W->Primary(); });
    At(320, [&]() { Check(W->Equipped().InstanceId == P.Gun && W->Equipped().ConfigurationRevision == 7 && W->Equipped().Magazine == 4 && W->EffectiveStats().MagazineSize == 15 && W->EffectiveStats().AimFOV == 48, TEXT("pickup restores same instance assembly and effective parameters")); if (!Server) PC->ToggleCustomization(); });
    At(325, [&]() { if (!Server) W->ModifyAttachment(EFPSAttachmentCategory::Magazine, TEXT("Attachment.M4_MagazineExtended")); });
    At(329, [&]()
    {
        Check(W->Loadout.Action == EFPSWeaponAction::Reloading, TEXT("pending modification armed before actual GAS lethal damage"));
        P.OldAction = W->ActionTicket;
    });
    At(331, [&]()
    {
        if (Server)
        {
            auto* ASC = Pawn->GetAbilitySystemComponent();
            const auto Spec = ASC->MakeOutgoingSpec(UFPSDemoDamageEffect::StaticClass(), 1, ASC->MakeEffectContext());
            for (int32 Hit = 0; Hit < 4; ++Hit) ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
        }
    });
    At(343, [&]()
    {
        Check(Pawn->IsEliminated() && !W->ActionAbility && !W->PendingAttachmentTicket.Sequence && W->Loadout.Primary.InstanceId == P.Gun && W->Loadout.Primary.ConfigurationRevision == 7 && W->Loadout.Primary.Magazine == 4 && !W->FinishAction(P.OldAction, false), TEXT("death cancels pending magazine and old callback cannot commit"));
        Check(!P.Failed, TEXT("COMPLETE"));
    });
#endif
}
