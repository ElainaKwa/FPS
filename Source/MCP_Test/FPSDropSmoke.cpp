#include "FPSDropSmoke.h"
#include "FPSMatchFlow.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "FPSDemoEffects.h"
#include "FPSBaseMembership.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponAbilities.h"
#include "FPSWeaponPickup.h"
#include "FPSDynamicDrops.h"
#include "FPSModeRules.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerInput.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Net/UnrealNetwork.h"
#include "UnrealClient.h"

AFPSDropSmokeState::AFPSDropSmokeState()
{
    bReplicates = true; bAlwaysRelevant = true; SetNetUpdateFrequency(20);
}
AFPSDropSmokeWall::AFPSDropSmokeWall()
{
    bReplicates = true; bAlwaysRelevant = true; SetReplicateMovement(true);
    auto* Wall = CreateDefaultSubobject<UBoxComponent>(TEXT("SmokeSceneWall"));
    SetRootComponent(Wall); Wall->InitBoxExtent(FVector(15,120,200));
    Wall->SetCollisionObjectType(ECC_WorldStatic); Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Wall->SetCollisionResponseToAllChannels(ECR_Block);
}
void AFPSDropSmokeState::OnFixtureHit(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, FVector, const FHitResult&)
{
    if (HasAuthority() && OtherActor && OtherActor->IsA<AFPSDropSmokeWall>()) { bWallHit = true; ForceNetUpdate(); }
}
void AFPSDropSmokeState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AFPSDropSmokeState, Epoch);
    DOREPLIFETIME(AFPSDropSmokeState, CooldownReference);
    DOREPLIFETIME(AFPSDropSmokeState, SettledGuid);
    DOREPLIFETIME(AFPSDropSmokeState, SettledPosition);
    DOREPLIFETIME(AFPSDropSmokeState, bSettled);
    DOREPLIFETIME(AFPSDropSmokeState, bWallHit);
    DOREPLIFETIME(AFPSDropSmokeState, bLateReady);
    DOREPLIFETIME(AFPSDropSmokeState, RaceGuid);
    DOREPLIFETIME(AFPSDropSmokeState, bRaceResolved);
    DOREPLIFETIME(AFPSDropSmokeState, bClientWonRace);
    DOREPLIFETIME(AFPSDropSmokeState, SurvivingGuid);
    DOREPLIFETIME(AFPSDropSmokeState, DyingGuid);
    DOREPLIFETIME(AFPSDropSmokeState, LifetimeGuid);
    DOREPLIFETIME(AFPSDropSmokeState, ReacquireGuid);
    DOREPLIFETIME(AFPSDropSmokeState, ReacquireDropped);
    DOREPLIFETIME(AFPSDropSmokeState, ReacquireReference);
    DOREPLIFETIME(AFPSDropSmokeState, CoalescedReference);
    DOREPLIFETIME(AFPSDropSmokeState, CoalescedRevision);
    DOREPLIFETIME(AFPSDropSmokeState, PresetCount);
}
void FFPSDropSmoke::Start(AFPSMatchMode* Mode)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSDropSmoke"))) return;
    FTimerHandle Timer;
    Mode->GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(Mode, [Mode]()
    {
        auto* GS = Mode->GetGameState<AFPSDemoGameState>();
        if (!GS || !GS->WeaponDataError.IsEmpty()) return;
        if (GS->bWaiting)
        {
            if (GS->PlayerArray.Num() != 2) return;
            for (auto It = Mode->GetWorld()->GetPlayerControllerIterator(); It; ++It)
                if (!CastChecked<AFPSFrontController>(It->Get())->IsWeaponContentVerified()) return;
            Mode->StartRound(Mode->GetWorld()->GetFirstPlayerController());
        }
        if (GS->bWaiting) return;
        // Give the owning rendered client time to finish viewport/scene startup.
        // This opt-in epoch delay changes only the fixture, never the round rules.
        static TMap<TWeakObjectPtr<UWorld>, double> ReadyAt;
        double& Ready = ReadyAt.FindOrAdd(Mode->GetWorld());
        if (Ready == 0) Ready = GS->GetServerWorldTimeSeconds();
        if (GS->GetServerWorldTimeSeconds() - Ready < 10) return;
        for (TActorIterator<AFPSDropSmokeState> It(Mode->GetWorld()); It; ++It) return;
        auto* State = Mode->GetWorld()->SpawnActor<AFPSDropSmokeState>();
        State->Epoch = GS->GetServerWorldTimeSeconds();
        for (TActorIterator<AFPSWeaponPickup> It(Mode->GetWorld()); It; ++It)
            if (!It->IsDynamic()) ++State->PresetCount;
        State->ForceNetUpdate();
    }), .2f, true);
