#include "FPSLoadoutSmoke.h"
#include "FPSMatchFlow.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "FPSDemoAttributeSet.h"
#include "FPSDemoEffects.h"
#include "FPSBaseMembership.h"
#include "FPSBaseRegion.h"
#include "FPSActionPolicy.h"
#include "FPSLifeComponent.h"
#include "FPSWeaponComponent.h"
#include "FPSModeRules.h"
#include "FPSWeaponPickup.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerInput.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "EngineUtils.h"

UFPSHostilePeriodicSmokeEffect::UFPSHostilePeriodicSmokeEffect()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration; DurationMagnitude = FScalableFloat(10);
    Period = FScalableFloat(.5f);
    FGameplayModifierInfo Modifier; Modifier.Attribute = UFPSDemoAttributeSet::GetHealthAttribute();
    Modifier.ModifierOp = EGameplayModOp::Additive; Modifier.ModifierMagnitude = FScalableFloat(-2);
    Modifiers.Add(Modifier);
}
void FFPSLoadoutSmoke::Start(AFPSMatchMode* Mode)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSLoadoutSmoke"))) return;
    FTimerHandle Timer;
    Mode->GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(Mode, [Mode]()
    {
        auto* GS = Mode->GetGameState<AFPSDemoGameState>();
        if (!GS->bWaiting || GS->PlayerArray.Num() != 2) return;
        for (auto It = Mode->GetWorld()->GetPlayerControllerIterator(); It; ++It)
            if (!CastChecked<AFPSFrontController>(It->Get())->IsWeaponContentVerified()) return;
        Mode->StartRound(Mode->GetWorld()->GetFirstPlayerController());
    }), .2f, true);
