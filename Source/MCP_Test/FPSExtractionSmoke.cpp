#include "FPSExtractionSmoke.h"
#include "FPSExtractionMode.h"
#include "FPSExtractionSubsystem.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoEffects.h"
#include "FPSWeaponComponent.h"
#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"
#include "InputKeyEventArgs.h"

void FFPSExtractionSmoke::Tick(AFPSExtractionMode* Mode)
{
#if !UE_BUILD_SHIPPING
    const bool Reload = FParse::Param(FCommandLine::Get(), TEXT("FPSExtractionReloadCheck"));
    if (!Mode || (!Reload && !FParse::Param(FCommandLine::Get(), TEXT("FPSExtractionSmoke")))) return;
    auto* Loop = Mode->GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>();
    if (!Loop || !Loop->Profile || !Loop->bStorageHealthy || Mode->GetWorld()->GetTimeSeconds() < 1.5f) return;
    struct FRun { int32 Step = 0, InputStep = 0; double At = 0; FGuid Gun; int32 Magazine = 0; bool Failed = false, Fired = false, Captured = false; };
    static TMap<TWeakObjectPtr<UGameInstance>, FRun> Runs;
    auto& R = Runs.FindOrAdd(Mode->GetGameInstance());
    if (R.Failed) return;
    const double Now = FPlatformTime::Seconds();
    auto Check = [&](bool Pass, const TCHAR* Name)
    {
        UE_LOG(LogTemp, Display, TEXT("FPS_EXTRACTION_TEST %s %s step=%d"), Pass ? TEXT("PASS") : TEXT("FAIL"), Name, R.Step);
        if (!Pass) { R.Failed = true; FPlatformMisc::RequestExitWithStatus(false, 1); }
        return Pass;
    };
    auto Next = [&]() { ++R.Step; R.At = Now; R.Captured = false; };
    auto Capture = [&](const TCHAR* Name)
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("FPSExtractionCapture")))
        {
            FString Directory; FParse::Value(FCommandLine::Get(), TEXT("ExtractionEvidence="), Directory);
            if (!Directory.IsEmpty()) FScreenshotRequest::RequestScreenshot(Directory / Name, true, false);
        }
    };
    if (Reload)
    {
        Check(Loop->GetPhase() == EFPSExtractionPhase::Base && !Loop->Profile->bExpeditionActive &&
            Loop->Profile->Loadout.Primary.AttachmentIds.Contains(TEXT("Attachment.M4_MuzzleBrake")) &&
            Loop->Count(Loop->Profile->Warehouse, TEXT("Scrap")) == 3 && Loop->Profile->Credits == 58 &&
            Loop->Profile->Version==2 && Loop->Profile->PreparedBackpack.Num()==1 &&
            Loop->Profile->PreparedBackpack[0].ItemId==TEXT("Wire") && Loop->Profile->PreparedBackpack[0].Quantity==1 &&
            Loop->Profile->PreparedBackpack[0].X==4 && Loop->Profile->PreparedBackpack[0].Y==3,
            TEXT("FRESH_PROCESS_PERSISTENCE"));
        if (!R.Failed) { UE_LOG(LogTemp, Display, TEXT("FPS_EXTRACTION_TEST RELOAD COMPLETE")); FPlatformMisc::RequestExit(false); R.Failed = true; }
        return;
    }
    auto* PC = Cast<AFPSExtractionController>(Mode->GetWorld()->GetFirstPlayerController());
    auto* Body = PC ? Cast<AFPSDemoCharacter>(PC->GetPawn()) : nullptr;
    auto Key = [&](FKey K, EInputEvent Event) { PC->InputKey(FInputKeyEventArgs(nullptr, FInputDeviceId::CreateFromInternalId(0), K, Event, Event == IE_Pressed ? 1.f : 0.f, false, FPlatformTime::Cycles64())); };
    auto Pick = [&](FName Id)
    {
        AFPSExtractionLoot* Closest = nullptr;
        float Best = MAX_flt;
        for (TActorIterator<AFPSExtractionLoot> It(Mode->GetWorld()); It; ++It)
            if (It->ItemId == Id && It->Quantity > 0)
            {
                const float D = FVector::DistSquared(Body->GetActorLocation(), It->GetActorLocation());
                if (D < Best) { Best = D; Closest = *It; }
            }
        if (!Closest) return false;
        Body->SetActorLocation(Closest->GetActorLocation() + FVector(-100,0,90), false, nullptr, ETeleportType::TeleportPhysics);
        PC->SetControlRotation((Closest->GetActorLocation() - Body->GetPawnViewLocation()).Rotation());
        const int32 Before = Loop->Count(Loop->Backpack, Id), Quantity = Closest->Quantity;
        Mode->Interact(PC);
        return Loop->Count(Loop->Backpack, Id) == Before + Quantity;
    };
    switch (R.Step)
    {
    case 0:
    {
        if (!Check(Loop->Profile->Warehouse.IsEmpty(), TEXT("isolated empty profile"))) return;
        TArray<FFPSExtractionStack> Bag;
        if (!Check(Loop->TryAdd(Bag,TEXT("Scrap"),200,20) && !Loop->TryAdd(Bag,TEXT("Wire"),1,20) &&
            Loop->Count(Bag,TEXT("Scrap")) == 200 && Loop->SlotsUsed(Bag)==20 && !Loop->TryRemove(Bag,TEXT("Scrap"),-1) &&
            !Loop->TryAdd(Bag,TEXT("Unknown"),1,20), TEXT("grid capacity invalid quantity unknown item atomic rejection"))) return;
        if (!Check(GridChecks(Loop),TEXT("physical grid transaction migration and rotation"))) return;
        if (!Check(!Loop->Sell(TEXT("Scrap"),1) && !Loop->StartProduction(TEXT("CraftBrake")) &&
            !Loop->InstallAttachment(TEXT("Brake")) && !Loop->ClaimProduction(), TEXT("missing inventory rejects without mutation"))) return;
        R.Gun = Loop->Profile->Loadout.Primary.InstanceId;
        Capture(TEXT("Base.png")); Next(); break;
    }
    case 1:
        if (Now - R.At < 1) return;
        Next(); Check(Loop->StartExpedition(),TEXT("start expedition")); break;
    case 2:
        if (!Mode->IsRaid() || !Body || !Mode->bRestored || Now - R.At < 3) return;
        if (R.InputStep == 0)
        { Key(EKeys::Tab,IE_Pressed); R.InputStep = 1; R.At = Now-2.7; return; }
        if (R.InputStep == 1)
        {
            Key(EKeys::Tab,IE_Released);
            if (!Check(PC->IsExtractionPanelOpen() && PC->IsMoveInputIgnored() && !Body->GetWeapons()->CanStart(EFPSWeaponRequest::Fire), TEXT("native Tab opens bag and blocks combat input"))) return;
            Capture(TEXT("Backpack.png")); Key(EKeys::Escape,IE_Pressed); R.InputStep = 2; R.At = Now-2.7; return;
        }
        if (R.InputStep == 2)
        {
            Key(EKeys::Escape,IE_Released);
            if (!Check(!PC->IsExtractionPanelOpen() && !PC->IsMoveInputIgnored(), TEXT("native Escape closes bag and releases movement"))) return;
            R.InputStep = 3;
        }
        if (!Check(Body->GetWeapons()->GetLoadout().Primary.InstanceId == R.Gun &&
            !Body->GetWeapons()->CanStart(EFPSWeaponRequest::Drop) && !Body->GetWeapons()->CanStart(EFPSWeaponRequest::Pickup), TEXT("persistent equipment and protected transfer"))) return;
        if (!Check(Pick(TEXT("Scrap")) && Pick(TEXT("Wire")), TEXT("world search pickup to backpack"))) return;
        if (!Check(Loop->DropItem(TEXT("Wire"),1) && Loop->Count(Loop->Backpack,TEXT("Wire")) == 1, TEXT("cargo drop removes exactly one"))) return;
        if (!Check(Pick(TEXT("Wire")), TEXT("dropped cargo can be reclaimed"))) return;
        Body->SetActorLocation(Mode->ExitLocation, false, nullptr, ETeleportType::TeleportPhysics); Mode->Interact(PC);
        PC->SetControlRotation(FRotator(0,0,0));
        if (!Check(Mode->ExitStarted >= 0, TEXT("extraction starts in zone"))) return;
        Body->SetActorLocation(Mode->ExitLocation + FVector(400,0,0), false, nullptr, ETeleportType::TeleportPhysics);
        Next(); break;
    case 3:
        if (Now - R.At < .5) return;
        if (!Check(Mode->ExitStarted < 0, TEXT("leaving extraction cancels countdown"))) return;
        Body->SetActorLocation(Mode->ExitLocation, false, nullptr, ETeleportType::TeleportPhysics); Mode->Interact(PC);
        Capture(TEXT("Raid.png")); Next(); break;
    case 4:
        if (Loop->Phase != EFPSExtractionPhase::Result) return;
        if (!Check(Loop->bLastSuccess && Loop->Count(Loop->Profile->Warehouse,TEXT("Scrap")) == 8 &&
            Loop->Count(Loop->Profile->Warehouse,TEXT("Wire")) == 2 && !Loop->FinishExpedition(true), TEXT("natural countdown settles once"))) return;
        Next(); break;
    case 5:
        if (Now - R.At > .3 && !R.Captured) { Capture(TEXT("Success.png")); R.Captured = true; }
        if (Now - R.At < 1) return;
        Next(); Check(Loop->ReturnToBase(),TEXT("return to base")); break;
    case 6:
        if (Mode->IsRaid()) return;
        {
            const auto Scrap=Loop->Profile->Warehouse.FindByPredicate([](const auto& S){return S.ItemId==TEXT("Scrap");})->StackId;
            const auto Wire=Loop->Profile->Warehouse.FindByPredicate([](const auto& S){return S.ItemId==TEXT("Wire");})->StackId;
            if(!Check(Loop->MoveStack(EFPSExtractionContainer::Warehouse,Scrap,EFPSExtractionContainer::Backpack,2,1,true) &&
                !Loop->MoveStack(EFPSExtractionContainer::Warehouse,Wire,EFPSExtractionContainer::Backpack,2,2,false) &&
                Loop->Count(Loop->Profile->Warehouse,TEXT("Wire"))==2 &&
                Loop->AutoTransferStack(EFPSExtractionContainer::Backpack,Scrap,EFPSExtractionContainer::Warehouse),TEXT("base drag transfer rotation overlap rollback")))return;
        }
        if (!Check(Loop->Sell(TEXT("Scrap"),1) && Loop->StartProduction(TEXT("ProcessParts")) &&
            !Loop->StartProduction(TEXT("ProcessParts")) && !Loop->ClaimProduction(), TEXT("sale production reservation no early claim"))) return;
        Next(); break;
    case 7:
        if (!Loop->GetView().bProductionReady) return;
        if (!Check(Loop->ClaimProduction() && !Loop->ClaimProduction() && Loop->Count(Loop->Profile->Warehouse,TEXT("Parts")) == 2,
            TEXT("production output claimed once"))) return;
        if (!Check(Loop->StartProduction(TEXT("CraftBrake")),TEXT("manufacture attachment"))) return;
        Next(); break;
    case 8:
        if (!Loop->GetView().bProductionReady) return;
        if (!Check(Loop->ClaimProduction() && Loop->InstallAttachment(TEXT("Brake")) && !Loop->InstallAttachment(TEXT("Brake")) &&
            Loop->Profile->Loadout.Primary.InstanceId == R.Gun && Loop->Count(Loop->Profile->Warehouse,TEXT("Brake")) == 0,
            TEXT("inventory attachment install preserves gun identity"))) return;
        if (!Check(Loop->RemoveAttachment(TEXT("Attachment.M4_MuzzleBrake")) && Loop->InstallAttachment(TEXT("Brake")), TEXT("detach and reinstall conserves inventory"))) return;
        Next(); break;
    case 9:
        if (Now - R.At > .3 && !R.Captured) { Capture(TEXT("Upgraded.png")); R.Captured = true; }
        if (Now - R.At < 1) return;
        {
            const auto Wire=Loop->Profile->Warehouse.FindByPredicate([](const auto& S){return S.ItemId==TEXT("Wire");})->StackId;
            if(!Check(Loop->MoveStack(EFPSExtractionContainer::Warehouse,Wire,EFPSExtractionContainer::Backpack,4,3,false),TEXT("prepare carried supplies")))return;
        }
        Next(); Check(Loop->StartExpedition(),TEXT("second expedition")); break;
    case 10:
        if (!Mode->IsRaid() || !Body || !Mode->bRestored || Now - R.At < 3) return;
        if (!Check(Body->GetWeapons()->GetLoadout().Primary.InstanceId == R.Gun &&
            Body->GetWeapons()->EffectiveStats().VerticalRecoilMultiplier < 1.f, TEXT("second expedition consumes crafted modifier"))) return;
        if (!Check(Pick(TEXT("Scrap")),TEXT("second expedition loot"))) return;
        {
            const auto Wire=Loop->Backpack.FindByPredicate([](const auto& S){return S.ItemId==TEXT("Wire");})->StackId;
            if(!Check(!Loop->AutoTransferStack(EFPSExtractionContainer::Backpack,Wire,EFPSExtractionContainer::Warehouse) &&
                Loop->MoveStack(EFPSExtractionContainer::Backpack,Wire,EFPSExtractionContainer::Backpack,3,3,false),TEXT("raid grid sorting and remote warehouse rejection")))return;
        }
        R.Magazine = Body->GetWeapons()->Equipped().Magazine;
        PC->SetControlRotation(FRotator(-40,0,0)); Next(); break;
    case 11:
        if (Now - R.At < .4) return;
        // Position injection must allow the ordinary base-membership tick to remove protection.
        if (!R.Fired) { Body->GetWeapons()->StartFire(); R.Fired = true; R.At = Now; return; }
        Body->GetWeapons()->StopFire();
        if (!Check(Body->GetWeapons()->Equipped().Magazine < R.Magazine, TEXT("real GAS shooting consumes ammo"))) return;
        for (int32 N = 0; N < 4; ++N) Body->GetAbilitySystemComponent()->ApplyGameplayEffectToSelf(GetDefault<UFPSDemoDamageEffect>(),1,Body->GetAbilitySystemComponent()->MakeEffectContext());
        if (!Check(Body->IsEliminated() && Loop->Phase == EFPSExtractionPhase::Result && !Loop->bLastSuccess &&
            Loop->Count(Loop->Profile->Warehouse,TEXT("Scrap")) == 3 && Loop->Profile->Loadout.Primary.InstanceId == R.Gun &&
            Loop->Profile->Loadout.Primary.AttachmentIds.Contains(TEXT("Attachment.M4_MuzzleBrake")), TEXT("actual GAS death loses haul preserves equipment"))) return;
        Next(); break;
    case 12:
        if (Now - R.At > .3 && !R.Captured) { Capture(TEXT("Failure.png")); R.Captured = true; }
        if (Now - R.At < 1) return;
        Next(); Check(Loop->ReturnToBase(),TEXT("return after death")); break;
    case 13:
        if (Mode->IsRaid()) return;
        if(!Check(Loop->Count(Loop->GetView().Backpack,TEXT("Wire"))==1 && Loop->Count(Loop->LastHaul,TEXT("Wire"))==0,
            TEXT("death restores carried supplies but loses new haul")))return;
        if (!Check(Loop->Profile->Credits == 58 && !Loop->Profile->bExpeditionActive && Loop->Profile->Loadout.Primary.InstanceId == R.Gun,
            TEXT("two expeditions persistent economy"))) return;
        UE_LOG(LogTemp, Display, TEXT("FPS_EXTRACTION_TEST COMPLETE")); R.Failed = true; FPlatformMisc::RequestExit(false); break;
    }
#endif
}