#endif
}
void FFPSDropSmoke::Tick(AFPSFrontController* PC)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSDropSmoke")) || !PC->IsLocalController()) return;
    auto* World = PC->GetWorld(); auto* GS = World->GetGameState<AFPSDemoGameState>();
    auto* Body = Cast<AFPSDemoCharacter>(PC->GetPawn());
    if (!GS || GS->bWaiting || !Body || !PC->IsWeaponContentVerified()) return;
    AFPSDropSmokeState* State = nullptr;
    for (TActorIterator<AFPSDropSmokeState> It(World); It; ++It) { State = *It; break; }
    if (!State || State->Epoch <= 0) return;
    const bool Server = PC->HasAuthority(), Training = GS->bTraining;
    const double Time = GS->GetServerWorldTimeSeconds() - State->Epoch;
    if (FParse::Param(FCommandLine::Get(),TEXT("FPSDropObserver")))
    {
        static TSet<TWeakObjectPtr<UWorld>> Finished;
        if (Finished.Contains(World) || Server) return;
        AFPSWeaponPickup* Drop = nullptr;
        if (State->bLateReady) for (TActorIterator<AFPSWeaponPickup> It(World); It; ++It)
            if (It->GetInstance().InstanceId == State->SettledGuid) { Drop = *It; break; }
        if (!Drop && Time < 74) return;
        const float Error = Drop ? FVector::Dist(Drop->GetActorLocation(),State->SettledPosition) : FLT_MAX;
        const bool Pass = Drop && Error < 25 && Drop->FindComponentByClass<UBoxComponent>()->IsSimulatingPhysics();
        if (!Pass && Time < 74) return; // Wait for the initial actor and rigid-body snapshot to converge.
        UE_LOG(LogTemp,Display,TEXT("FPS_DROP_OBSERVER MEASURE CLIENT rest_error_cm=%.3f server=(%s) client=(%s)"),Error,*State->SettledPosition.ToCompactString(),Drop ? *Drop->GetActorLocation().ToCompactString() : TEXT("missing"));
        UE_LOG(LogTemp,Display,TEXT("FPS_DROP_OBSERVER %s CLIENT COMPLETE late join resting physics snapshot"),Pass ? TEXT("PASS") : TEXT("FAIL"));
        Finished.Add(World); return;
    }
    struct FProgress
    {
        TSet<int32> Done; bool Failed = false;
        TWeakObjectPtr<AFPSFrontController> Remote;
        TWeakObjectPtr<AFPSDemoCharacter> OldBody;
        TArray<TWeakObjectPtr<AFPSWeaponPickup>> Presets;
        TWeakObjectPtr<AFPSWeaponPickup> Settled, KillZ, LifeDrop;
        TWeakObjectPtr<AFPSDropSmokeWall> Wall;
        FGuid BeforeRace; FVector LaunchPosition; int32 DeathCount = 0;
        bool bPreviousBoundsChecks = false;
        FGuid CoalescedSecondary;
        uint32 CoalescedSecondaryRevision = 0;
    };
    static TMap<TWeakObjectPtr<UWorld>, FProgress> Progress;
    auto& P = Progress.FindOrAdd(World);
    auto Check = [&](bool Pass, const TCHAR* Name)
    {
        P.Failed |= !Pass;
        UE_LOG(LogTemp, Display, TEXT("FPS_DROP_SMOKE %s %s %s time=%.2f"), Pass ? TEXT("PASS") : TEXT("FAIL"), Server ? TEXT("SERVER") : TEXT("CLIENT"), Name, Time);
    };
    auto At = [&](int32 Tenth, TFunction<void()> Action)
    { if (Time >= Tenth * .1 && !P.Done.Contains(Tenth)) { P.Done.Add(Tenth); Action(); } };
    auto Find = [&](FGuid Id) -> AFPSWeaponPickup*
    {
        if (!Id.IsValid()) return nullptr;
        for (TActorIterator<AFPSWeaponPickup> It(World); It; ++It)
            if (!It->IsActorBeingDestroyed() && It->GetInstance().InstanceId == Id) return *It;
        return nullptr;
    };
    auto DynamicCount = [&]()
    {
        int32 Count = 0;
        for (TActorIterator<AFPSWeaponPickup> It(World); It; ++It) if (It->IsDynamic() && !It->IsActorBeingDestroyed()) ++Count;
        return Count;
    };
    auto PresetCount = [&]()
    {
        int32 Count = 0;
        for (TActorIterator<AFPSWeaponPickup> It(World); It; ++It) if (!It->IsDynamic() && !It->IsActorBeingDestroyed()) ++Count;
        return Count;
    };
    auto RemoteBody = [&]() -> AFPSDemoCharacter*
    { return Training ? Cast<AFPSDemoCharacter>(PC->GetPawn()) : P.Remote.IsValid() ? Cast<AFPSDemoCharacter>(P.Remote->GetPawn()) : nullptr; };
    auto* Registry = World->GetSubsystem<UFPSDynamicDrops>(); auto* W = Body->GetWeapons();
    const FVector Ground(0, -450, 100); // Clear floor in both maps, away from training dividers.
    auto Relocate = [](AFPSDemoCharacter* Pawn, FVector Position, bool Flying)
    {
        if (!Pawn) return;
        Pawn->GetWeapons()->StopAllActions();
        Pawn->GetCharacterMovement()->StopMovementImmediately();
        Pawn->GetCharacterMovement()->SetMovementMode(Flying ? MOVE_Flying : MOVE_Walking);
        Pawn->SetActorLocation(Position);
        Pawn->SetActorRotation(FRotator::ZeroRotator);
        if (Pawn->GetController()) Pawn->GetController()->SetControlRotation(FRotator::ZeroRotator);
        Pawn->FindComponentByClass<UFPSBaseMembership>()->RefreshAuthority();
        Pawn->ForceNetUpdate();
    };
    auto Spawn = [&](FVector Position, FVector Velocity, FName Weapon = FName(TEXT("Weapon.M4"))) -> AFPSWeaponPickup*
    {
        auto* Drop = World->SpawnActor<AFPSWeaponPickup>(Position, FRotator::ZeroRotator);
        if (!Drop) return nullptr;
        Drop->Initialize(W->Catalog()->CreateInstance(Weapon));
        if (!Drop->LaunchDynamic(Velocity, FVector(90,140,50))) { Drop->Destroy(); return nullptr; }
        return Drop;
    };
    auto SameInstance = [](const FFPSWeaponInstance& A, const FFPSWeaponInstance& B)
    {
        return A.InstanceId == B.InstanceId && A.WeaponId == B.WeaponId && A.Magazine == B.Magazine && A.Reserve == B.Reserve &&
            A.NextShotAt == B.NextShotAt && A.LastShotAt == B.LastShotAt && A.RecoilIndex == B.RecoilIndex && A.ShotSerial == B.ShotSerial;
    };
    auto Copies = [&](FGuid Id)
    {
        int32 Count = Find(Id) ? 1 : 0;
        for (TActorIterator<AFPSDemoCharacter> It(World); It; ++It)
            for (const auto* Gun : {&It->GetWeapons()->GetLoadout().Primary, &It->GetWeapons()->GetLoadout().Secondary})
                if (Gun->IsValid() && Gun->InstanceId == Id) ++Count;
        return Count;
    };
    At(5, [&]()
    {
        const auto* Rules = UFPSModeRules::ForWorld(World);
        Check(Rules && Rules->DynamicDropLimit == 30 && FMath::IsNearlyEqual(Rules->DynamicDropLifetime,180.f) && FMath::IsNearlyEqual(Rules->SelectionRequestInterval,.1f), TEXT("mode policy cap=30 lifetime=180 interval=.1"));
        if (Server)
        {
            for (TActorIterator<AFPSWeaponPickup> It(World); It; ++It) if (!It->IsDynamic()) P.Presets.Add(*It);
            if (!Training) for (auto It = World->GetPlayerControllerIterator(); It; ++It)
                if (It->Get() != PC) P.Remote = Cast<AFPSFrontController>(It->Get());
        }
        Check(Body->GetAbilitySystemComponent()->GetActivatableAbilities().Num() >= 10, TEXT("real GAS weapon abilities available"));
    });
    if (FParse::Param(FCommandLine::Get(), TEXT("FPSDropLifetimeSmoke")))
    {
        if (Server)
        {
            At(10, [&]()
            {
                P.LifeDrop = Spawn(Ground + FVector(250,200,150), FVector::ZeroVector);
                Check(P.LifeDrop.IsValid(), TEXT("real 180-second lifetime starts without time dilation"));
                if (P.LifeDrop.IsValid()) State->LifetimeGuid = P.LifeDrop->GetInstance().InstanceId;
                State->ForceNetUpdate();
            });
            At(1790, [&]() { Check(P.LifeDrop.IsValid() && Find(State->LifetimeGuid) && P.LifeDrop->GetLifeSpan() > 0 && P.LifeDrop->GetLifeSpan() < 3, TEXT("drop remains alive shortly before real lifetime deadline")); });
            At(1820, [&]() { Check(!P.LifeDrop.IsValid() && !Find(State->LifetimeGuid) && Registry->Count() == 0 && PresetCount() == State->PresetCount, TEXT("natural 180-second expiry removes registry entry and retains presets")); });
        }
        else
        {
            At(30, [&]() { Check(Find(State->LifetimeGuid) && Find(State->LifetimeGuid)->IsDynamic(), TEXT("client receives lifetime drop")); });
            At(1790, [&]() { Check(Find(State->LifetimeGuid) != nullptr, TEXT("client retains drop before lifetime deadline")); });
            At(1830, [&]() { Check(!Find(State->LifetimeGuid) && PresetCount() == State->PresetCount, TEXT("client receives natural expiry destruction without losing presets")); });
        }
        At(1850, [&]() { Check(!P.Failed, TEXT("COMPLETE")); }); return;
    }
    if (Server)
    {
        At(7, [&]()
        {
            const FGuid Original = W->Equipped().InstanceId;
            PC->RequestSelection(TEXT("Weapon.AK")); const FGuid Granted = W->Equipped().InstanceId;
            const uint32 GrantedRevision = W->Loadout.PrimaryInventoryRevision;
            PC->RequestSelection(TEXT("Weapon.UMP45"));
            Check(Granted != Original && W->Equipped().WeaponId == TEXT("Weapon.AK") && W->Equipped().InstanceId == Granted && !PC->LastSelectionSucceeded(), TEXT(".1-second selection gate rejects second request without another grant"));
            Check(W->Loadout.PrimaryInventoryRevision == GrantedRevision,TEXT("rate-limited selection preserves ownership revision"));
        });
        At(10, [&]()
        {
            Registry->ClearRound();
            Relocate(Body,Ground + FVector(0,0,270),true);
            if (!Training) Relocate(RemoteBody(),Ground + FVector(130,0,270),true);
            P.LaunchPosition = Ground + FVector(300,180,250);
            P.Wall = World->SpawnActor<AFPSDropSmokeWall>(P.LaunchPosition+FVector(150,0,-150),FRotator::ZeroRotator);
            P.Settled = Spawn(P.LaunchPosition,FVector(300,0,40));
            Check(P.Settled.IsValid(), TEXT("physical fixture launched on server"));
            if (P.Settled.IsValid())
            {
                State->SettledGuid = P.Settled->GetInstance().InstanceId;
                auto* Primitive = P.Settled->FindComponentByClass<UBoxComponent>();
                Primitive->SetNotifyRigidBodyCollision(true);
                Primitive->OnComponentHit.AddDynamic(State,&AFPSDropSmokeState::OnFixtureHit);
                Check(Primitive && Primitive->IsSimulatingPhysics() && Primitive->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Block && Primitive->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Block &&
                    Primitive->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore && Primitive->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Ignore, TEXT("scene collisions enabled while pawn and bullet query are ignored"));
                FVector Center,Extent; P.Settled->GetActorBounds(true,Center,Extent);
                FHitResult Visibility,PawnHit; FCollisionQueryParams Query(SCENE_QUERY_STAT(DropSmokePassage),false,Body);
                const FVector Span(Extent.X+5,0,0);
                const bool BulletBlocked = World->LineTraceSingleByChannel(Visibility,Center-Span,Center+Span,ECC_Visibility,Query);
                const bool PawnBlocked = World->SweepSingleByChannel(PawnHit,Center-Span,Center+Span,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(38,88),Query);
                Check(!BulletBlocked && !PawnBlocked,TEXT("actual visibility bullet trace and capsule pawn sweep pass through airborne gun"));
            }
        });
        At(15, [&]()
        {
            auto& Gun = W->Loadout.Primary; Gun.Magazine = 7; Gun.Reserve = 19;
            Gun.NextShotAt = W->ServerTime() + 8; Gun.LastShotAt = W->ServerTime(); Gun.RecoilIndex = 5; Gun.ShotSerial = 33;
            State->CooldownReference = Gun; State->ForceNetUpdate(); W->Changed();
        });
        At(20, [&]()
        {
            W->Drop(); auto* Drop = Find(State->CooldownReference.InstanceId);
            UE_LOG(LogTemp,Display,TEXT("FPS_DROP_SMOKE MEASURE SERVER launch pawn=(%s) yaw=%.2f drop=(%s)"),*Body->GetActorLocation().ToCompactString(),Body->GetActorRotation().Yaw,Drop ? *Drop->GetActorLocation().ToCompactString() : TEXT("missing"));
            Check(!W->GetLoadout().Primary.IsValid() && Drop && Drop->IsDynamic() && SameInstance(Drop->GetInstance(),State->CooldownReference), TEXT("actual Drop ability transfers complete instance into physical actor"));
        });
        At(24, [&]()
        {
            if (Training)
            {
                auto* Drop = Find(State->CooldownReference.InstanceId);
                Check(Drop && Drop->GetActorLocation().Z > Ground.Z + 80 && W->CanPickup(Drop), TEXT("training gun is legal to actively pick while airborne"));
                W->Request(EFPSWeaponRequest::Pickup,Drop);
            }
        });
        At(35, [&]()
        {
            auto* Recipient = RemoteBody();
            Check(Recipient && SameInstance(Recipient->GetWeapons()->GetLoadout().Primary,State->CooldownReference) && !Find(State->CooldownReference.InstanceId) && Copies(State->CooldownReference.InstanceId) == 1,
                Training ? TEXT("same-owner airborne GAS pickup retains all instance fields") : TEXT("remote GAS RPC pickup retains GUID ammo and original cooldown across holders"));
            if (Training) W->Primary();
        });
        At(40, [&]() { if (Training) { PC->SetControlRotation(FRotator(45,90,0)); W->StartFire(); } });
        At(55, [&]()
        {
            auto* Recipient = RemoteBody(); auto* RW = Recipient ? Recipient->GetWeapons() : nullptr;
            Check(RW && RW->Equipped().InstanceId == State->CooldownReference.InstanceId && RW->Equipped().Magazine == 7 && RW->Equipped().NextShotAt == State->CooldownReference.NextShotAt && RW->GetLoadout().Action == EFPSWeaponAction::Idle,
                TEXT("completed new-holder equip cannot bypass instance firing deadline"));
        });
        At(85, [&]() { auto* Recipient = RemoteBody(); Check(Recipient && Recipient->GetWeapons()->Equipped().Magazine == 7, TEXT("held fire still consumes no ammunition before original deadline")); });
        At(102, [&]()
        {
            auto* Recipient = RemoteBody(); auto* RW = Recipient ? Recipient->GetWeapons() : nullptr;
            Check(RW && RW->Equipped().Magazine < 7 && RW->Equipped().NextShotAt > State->CooldownReference.NextShotAt && RW->Equipped().LastShotAt >= State->CooldownReference.NextShotAt - .003,
                TEXT("real held-fire ability resumes only after original deadline"));
            if (Training) W->StopFire();
        });
        At(110, [&]()
        {
            if (auto* Drop = P.Settled.Get())
            {
                auto* Primitive = Drop->FindComponentByClass<UBoxComponent>();
                FVector Center,Extent; Drop->GetActorBounds(true,Center,Extent);
                FHitResult Floor; FCollisionQueryParams Query(SCENE_QUERY_STAT(DropSmokeFloor),false,Drop);
                World->LineTraceSingleByChannel(Floor,Center,Center-FVector(0,0,150),ECC_Visibility,Query);
                Check(Primitive && !Primitive->IsAnyRigidBodyAwake() && Drop->NetDormancy == DORM_DormantAll && FVector::Dist(Drop->GetActorLocation(),P.LaunchPosition) > 100 && Floor.bBlockingHit && Center.Z-Extent.Z-Floor.ImpactPoint.Z < 35 && State->bWallHit,
                    TEXT("server actual scene-wall contact and floor landing reach resting dormancy"));
                State->SettledPosition = Drop->GetActorLocation(); State->bSettled = true; State->ForceNetUpdate();
            }
            else Check(false,TEXT("settled fixture survived until measurement"));
            Relocate(Body,Ground,false); if (!Training) Relocate(RemoteBody(),Ground + FVector(130,0,0),false);
        });
        At(115, [&]()
        {
            auto* Drop = Spawn(Ground + FVector(65,0,40),FVector(0,0,180),TEXT("Weapon.UMP45"));
            Check(Drop != nullptr,TEXT("shared race pickup exists for both players"));
            if (Drop) { State->RaceGuid = Drop->GetInstance().InstanceId; State->RaceCandidate = Drop; State->ForceNetUpdate(); }
            P.BeforeRace = W->GetLoadout().Primary.InstanceId;
        });
        At(128, [&]()
        {
            // Keep the nominated candidate even if the earlier remote RPC already destroyed it.
            // A null public Request would auto-select a different nearby gun and spoil the race.
            FGameplayEventData Event; Event.OptionalObject = State->RaceCandidate.Get();
            Body->GetAbilitySystemComponent()->HandleGameplayEvent(FGameplayTag::RequestGameplayTag(TEXT("Input.Weapon.Pickup")),&Event);
            UE_LOG(LogTemp,Display,TEXT("FPS_DROP_SMOKE REQUEST SERVER contested_guid=%s"),*State->RaceGuid.ToString());
        });
        At(135, [&]()
        {
            const bool Host = W->GetLoadout().Primary.InstanceId == State->RaceGuid;
            const bool Client = !Training && RemoteBody() && RemoteBody()->GetWeapons()->GetLoadout().Primary.InstanceId == State->RaceGuid;
            Check(State->RaceGuid.IsValid() && !Find(State->RaceGuid) && Copies(State->RaceGuid) == 1 && (Training ? Host : Host != Client), TEXT("two actual GAS pickup requests produce one owner and one GUID"));
            State->bClientWonRace = Client; State->bRaceResolved = true; State->ForceNetUpdate();
            State->RaceCandidate = nullptr;
        });
        At(140, [&]()
        {
            // The losing host can still have an empty primary from the earlier cross-holder drop.
            // Seed the explicit full-slot precondition before testing replacement at a full cap.
            if (!W->Loadout.Primary.IsValid()) { W->Loadout.Primary = W->Catalog()->CreateInstance(TEXT("Weapon.M4")); W->Changed(); }
            W->Secondary();
            // Later host queries must not accidentally hit the opposing player's capsule.
            // This flank floor is clear of the Foundry cargo and low-cover geometry.
            if (!Training) Relocate(RemoteBody(),Ground+FVector(0,-750,0),false);
        });
        At(145, [&]()
        {
            Registry->ClearRound(); if (P.Wall.IsValid()) P.Wall->Destroy(); TArray<TWeakObjectPtr<AFPSWeaponPickup>> Batch;
            for (int32 Index=0;Index<30;++Index) Batch.Add(Spawn(Ground+FVector(700,Index*22,200),FVector::ZeroVector,TEXT("Weapon.AK")));
            Check(Batch.Num() == 30 && Batch[0].IsValid() && Registry->Count() == 30 && PresetCount() == State->PresetCount, TEXT("thirty dynamic drops exclude all map presets from cap"));
            auto* Newest = Spawn(Ground+FVector(700,700,200),FVector::ZeroVector,TEXT("Weapon.AK"));
            Check(Newest && !Batch[0].IsValid() && Batch[1].IsValid() && Registry->Count() == 30, TEXT("thirty-first dynamic drop retires exactly oldest available actor"));
            auto* Oldest = Batch[1].Get();
            if (!Oldest) { Check(false,TEXT("oldest full-cap candidate retained")); return; }
            Oldest->SetActorLocation(Body->GetActorLocation()+FVector(90,0,30));
            const FGuid Acquired = Oldest->GetInstance().InstanceId, Replaced = W->GetLoadout().Primary.InstanceId;
            const auto Active = W->GetLoadout().ActiveSlot; const FGuid Equipped = W->Equipped().InstanceId; const double EquipTime = W->GetLoadout().EquipStartedAt;
            const uint32 PrimaryRevision = W->Loadout.PrimaryInventoryRevision, SecondaryRevision = W->Loadout.SecondaryInventoryRevision;
            W->Request(EFPSWeaponRequest::Pickup,Oldest);
            Check(W->GetLoadout().Primary.InstanceId == Acquired && Copies(Acquired) == 1 && Copies(Replaced) == 1 && Registry->Count() == 30 && Batch[2].IsValid(), TEXT("claim oldest before full-slot replacement cap keeps transfer atomic"));
            Check(Active == EFPSWeaponSlot::Secondary && W->GetLoadout().ActiveSlot == Active && W->Equipped().InstanceId == Equipped && W->GetLoadout().EquipStartedAt == EquipTime, TEXT("ordinary non-current pickup keeps active slot and does not re-equip unchanged held gun"));
            Check(W->Loadout.PrimaryInventoryRevision == PrimaryRevision + 1 && W->Loadout.SecondaryInventoryRevision == SecondaryRevision,
                TEXT("non-current pickup increments only destination slot ownership revision"));
        });
        At(160, [&]()
        {
            Registry->ClearRound();
            auto* Target = World->SpawnActor<AFPSWeaponPickup>(Body->GetActorLocation()+FVector(100,0,20),FRotator::ZeroRotator);
            Target->Initialize(W->Catalog()->CreateInstance(TEXT("Weapon.AK")));
            auto* Blocker = World->SpawnActor<AActor>(); auto* Box = NewObject<UBoxComponent>(Blocker);
            Blocker->SetRootComponent(Box); Box->SetBoxExtent(FVector(180)); Box->SetCollisionObjectType(ECC_WorldStatic);
            Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Box->SetCollisionResponseToAllChannels(ECR_Block);
            Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore); Box->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
            Box->RegisterComponent(); Blocker->SetActorLocation(Body->GetActorLocation());
            const auto Before = W->GetLoadout().Primary;
            const uint32 PrimaryRevision = W->Loadout.PrimaryInventoryRevision, SecondaryRevision = W->Loadout.SecondaryInventoryRevision;
            Check(W->CanPickup(Target),TEXT("rollback fixture reaches claim with clear pickup query"));
            W->Request(EFPSWeaponRequest::Pickup,Target);
            Check(SameInstance(Before,W->GetLoadout().Primary) && Target->IsAvailable() && Registry->Count() == 0 && Copies(Target->GetInstance().InstanceId) == 1,
                TEXT("failed safe replacement drop releases claim and preserves both inventories"));
            Check(W->Loadout.PrimaryInventoryRevision == PrimaryRevision && W->Loadout.SecondaryInventoryRevision == SecondaryRevision,
                TEXT("failed pickup transaction changes neither ownership revision"));
            Blocker->Destroy(); Target->Destroy();
        });
        At(180, [&]()
        {
            P.bPreviousBoundsChecks = World->GetWorldSettings()->bEnableWorldBoundsChecks;
            World->GetWorldSettings()->bEnableWorldBoundsChecks = false;
            P.KillZ = Spawn(FVector(0,0,World->GetWorldSettings()->KillZ-300),FVector::ZeroVector);
            Check(P.KillZ.IsValid(),TEXT("out-of-world physical actor launched below configured KillZ"));
        });
        At(185, [&]()
        {
            Check(!P.KillZ.IsValid() && Registry->Count() == 0,TEXT("KillZ cleanup survives disabled engine world-bounds checks"));
            World->GetWorldSettings()->bEnableWorldBoundsChecks = P.bPreviousBoundsChecks;
        });
        At(190, [&]()
        {
            auto* Target = World->SpawnActor<AFPSWeaponPickup>(Body->GetActorLocation()+FVector(300,0,0),FRotator::ZeroRotator);
            Target->Initialize(W->Catalog()->CreateInstance(TEXT("Weapon.AK"))); const auto Before = W->GetLoadout().Primary;
            W->Request(EFPSWeaponRequest::Pickup,Target);
            Check(!W->CanPickup(Target) && SameInstance(Before,W->GetLoadout().Primary) && Target->IsAvailable(),TEXT("distant nominated pickup cannot mutate inventory"));
            Target->SetActorLocation(Body->GetActorLocation()+FVector(100,0,20));
            auto* Blocker = World->SpawnActor<AActor>(); auto* Box = NewObject<UBoxComponent>(Blocker);
            Blocker->SetRootComponent(Box); Box->SetBoxExtent(FVector(20,100,150)); Box->SetCollisionObjectType(ECC_WorldStatic);
            Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Box->SetCollisionResponseToAllChannels(ECR_Ignore); Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
            Box->RegisterComponent(); Blocker->SetActorLocation(Body->GetActorLocation()+FVector(50,0,0));
            W->Request(EFPSWeaponRequest::Pickup,Target);
            Check(!W->CanPickup(Target) && SameInstance(Before,W->GetLoadout().Primary) && Target->IsAvailable(),TEXT("occluded pickup cannot pass server action admission"));
            Blocker->Destroy(); Target->Destroy();
        });
        At(210, [&]()
        {
            auto* Victim = RemoteBody(); if (!Victim) { Check(false,TEXT("death fixture has recipient")); return; }
            auto* Drop = Spawn(Ground+FVector(400,200,150),FVector::ZeroVector);
            if (!Drop) { Check(false,TEXT("surviving pre-death dynamic drop launched")); return; }
            State->SurvivingGuid = Drop->GetInstance().InstanceId; State->DyingGuid = Victim->GetWeapons()->GetLoadout().Primary.InstanceId;
            P.OldBody = Victim; P.DeathCount = Registry->Count(); State->ForceNetUpdate();
            auto* ASC = Victim->GetAbilitySystemComponent(); auto Spec = ASC->MakeOutgoingSpec(UFPSWeaponDamageEffect::StaticClass(),1,FGameplayEffectContextHandle(new FGameplayEffectContext()));
            Spec.Data->SetSetByCallerMagnitude(FName(TEXT("WeaponDamage")),-1000); ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
            Check(Victim->IsEliminated() && Registry->Count() == P.DeathCount && !Find(State->DyingGuid),TEXT("real GAS death creates no gun drops and leaves old dynamic actor alive"));
        });
        At(255, [&]()
        {
            auto* Victim = RemoteBody();
            Check(Victim && Victim != P.OldBody.Get() && !Victim->IsEliminated() && Victim->GetWeapons()->GetLoadout().Primary.InstanceId != State->DyingGuid && Find(State->SurvivingGuid) && Registry->Count() == P.DeathCount,
                TEXT("actual delayed respawn creates fresh gun without deleting old physical drop"));
        });
        At(280, [&]()
        {
            auto* Drop = Find(State->SurvivingGuid);
            Check(Drop && !Drop->FindComponentByClass<UBoxComponent>()->IsAnyRigidBodyAwake() && Drop->NetDormancy == DORM_DormantAll,TEXT("late-observer resting fixture ready"));
            if (Drop)
            {
                State->SettledGuid = State->SurvivingGuid; State->SettledPosition = Drop->GetActorLocation();
                State->bSettled = true; State->bLateReady = true; State->ForceNetUpdate();
            }
        });
        At(300, [&]()
        {
            if (Training) return;
            auto* Recipient = RemoteBody();
            if (!Recipient) { Check(false,TEXT("same-GUID reacquire fixture has remote owner")); return; }
            Relocate(Recipient,Ground+FVector(130,0,270),true);
            auto* RW = Recipient->GetWeapons();
            RW->Loadout.Primary = RW->Catalog()->CreateInstance(TEXT("Weapon.M4"));
            RW->Loadout.Primary.Magazine = 25; RW->Loadout.ActiveSlot = EFPSWeaponSlot::Primary;
            State->ReacquireGuid = RW->Loadout.Primary.InstanceId; RW->Changed();
            Recipient->GetAbilitySystemComponent()->TryActivateAbilityByClass(UFPSEquipAbility::StaticClass());
            State->ForceNetUpdate();
        });
        At(323, [&]()
        {
            if (Training) return;
            auto* Drop = Find(State->ReacquireGuid); auto* Recipient = RemoteBody();
            Check(Drop && Recipient && !Recipient->GetWeapons()->GetLoadout().Primary.IsValid() && Drop->GetInstance().ShotSerial > 0 && Copies(State->ReacquireGuid) == 1,
                TEXT("remote held-trigger Drop RPC moves shot instance out of inventory once"));
            if (Drop) { State->ReacquireDropped = Drop->GetInstance(); State->ForceNetUpdate(); }
        });
        At(331, [&]()
        {
            if (Training) return;
            auto* Recipient = RemoteBody();
            Check(Recipient && SameInstance(Recipient->GetWeapons()->GetLoadout().Primary,State->ReacquireDropped) && !Find(State->ReacquireGuid) && Copies(State->ReacquireGuid) == 1,
                TEXT("remote real pickup RPC reacquires same GUID with exact dropped shot runtime"));
        });
        At(355, [&]()
        {
            if (Training) return;
            auto* Recipient = RemoteBody(); auto* RW = Recipient ? Recipient->GetWeapons() : nullptr;
            Check(RW && RW->Equipped().InstanceId == State->ReacquireGuid && RW->Equipped().ShotSerial > State->ReacquireDropped.ShotSerial && !RW->GetLoadout().bFireActive && Copies(State->ReacquireGuid) == 1,
                TEXT("same-GUID re-equip resumes held fire then authoritative stop leaves one instance"));
            if (RW) { State->ReacquireReference = RW->Equipped(); State->ForceNetUpdate(); }
        });
        At(370, [&]()
        {
            if (Training) return;
            auto* Recipient = RemoteBody(); auto* RW = Recipient ? Recipient->GetWeapons() : nullptr;
            if (!RW) { Check(false,TEXT("coalesced transfer has remote owner")); return; }
            RW->StopAllActions();
            RW->Loadout.Primary = RW->Catalog()->CreateInstance(TEXT("Weapon.M4"));
            RW->Loadout.Primary.Magazine = 12; RW->Loadout.Primary.ShotSerial = 100;
            RW->Loadout.Primary.NextShotAt = RW->ServerTime() + 12;
            RW->Loadout.Primary.LastShotAt = RW->ServerTime();
            RW->Loadout.ActiveSlot = EFPSWeaponSlot::Primary;
            State->CoalescedReference = RW->Loadout.Primary;
            State->CoalescedRevision = RW->Loadout.PrimaryInventoryRevision; RW->Changed();
            Recipient->GetAbilitySystemComponent()->TryActivateAbilityByClass(UFPSEquipAbility::StaticClass());
            State->ForceNetUpdate();
        });
        At(390, [&]()
        {
            if (Training) return;
            auto* Recipient = RemoteBody(); auto* RW = Recipient ? Recipient->GetWeapons() : nullptr;
            if (!RW) { Check(false,TEXT("coalesced transfer retains remote owner")); return; }
            RW->Drop();
            auto* Drop = Find(State->CoalescedReference.InstanceId);
            Check(Drop && !RW->Loadout.Primary.IsValid(),TEXT("coalesced frame actual GAS Drop creates one instance"));
            if (Drop)
            {
                // Inject the client activation endpoint on authority in this same frame.
                // A remote LocalPredicted event is otherwise routed back to its owning client.
                auto* ASC = Recipient->GetAbilitySystemComponent();
                FGameplayEventData Event; Event.EventTag = FGameplayTag::RequestGameplayTag(TEXT("Input.Weapon.Pickup")); Event.OptionalObject = Drop;
                for (const auto& Spec : ASC->GetActivatableAbilities())
                    if (Spec.Ability && Spec.Ability->IsA<UFPSPickupAbility>())
                    {
                        struct FActivationParams { FGameplayAbilitySpecHandle Handle; bool InputPressed; FPredictionKey Key; FGameplayEventData Payload; };
                        FPredictionKey InjectedKey; InjectedKey.Current = 30000;
                        FActivationParams Params{Spec.Handle,true,InjectedKey,Event};
                        ASC->ProcessEvent(ASC->FindFunctionChecked(TEXT("ServerTryActivateAbilityWithEventData")), &Params);
                        break;
                    }
            }
            Check(SameInstance(RW->Loadout.Primary,State->CoalescedReference) && !Find(State->CoalescedReference.InstanceId) && Copies(State->CoalescedReference.InstanceId) == 1 && RW->Loadout.bFireActive,
                TEXT("same-frame GAS Drop and Pickup restore authority without replicating intermediate empty slot"));
            Check(RW->Loadout.PrimaryInventoryRevision == State->CoalescedRevision + 2,
                TEXT("same-frame successful Drop and Pickup increment only affected ownership revision twice"));
        });
        At(350, [&]()
        {
            if (Training && FParse::Param(FCommandLine::Get(),TEXT("FPSDropVisual"))) PC->InputKey(FInputKeyParams(EKeys::P,IE_Pressed,1.0));
        });
        At(351, [&]() { if (Training) PC->InputKey(FInputKeyParams(EKeys::P,IE_Released,0.0)); });
        At(360, [&]()
        {
            if (Training && FParse::Param(FCommandLine::Get(),TEXT("FPSDropVisual")))
            {
                Check(PC->IsLoadoutOpen(),TEXT("training physical-drop footer menu renders through P input"));
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/WindowsEditor/DropLoadoutMenu.png"),true,false);
            }
        });
        At(760, [&]()
        {
            Check(Find(State->SurvivingGuid) && PresetCount() == State->PresetCount,TEXT("round cleanup fixture has dynamic and map preset actors"));
            World->GetAuthGameMode<AFPSMatchMode>()->FinishRound();
            Check(Registry->Count() == 0 && DynamicCount() == 0 && PresetCount() == State->PresetCount,TEXT("actual FinishRound clears only dynamic drops"));
        });
    }
    else
    {
        At(10, [&]() { Relocate(Body,Ground+FVector(130,0,270),true); });
        At(12, [&]() { Check(Registry->Count() == 0,TEXT("client does not own authority FIFO registry")); });
        At(24, [&]()
        {
            auto* Drop = Find(State->CooldownReference.InstanceId);
            UE_LOG(LogTemp,Display,TEXT("FPS_DROP_SMOKE MEASURE CLIENT airborne pawn=(%s) yaw=%.2f drop=(%s) distance_cm=%.3f"),*Body->GetActorLocation().ToCompactString(),Body->GetActorRotation().Yaw,Drop ? *Drop->GetActorLocation().ToCompactString() : TEXT("missing"),Drop ? FVector::Dist(Body->GetActorLocation(),Drop->GetActorLocation()) : FLT_MAX);
            Check(Drop && Drop->IsDynamic() && Drop->GetActorLocation().Z > Ground.Z + 80 && W->CanPickup(Drop),TEXT("client can actively nominate replicated airborne gun"));
            W->Request(EFPSWeaponRequest::Pickup,Drop);
        });
        At(36, [&]() { Check(SameInstance(W->GetLoadout().Primary,State->CooldownReference) && !Find(State->CooldownReference.InstanceId),TEXT("owner receives complete cross-holder instance and authoritative destruction")); });
        At(40, [&]() { PC->SetControlRotation(FRotator(45,90,0)); W->StartFire(); });
        At(60, [&]() { Check(W->Equipped().InstanceId == State->CooldownReference.InstanceId && W->Equipped().Magazine == 7 && W->Equipped().NextShotAt == State->CooldownReference.NextShotAt && W->GetLoadout().Action == EFPSWeaponAction::Idle,TEXT("predicted firing waits after equip until original gun cooldown")); });
        At(85, [&]() { Check(W->Equipped().Magazine == 7,TEXT("owner still has no predicted shots before original deadline")); });
        At(103, [&]() { W->StopFire(); Check(W->Equipped().Magazine < 7 && W->Equipped().NextShotAt > State->CooldownReference.NextShotAt,TEXT("owner observes firing only after original deadline")); });
        At(116, [&]()
        {
            auto* Drop = Find(State->SettledGuid);
            const float Error = Drop ? FVector::Dist(Drop->GetActorLocation(),State->SettledPosition) : FLT_MAX;
            UE_LOG(LogTemp,Display,TEXT("FPS_DROP_SMOKE MEASURE CLIENT rest_error_cm=%.3f server=(%s) client=(%s)"),Error,*State->SettledPosition.ToCompactString(),Drop ? *Drop->GetActorLocation().ToCompactString() : TEXT("missing"));
            Check(Drop && State->bSettled && Error < 25 && Drop->FindComponentByClass<UBoxComponent>()->IsSimulatingPhysics(),TEXT("client physical resting transform converges to server within 25cm"));
            if (Drop && FParse::Param(FCommandLine::Get(),TEXT("FPSDropVisual")))
            {
                PC->SetControlRotation((Drop->GetActorLocation()-Body->GetPawnViewLocation()).Rotation());
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/WindowsEditor/PhysicalDrop.png"),true,false);
            }
        });
        At(120, [&]()
        {
            P.BeforeRace = W->GetLoadout().Primary.InstanceId;
            auto* Target = Find(State->RaceGuid); Check(Target && W->CanPickup(Target),TEXT("client sends real contested pickup RPC"));
            W->Request(EFPSWeaponRequest::Pickup,Target);
            UE_LOG(LogTemp,Display,TEXT("FPS_DROP_SMOKE REQUEST CLIENT contested_guid=%s"),*State->RaceGuid.ToString());
        });
        At(140, [&]()
        {
            Check(State->bRaceResolved && !Find(State->RaceGuid) && (State->bClientWonRace ? W->GetLoadout().Primary.InstanceId == State->RaceGuid : W->GetLoadout().Primary.InstanceId == P.BeforeRace),TEXT("owner receives race winner or unchanged losing inventory"));
        });
        At(220, [&]() { Check(Body->IsEliminated() && Find(State->SurvivingGuid) && !Find(State->DyingGuid),TEXT("client receives death without death drops or old-drop deletion")); });
        At(260, [&]() { Check(!Body->IsEliminated() && W->GetLoadout().Primary.InstanceId != State->DyingGuid && Find(State->SurvivingGuid),TEXT("client receives fresh respawn while old physical drop persists")); });
        At(310, [&]()
        {
            Check(W->Equipped().InstanceId == State->ReacquireGuid && W->Equipped().Magazine == 25,TEXT("owner receives fresh automatic-gun reacquire fixture"));
            PC->SetControlRotation(FRotator(45,90,0)); W->StartFire();
        });
        At(320, [&]() { W->Drop(); });
        At(323, [&]()
        {
            Check(!W->GetLoadout().Primary.IsValid() && !W->PredictedShots.Contains(State->ReacquireGuid) && W->bTriggerHeld,
                TEXT("owner authoritative drop removes departed-GUID predictions while retaining trigger intent"));
        });
        At(324, [&]()
        {
            auto* Drop = Find(State->ReacquireGuid);
            Check(Drop && W->CanPickup(Drop),TEXT("owner nominates its own moving dropped GUID for real pickup RPC"));
            W->Request(EFPSWeaponRequest::Pickup,Drop);
        });
        At(332, [&]()
        {
            Check(SameInstance(W->GetLoadout().Primary,State->ReacquireDropped) && !Find(State->ReacquireGuid),TEXT("owner reacquires exact dropped instance without stale prediction overwrite"));
        });
        At(333, [&]() { W->Primary(); });
        At(343, [&]()
        {
            Check(W->Equipped().InstanceId == State->ReacquireGuid && W->Equipped().ShotSerial > State->ReacquireDropped.ShotSerial && W->PredictedShots.Contains(State->ReacquireGuid) && W->bTriggerHeld,
                TEXT("owner explicit re-equip resumes held fire with current-instance predictions"));
        });
        At(345, [&]() { W->StopFire(); });
        At(365, [&]()
        {
            UE_LOG(LogTemp,Display,TEXT("FPS_DROP_SMOKE MEASURE CLIENT reacquire ammo=%d/%d next=%.6f/%.6f serial=%u/%u"),W->Equipped().Magazine,State->ReacquireReference.Magazine,W->Equipped().NextShotAt,State->ReacquireReference.NextShotAt,W->Equipped().ShotSerial,State->ReacquireReference.ShotSerial);
            Check(SameInstance(W->Equipped(),State->ReacquireReference) && !W->GetLoadout().bFireActive && !Find(State->ReacquireGuid),TEXT("stopped owner ammo NextShot and ShotSerial exactly converge after same-GUID reacquire"));
        });
        At(380, [&]()
        {
            Check(SameInstance(W->Equipped(),State->CoalescedReference),TEXT("owner receives coalesced transfer fixture"));
            W->StartFire();
        });
        At(386, [&]()
        {
            auto Predicted = W->Loadout.Primary;
            Predicted.Magazine = 3; Predicted.ShotSerial += 9; Predicted.NextShotAt += 50;
            W->PredictedShots.Add(Predicted.InstanceId,Predicted);
            auto Secondary = W->Loadout.Secondary; Secondary.ShotSerial += 5;
            P.CoalescedSecondary = Secondary.InstanceId;
            P.CoalescedSecondaryRevision = W->Loadout.SecondaryInventoryRevision;
            W->PredictedShots.Add(Secondary.InstanceId,Secondary);
            Check(W->Loadout.bFireActive && W->PredictedShots.Contains(State->CoalescedReference.InstanceId),
                TEXT("coalesced fixture retains an unacknowledged prediction under actual held-fire ability"));
        });
        At(400, [&]()
        {
            Check(SameInstance(W->Loadout.Primary,State->CoalescedReference) && !W->PredictedShots.Contains(State->CoalescedReference.InstanceId),
                TEXT("coalesced same-GUID transfer discards former ownership predictions without an empty-slot snapshot"));
            Check(W->Loadout.PrimaryInventoryRevision == State->CoalescedRevision + 2 && W->Loadout.SecondaryInventoryRevision == P.CoalescedSecondaryRevision && W->PredictedShots.Contains(P.CoalescedSecondary),
                TEXT("coalesced primary transfer preserves unchanged secondary ownership and prediction"));
            W->StopFire();
        });
        At(402, [&]() { W->Primary(); });
        At(412, [&]()
        {
            Check(W->Loadout.PrimaryInventoryRevision == State->CoalescedRevision + 2 && W->Loadout.SecondaryInventoryRevision == P.CoalescedSecondaryRevision && W->Loadout.ActiveSlot == EFPSWeaponSlot::Primary,
                TEXT("ordinary switch changes active slot without changing either inventory revision"));
        });
        At(775, [&]() { Check(GS->bFinished && DynamicCount() == 0 && PresetCount() == State->PresetCount,TEXT("client receives end-of-round dynamic destruction and unchanged presets")); });
    }
    At(780, [&]() { Check(!P.Failed,TEXT("COMPLETE")); });
#endif
}