#endif
}
void FFPSLoadoutSmoke::Tick(AFPSFrontController* PC)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSLoadoutSmoke")) || !PC->IsLocalController()) return;
    auto* World = PC->GetWorld(); auto* GS = World->GetGameState<AFPSDemoGameState>();
    auto* Body = Cast<AFPSDemoCharacter>(PC->GetPawn());
    if (!GS || GS->bWaiting || !Body || !PC->IsWeaponContentVerified()) return;
    const bool Server = PC->HasAuthority(), Training = GS->bTraining;
    struct FProgress
    {
        TSet<int32> Done; bool Failed = false; double Epoch = 0;
        TWeakObjectPtr<AFPSFrontController> Remote; TWeakObjectPtr<AFPSDemoCharacter> OldBody;
        TWeakObjectPtr<AFPSBaseRegion> OwnRegion, OtherRegion, ExtraRegion;
        TWeakObjectPtr<AActor> SpawnBlocker;
        FVector RemoteSpawn; FGuid Gun, Secondary, Repeated, OldLife, VictimGun;
        float Health = 0; FActiveGameplayEffectHandle External, Periodic;
    };
    static TMap<TWeakObjectPtr<UWorld>, FProgress> Progress; auto& P = Progress.FindOrAdd(World);
    if (P.Epoch == 0) P.Epoch = World->GetTimeSeconds();
    const double Time = Training ? World->GetTimeSeconds() - P.Epoch : GS->GetServerWorldTimeSeconds() - (GS->RoundEndsAt - 300);
    auto Check = [&](bool Pass, const TCHAR* Name)
    { P.Failed |= !Pass; UE_LOG(LogTemp, Display, TEXT("FPS_LOADOUT_SMOKE %s %s %s"), Pass ? TEXT("PASS") : TEXT("FAIL"), Server ? TEXT("SERVER") : TEXT("CLIENT"), Name); };
    auto At = [&](int32 Tenth, TFunction<void()> Action)
    { if (Time >= Tenth * .1 && !P.Done.Contains(Tenth)) { P.Done.Add(Tenth); Action(); } };
    auto* Membership = Body->FindComponentByClass<UFPSBaseMembership>();
    auto* W = Body->GetWeapons();
    const FName Chosen = Server ? FName(TEXT("Weapon.AK")) : FName(TEXT("Weapon.UMP45"));
    auto RemoteBody = [&]() { return P.Remote.IsValid() ? Cast<AFPSDemoCharacter>(P.Remote->GetPawn()) : nullptr; };
    auto RawDamage = [&](AFPSDemoCharacter* Victim, float Magnitude, AFPSDemoCharacter* Source)
    {
        auto* ASC = Victim->GetAbilitySystemComponent();
        auto Context = Source ? Source->GetAbilitySystemComponent()->MakeEffectContext() : FGameplayEffectContextHandle(new FGameplayEffectContext());
        auto Spec = ASC->MakeOutgoingSpec(UFPSWeaponDamageEffect::StaticClass(), 1, Context);
        Spec.Data->SetSetByCallerMagnitude(FName(TEXT("WeaponDamage")), Magnitude); ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    };
    At(5, [&]()
    {
        P.Gun = W->GetLoadout().Primary.InstanceId; P.Secondary = W->GetLoadout().Secondary.InstanceId;
        Check(Membership && Membership->CanSelect() && W->GetLoadout().Primary.WeaponId == TEXT("Weapon.M4") && W->GetLoadout().Secondary.WeaponId == TEXT("Weapon.M9"), TEXT("uniform initial loadout and current-life permission"));
        if (!Training) Check(!Body->GetActionPolicy()->CanAttack() && !Body->GetActionPolicy()->CanDrop(), TEXT("base protection forbids attack and active drop"));
        if (Server && !Training)
        {
            for (auto It = World->GetPlayerControllerIterator(); It; ++It) if (It->Get() != PC) P.Remote = Cast<AFPSFrontController>(It->Get());
            if (auto* Victim = RemoteBody()) { P.OldBody = Victim; P.RemoteSpawn = Victim->GetActorLocation(); P.OldLife = Victim->FindComponentByClass<UFPSBaseMembership>()->GetLifeId(); }
            for (TActorIterator<AFPSBaseRegion> It(World); It; ++It)
                if (It->TeamId == Body->GetPlayerState<AFPSDemoPlayerState>()->TeamId) P.OwnRegion = *It; else P.OtherRegion = *It;
        }
    });
    At(10, [&]() { PC->RequestSelection(Chosen); });
    At(20, [&]()
    {
        Check(PC->LastSelectionSucceeded() && W->Equipped().WeaponId == Chosen && W->Equipped().InstanceId != P.Gun && W->GetLoadout().Secondary.InstanceId == P.Secondary, TEXT("RPC selection grants new instance equips and preserves other slot"));
        if (!Training) Check(PC->GetPlayerState<AFPSDemoPlayerState>()->SpawnPrimary() == Chosen, TEXT("successful selection is immediately saved per player"));
        P.Repeated = W->Equipped().InstanceId;
        if (Server && RemoteBody()) { RemoteBody()->GetWeapons()->Loadout.Primary.Magazine = 7; RemoteBody()->GetWeapons()->Changed(); }
    });
    At(28, [&]() { PC->RequestSelection(Chosen); });
    At(38, [&]()
    {
        Check(PC->LastSelectionSucceeded() && W->Equipped().InstanceId != P.Repeated && W->Equipped().Magazine == W->Definition()->InitialMagazine, TEXT("same-model supply creates fresh GUID and initial ammunition")); P.Repeated = W->Equipped().InstanceId;
        if (Server && !Training) Check(PC->GetPlayerState<AFPSDemoPlayerState>()->SpawnPrimary() == TEXT("Weapon.AK") && P.Remote->GetPlayerState<AFPSDemoPlayerState>()->SpawnPrimary() == TEXT("Weapon.UMP45"), TEXT("two players have independent preferences"));
    });
    At(40, [&]() { PC->RequestSelection(TEXT("Weapon.Unadmitted")); });
    At(48, [&]() { Check(!PC->LastSelectionSucceeded() && W->Equipped().InstanceId == P.Repeated, TEXT("illegal selection leaves inventory unchanged and gives failure feedback")); });
    At(50, [&]() { PC->InputKey(FInputKeyParams(EKeys::P,IE_Pressed,1.0)); });
    At(51, [&]() { PC->InputKey(FInputKeyParams(EKeys::P,IE_Released,0.0)); });
    At(56, [&]()
    {
        Check(PC->IsLoadoutOpen() && PC->bShowMouseCursor && PC->IsMoveInputIgnored() && PC->IsLookInputIgnored(), TEXT("P input opens selection menu owns mouse and suspends local game input"));
        W->StartFire(); Check(!W->GetLoadout().bFireActive, TEXT("menu cannot leave a held trigger"));
        if (FParse::Param(FCommandLine::Get(), TEXT("FPSLoadoutVisual"))) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/WindowsEditor/LoadoutMenu.png"), true, false);
    });
    if (Training)
    {
        At(60, [&]() { PC->InputKey(FInputKeyParams(EKeys::Escape,IE_Pressed,1.0)); });
        At(61, [&]() { PC->InputKey(FInputKeyParams(EKeys::Escape,IE_Released,0.0)); });
        At(65, [&]()
        {
            Check(!PC->IsLoadoutOpen() && !PC->IsMoveInputIgnored() && !PC->IsLookInputIgnored(), TEXT("Escape closes selection and restores game input"));
            Body->SetActorLocation(FVector(-400,-1000,100));
            Check(Membership->CanSelect(), TEXT("training selection remains available outside original spawn"));
            bool Preserved = false;
            for (TActorIterator<AFPSWeaponPickup> It(World); It; ++It) Preserved |= It->GetInstance().InstanceId == P.Gun;
            Check(Preserved, TEXT("training replacement retains original gun instance as dynamic drop"));
        });
        At(80, [&]() { PC->RequestSelection(TEXT("Weapon.HeavyPistol")); });
        At(90, [&]() { Check(PC->LastSelectionSucceeded() && W->Equipped().WeaponId == TEXT("Weapon.HeavyPistol") && W->GetLoadout().Primary.InstanceId == P.Repeated, TEXT("choosing other slot equips it and preserves primary")); });
        At(100, [&]() { Check(!P.Failed, TEXT("COMPLETE")); }); return;
    }
    if (Server)
    {
        At(60, [&]() { if (auto* Victim = RemoteBody()) { Victim->SetActorLocation(FVector(1000,-1200,100)); Victim->FindComponentByClass<UFPSBaseMembership>()->RefreshAuthority(); } });
        At(75, [&]()
        {
            auto* Victim = RemoteBody(); if (!Victim) { Check(false, TEXT("remote exists")); return; }
            Check(!Victim->FindComponentByClass<UFPSBaseMembership>()->CanSelect() && Victim->GetActionPolicy()->CanAttack() && Victim->GetActionPolicy()->CanDrop(), TEXT("first exit revokes selection and permits outside actions"));
            Victim->SetActorLocation(P.RemoteSpawn); Victim->FindComponentByClass<UFPSBaseMembership>()->RefreshAuthority();
        });
        At(85, [&]()
        {
            auto* Victim = RemoteBody(); auto* M = Victim->FindComponentByClass<UFPSBaseMembership>();
            Check(M->IsProtected() && !M->CanSelect(), TEXT("own-base return restores protection but not selection"));
            const float Before = Victim->GetHealth(); RawDamage(Victim,-25,Body);
            Check(Victim->GetHealth() == Before, TEXT("direct hostile GAS execution is blocked without depending on shot trace"));
            RawDamage(Victim,-60,Victim); RawDamage(Victim,35,Body);
            Check(Victim->GetHealth() == 75, TEXT("protection does not heal and permits positive health effects"));
            auto* ASC = Victim->GetAbilitySystemComponent();
            auto Periodic = Body->GetAbilitySystemComponent()->MakeOutgoingSpec(UFPSHostilePeriodicSmokeEffect::StaticClass(),1,Body->GetAbilitySystemComponent()->MakeEffectContext());
            P.Periodic = ASC->ApplyGameplayEffectSpecToSelf(*Periodic.Data.Get()); P.Health = Victim->GetHealth();
            P.ExtraRegion = World->SpawnActor<AFPSBaseRegion>(P.OtherRegion->GetActorLocation(), FRotator::ZeroRotator);
            P.ExtraRegion->TeamId = Victim->GetPlayerState<AFPSDemoPlayerState>()->TeamId;
            M->RefreshAuthority();
            auto Context = ASC->MakeEffectContext(); Context.AddSourceObject(PC);
            auto Spec = ASC->MakeOutgoingSpec(UFPSBaseProtectionEffect::StaticClass(),1,Context);
            P.External = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
            Check(ASC->GetTagCount(FGameplayTag::RequestGameplayTag(TEXT("Character.Protection.HostileDamage"))) == 3, TEXT("overlap regions and external effect retain separate sources"));
        });
        At(100, [&]()
        {
            auto* Victim = RemoteBody(); auto* ASC = Victim->GetAbilitySystemComponent();
            Check(Victim->GetHealth() == P.Health && ASC->GetActiveGameplayEffect(P.Periodic), TEXT("existing periodic hostile effect remains timed but cannot damage inside"));
            P.ExtraRegion->Destroy(); Victim->FindComponentByClass<UFPSBaseMembership>()->RefreshAuthority();
            Check(ASC->GetTagCount(FGameplayTag::RequestGameplayTag(TEXT("Character.Protection.HostileDamage"))) == 2, TEXT("leaving one source removes only its effect"));
            Victim->SetActorLocation(FVector(1000,-1200,100)); Victim->FindComponentByClass<UFPSBaseMembership>()->RefreshAuthority();
            Check(ASC->GetTagCount(FGameplayTag::RequestGameplayTag(TEXT("Character.Protection.HostileDamage"))) == 1, TEXT("last region exit preserves independent protection"));
            ASC->RemoveActiveGameplayEffect(P.External);
        });
        At(115, [&]()
        {
            auto* Victim = RemoteBody();
            Check(Victim->GetHealth() < P.Health && Victim->GetAbilitySystemComponent()->GetActiveGameplayEffect(P.Periodic), TEXT("periodic hostile damage resumes outside before natural expiry"));
            // Teleport into the opposing base must be expelled, including a stationary movement update.
            Victim->SetActorLocation(P.OwnRegion->GetActorLocation()); Victim->FindComponentByClass<UFPSBaseMembership>()->EnforceMovement(Victim->GetActorLocation());
            Check(!P.OwnRegion->Contains(Victim->GetActorLocation(),38), TEXT("enemy exceptional teleport is expelled from forbidden base"));
            const FVector Previous(-1500,-1300,100);
            Victim->SetActorLocation(FVector(-2700,-1300,100)); Victim->FindComponentByClass<UFPSBaseMembership>()->EnforceMovement(Previous);
            Check(Victim->GetActorLocation().X > -1740 && !P.OwnRegion->Contains(Victim->GetActorLocation()), TEXT("swept passage cannot tunnel across enemy region"));
            Victim->SetActorLocation(FVector(1000,-1200,100));
            FVector End(-2500,0,164); const bool Clip = UFPSBaseMembership::ClipAttack(World,FVector(-1500,0,164),End);
            Check(Clip && End.X > -2400 && End.X < -1700, TEXT("outside-to-outside ray crossing a base is blocked"));
        });
        At(125, [&]()
        {
            PC->HideLoadout(); Body->SetActorLocation(FVector(-1200,-1200,100)); Membership->RefreshAuthority();
            W->StartFire();
        });
        At(133, [&]() { Body->SetActorLocation(P.OwnRegion->GetActorLocation()); Membership->RefreshAuthority(); P.Repeated = W->Equipped().InstanceId; P.Health = W->Equipped().Magazine; });
        At(140, [&]() { Body->SetActorLocation(FVector(-1200,-1200,100)); Membership->RefreshAuthority(); });
        At(150, [&]() { Check(!W->GetLoadout().bFireActive && W->Equipped().Magazine == P.Health, TEXT("entering cancels attack and leaving cannot resume old trigger")); });
        At(153, [&]() { W->Drop(); });
        At(158, [&]()
        {
            Check(!W->GetLoadout().Primary.IsValid() && PC->GetPlayerState<AFPSDemoPlayerState>()->SpawnPrimary() == TEXT("Weapon.AK"), TEXT("active drop changes inventory but not saved preference"));
            W->Pickup();
        });
        At(160, [&]()
        {
            auto* Victim = RemoteBody(); Victim->GetAbilitySystemComponent()->RemoveActiveGameplayEffect(P.Periodic);
            Victim->SetActorLocation(P.RemoteSpawn); Victim->FindComponentByClass<UFPSBaseMembership>()->RefreshAuthority();
            P.VictimGun = Victim->GetWeapons()->Equipped().InstanceId;
            RawDamage(Victim,-1000,nullptr);
            Check(Victim->IsEliminated(), TEXT("life can end before exiting after a saved selection"));
        });
        At(170, [&]() { Check(W->GetLoadout().Primary.InstanceId == P.Repeated && PC->GetPlayerState<AFPSDemoPlayerState>()->SpawnPrimary() == TEXT("Weapon.AK"), TEXT("pickup recovers instance without rewriting saved preference")); });
        At(195, [&]()
        {
            auto* Victim = RemoteBody();
            Check(Victim && Victim != P.OldBody.Get() && Victim->GetWeapons()->Equipped().WeaponId == TEXT("Weapon.UMP45") && Victim->GetWeapons()->Equipped().InstanceId != P.VictimGun && Victim->GetWeapons()->Equipped().Magazine == Victim->GetWeapons()->Definition()->InitialMagazine,
                TEXT("respawn reads saved model with fresh GUID and initial ammunition"));
            Check(Victim && Victim->FindComponentByClass<UFPSBaseMembership>()->CanSelect(), TEXT("new life receives new base selection permission"));
        });
        At(220, [&]()
        {
            auto* Victim = RemoteBody(); const FGuid Before = Victim->GetWeapons()->Equipped().InstanceId;
            P.Remote->ServerSelectWeapon(Victim,P.OldLife,TEXT("Weapon.AK"),999);
            Check(Victim->GetWeapons()->Equipped().InstanceId == Before && Victim->GetPlayerState<AFPSDemoPlayerState>()->SpawnPrimary() == TEXT("Weapon.UMP45"), TEXT("old-life selection payload cannot mutate new life"));
            // New session identity starts with defaults; no account/local persistence participates.
            auto* Fresh = World->SpawnActor<AFPSDemoPlayerState>(); Fresh->EnsureLoadout(UFPSModeRules::ForWorld(World));
            Check(Fresh->SpawnPrimary() == TEXT("Weapon.M4") && Fresh->SpawnSecondary() == TEXT("Weapon.M9"), TEXT("fresh match identity initializes uniform defaults")); Fresh->Destroy();
        });
        At(230, [&]()
        {
            auto* Victim = RemoteBody();
            FTransform Fallback;
            const int32 Attempts = P.OtherRegion->SpawnAttempts; P.OtherRegion->SpawnAttempts = 0;
            Check(P.OtherRegion->FindSpawn(P.Remote.Get(),Fallback), TEXT("configured fallback passes the same floor capsule and spacing checks"));
            P.OtherRegion->SpawnAttempts = Attempts;
            P.SpawnBlocker = World->SpawnActor<AActor>();
            auto* Box = NewObject<UBoxComponent>(P.SpawnBlocker.Get());
            P.SpawnBlocker->SetRootComponent(Box); Box->SetBoxExtent(P.OtherRegion->Bounds->GetUnscaledBoxExtent() + FVector(150));
            Box->SetCollisionObjectType(ECC_WorldDynamic); Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            Box->SetCollisionResponseToAllChannels(ECR_Ignore); Box->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);
            Box->RegisterComponent(); P.SpawnBlocker->SetActorLocation(P.OtherRegion->GetActorLocation());
            FTransform Rejected;
            Check(!P.OtherRegion->FindSpawn(P.Remote.Get(),Rejected), TEXT("occupied sampling and occupied fallback both reject spawn"));
            P.Remote->UnPossess(); Victim->Destroy();
            World->GetAuthGameMode<AFPSMatchMode>()->RestartPlayer(P.Remote.Get());
        });
        At(243, [&]() { Check(!P.Remote->GetPawn(), TEXT("occupied base leaves controller waiting instead of forcing a spawn")); P.SpawnBlocker->Destroy(); });
        At(265, [&]()
        {
            auto* Victim = RemoteBody();
            Check(Victim && P.OtherRegion->Contains(Victim->GetActorLocation()) && Victim->GetWeapons()->Equipped().WeaponId == TEXT("Weapon.UMP45"), TEXT("server retry spawns valid fresh saved loadout after obstruction clears"));
        });
        At(280, [&]()
        {
            auto* Victim = RemoteBody(); Victim->SetActorLocation(FVector(-1500,-1300,100));
            Victim->GetController()->SetControlRotation(FRotator(0,180,0));
        });
        At(325, [&]()
        {
            auto* Victim = RemoteBody();
            Check(Victim && Victim->GetActorLocation().X < -1620 && Victim->GetActorLocation().X > -1740 && !P.OwnRegion->Contains(Victim->GetActorLocation()), TEXT("actual client movement RPC stops at enemy base on server"));
            P.OldBody = Victim;
        });
        At(355, [&]() { Check(!P.Remote.IsValid() && !P.OldBody.IsValid(), TEXT("real client disconnect destroys pawn and its region effect sources")); });
    }
    else
    {
        At(69, [&]() { Check(!PC->IsLoadoutOpen() && !PC->IsMoveInputIgnored() && !Membership->CanSelect(), TEXT("authoritative exit closes owner menu and revokes permission")); PC->ServerSelectWeapon(Body,Membership->GetLifeId(),TEXT("Weapon.AK"),7); });
        At(82, [&]() { Check(Membership->IsProtected() && !Membership->CanSelect() && !PC->IsLoadoutOpen(), TEXT("return protection replicates without reopening selection")); });
        At(98, [&]() { Check(W->Equipped().WeaponId == TEXT("Weapon.UMP45") && !PC->LastSelectionSucceeded() && Body->GetHealth() == 75, TEXT("late outside request rejected with inventory and health protection replicated")); });
        At(172, [&]() { Check(Body->IsEliminated() && !PC->IsLoadoutOpen(), TEXT("death clears owner UI")); });
        At(200, [&]() { Check(!Body->IsEliminated() && W->Equipped().WeaponId == TEXT("Weapon.UMP45") && Membership->CanSelect() && PC->GetPlayerState<AFPSDemoPlayerState>()->SpawnPrimary() == TEXT("Weapon.UMP45"), TEXT("owner receives saved fresh respawn and renewed life permission")); });
        At(270, [&]() { Check(!Body->IsEliminated() && W->Equipped().WeaponId == TEXT("Weapon.UMP45") && Membership->CanSelect(), TEXT("owner receives deferred occupied-base respawn")); });
        if (Time >= 29 && Time < 31.5) Body->MoveForward(FInputActionValue(1.f));
        At(326, [&]() { Check(Body->GetActorLocation().X < -1620 && Body->GetActorLocation().X > -1740, TEXT("owner predicts enemy boundary without crossing it")); });
        At(335, [&]()
        {
            Check(!P.Failed, TEXT("COMPLETE"));
            FTimerHandle Disconnect;
            World->GetTimerManager().SetTimer(Disconnect, FTimerDelegate::CreateWeakLambda(PC,[PC]() { PC->ConsoleCommand(TEXT("disconnect")); }), .2f, false);
        });
    }
    if (Server) At(360, [&]() { Check(!P.Failed, TEXT("COMPLETE")); });
#endif
}
