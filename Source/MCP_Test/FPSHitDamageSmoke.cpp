#include "FPSHitDamageSmoke.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponShot.h"
#include "FPSHitRegions.h"
#include "FPSMovementComponent.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoAttributeSet.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "FPSDemoPlayerController.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void FFPSHitDamageSmoke::Tick(UFPSWeaponComponent* W)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSHitDamageSmoke")) || !W->IsEnabled()) return;
    auto* Pawn = W->Character(); auto* World = Pawn->GetWorld();
    const bool Server = Pawn->HasAuthority();
    Pawn->GetCharacterMovement()->GravityScale = 0;
    auto* GS = World->GetGameState<AFPSDemoGameState>();
    if (!GS || GS->bWaiting || GS->PlayerArray.Num() != 2) return;
    APlayerState* ShooterState=GS->PlayerArray[0]; APlayerState* TargetState=GS->PlayerArray[1];
    if(ShooterState->GetPlayerId()<TargetState->GetPlayerId()) Swap(ShooterState,TargetState);
    if(ShooterState->GetPlayerId()==TargetState->GetPlayerId()) return;
    const double SharedTime=W->ServerTime()-(GS->RoundEndsAt-300);
    if(Pawn->GetPlayerState()==TargetState && Pawn->IsLocallyControlled())
    {
        static TMap<TWeakObjectPtr<AFPSDemoCharacter>,int32> TargetStages;
        auto& Stage=TargetStages.FindOrAdd(Pawn);
        auto* Move=CastChecked<UFPSMovementComponent>(Pawn->GetCharacterMovement());
        if(SharedTime>=9.5 && Stage==0){Move->SetMovementMode(MOVE_Falling);Move->RequestCrouch(true);Stage=1;}
        if(SharedTime>=12.8 && Stage==1){Move->RequestCrouch(false);Stage=2;}
    }
    // PlayerArray ordering is local; replicated PlayerId selects the same shooter/target on every world.
    if (Pawn->GetPlayerState() != ShooterState || (!Server && !Pawn->IsLocallyControlled())) return;
    AFPSDemoCharacter* Target = nullptr;
    for (TActorIterator<AFPSDemoCharacter> It(World); It; ++It)
        if (It->GetPlayerState() == TargetState) { Target = *It; break; }
    if (!Target) return;
    const double Time = W->ServerTime() - (GS->RoundEndsAt - 300);
    struct FProgress { TSet<int32> Done; bool Failed = false; FVector StandingHead; FFPSWeaponShotContext Shot; };
    static TMap<TWeakObjectPtr<UFPSWeaponComponent>, FProgress> Progress;
    auto& P = Progress.FindOrAdd(W);
    auto Check = [&](bool Pass, const TCHAR* What)
    {
        P.Failed |= !Pass;
        UE_LOG(LogTemp, Display, TEXT("FPS_HIT_SMOKE %s %s %s health=%.3f mode=%d"), Pass?TEXT("PASS"):TEXT("FAIL"),
            Server?TEXT("SERVER"):TEXT("CLIENT"), What, Target->GetHealth(), int32(World->GetNetMode()));
    };
    auto At = [&](int32 Tenths, auto Fn) { if (Time >= Tenths / 10. && !P.Done.Contains(Tenths)) { P.Done.Add(Tenths); Fn(); } };
    auto Health = [&](float Value) { Target->GetAbilitySystemComponent()->SetNumericAttributeBase(UFPSDemoAttributeSet::GetHealthAttribute(), Value); };
    auto Fire = [&](FName Body)
    {
        if (Server) return;
        auto* Box = Target->GetHitRegions()->FindBody(Body);
        Check(Box != nullptr, TEXT("replicated target has pose hit bodies")); if (!Box) return;
        W->StartFire(); W->StopFire();
    };
    auto Aim = [&](FName Body)
    {
        if(!Server) if(auto* Box=Target->GetHitRegions()->FindBody(Body))
            Pawn->GetController()->SetControlRotation((Box->GetComponentLocation()-Pawn->GetPawnViewLocation()).Rotation());
    };
    At(15,[&](){Aim(TEXT("Head"));});
    At(37,[&](){Aim(TEXT("Torso"));});
    At(60,[&](){Aim(TEXT("LShin"));});
    At(105,[&](){Aim(TEXT("Head"));});
    At(170,[&](){Aim(TEXT("Head"));});
    At(0, [&]()
    {
        Pawn->GetCharacterMovement()->DisableMovement();
        // Fixture disables recoil only in these processes to isolate region boundary testing.
        if (auto* Def = const_cast<UFPSWeaponDefinition*>(W->Catalog()->Find(TEXT("Weapon.M9")))) Def->bNoRecoil = true;
        if (!Server) return;
        Target->GetCharacterMovement()->DisableMovement();
        Pawn->SetActorLocation(FVector(0,0,1600)); Target->SetActorLocation(FVector(500,0,1600));
        Target->SetActorRotation(FRotator::ZeroRotator);
        Pawn->GetPlayerState<AFPSDemoPlayerState>()->TeamId=0; Target->GetPlayerState<AFPSDemoPlayerState>()->TeamId=1;
        W->StopAllActions(); W->Loadout.Secondary=W->Catalog()->CreateInstance(TEXT("Weapon.M9"));
        W->Loadout.ActiveSlot=EFPSWeaponSlot::Secondary; W->Changed(); Health(100);
    });
    auto CaptureBody = [&](const TCHAR* Name)
    {
        FString Path; if(Server || !FParse::Value(FCommandLine::Get(),TEXT("FPSHitCapture="),Path)) return;
        Pawn->GetController()->SetControlRotation((Target->GetActorLocation()-Pawn->GetPawnViewLocation()).Rotation());
        TInlineComponentArray<UBoxComponent*> Boxes(Target);
        for(auto* Box:Boxes) if(Box->GetName().StartsWith(TEXT("Hit_")))
            DrawDebugBox(World,Box->GetComponentLocation(),Box->GetScaledBoxExtent(),Box->GetComponentQuat(),
                Box->GetName()==TEXT("Hit_Head")?FColor::Red:FColor::Green,false,.4f,0,1.f);
        FScreenshotRequest::RequestScreenshot(FPaths::GetPath(Path)/Name,false,false);
    };
    At(13,[&](){CaptureBody(TEXT("Standing.png"));});
    At(20, [&]() { Fire(TEXT("Head")); });
    At(28, [&]()
    {
        Check(FMath::IsNearlyEqual(Target->GetHealth(),50.f),TEXT("head GAS damage 25 times 2"));
        if (!Server) Check(CastChecked<AFPSDemoPlayerController>(Pawn->GetController())->WasHeadshot(),TEXT("owner receives confirmed head marker"));
        P.StandingHead=Target->GetHitRegions()->FindBody(TEXT("Head"))->GetComponentLocation();
    });
    At(32, [&]() { if (Server) Health(100); });
    At(42, [&]() { Fire(TEXT("Torso")); });
    At(50, [&]()
    {
        Check(FMath::IsNearlyEqual(Target->GetHealth(),75.f),TEXT("torso GAS damage 25 times 1"));
        if (!Server) Check(!CastChecked<AFPSDemoPlayerController>(Pawn->GetController())->WasHeadshot(),TEXT("ordinary hit clears prior head marker"));
    });
    At(55, [&]() { if (Server) Health(100); });
    At(65, [&]() { Fire(TEXT("LShin")); });
    At(73, [&]() { Check(FMath::IsNearlyEqual(Target->GetHealth(),81.25f),TEXT("limb GAS damage preserves fraction")); });
    At(80, [&]()
    {
        auto* Regions=Target->GetHitRegions();
        auto Trace=[&](FVector A,FVector B){FHitResult H; FCollisionQueryParams Q(SCENE_QUERY_STAT(HitProbe),true,Pawn); World->LineTraceSingleByChannel(H,A,B,UFPSHitRegions::ShotChannel,Q);return H;};
        auto* Head=Regions->FindBody(TEXT("Head")); auto* Torso=Regions->FindBody(TEXT("Torso"));
        Check(Regions->Classify(Trace(Head->GetComponentLocation()-FVector(150,0,0),Head->GetComponentLocation()+FVector(150,0,0)))==EFPSHitRegion::Head,TEXT("world trace selects head not movement capsule"));
        const FVector Gap=Target->GetActorLocation()+FVector(0,34,72);
        Check(Trace(Gap-FVector(100,0,0),Gap+FVector(100,0,0)).GetActor()!=Target,TEXT("outside body does not hit capsule padding"));
        // A ray crossing the forearm and torso must resolve the first real body, once.
        auto* Arm=Regions->FindBody(TEXT("LForearm"));
        const FVector Direction=(Torso->GetComponentLocation()-Arm->GetComponentLocation()).GetSafeNormal();
        const auto ArmHit=Trace(Arm->GetComponentLocation()-Direction*40,Torso->GetComponentLocation());
        Check(Regions->Classify(ArmHit)==EFPSHitRegion::Limb,TEXT("arm occludes torso without penetration"));
        FHitResult Edge; const FTransform Transform=Head->GetComponentTransform(); const auto Ext=Head->GetUnscaledBoxExtent();
        FCollisionQueryParams Query;
        Check(Head->LineTraceComponent(Edge,Transform.TransformPosition(FVector(-80,Ext.Y-.1,0)),Transform.TransformPosition(FVector(80,Ext.Y-.1,0)),Query),TEXT("head boundary inside hits"));
        Check(!Head->LineTraceComponent(Edge,Transform.TransformPosition(FVector(-80,Ext.Y+.1,0)),Transform.TransformPosition(FVector(80,Ext.Y+.1,0)),Query),TEXT("head boundary outside misses"));
        auto* Wall=World->SpawnActor<AActor>();
        auto* Block=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Block);Block->SetBoxExtent(FVector(3,40,40));
        Block->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Block->SetCollisionResponseToAllChannels(ECR_Ignore);
        Block->SetCollisionResponseToChannel(UFPSHitRegions::ShotChannel,ECR_Block);Block->RegisterComponent();
        Wall->SetActorLocation(Head->GetComponentLocation()-FVector(60,0,0));
        Check(Trace(Head->GetComponentLocation()-FVector(150,0,0),Head->GetComponentLocation()).GetActor()==Wall,TEXT("world cover blocks body trace"));
        Wall->Destroy();
        if (!Server) return;
        P.Shot=FFPSWeaponShotContext::Capture(Pawn,W->Equipped(),*W->Definition());
        auto* Profile=const_cast<UFPSHitDamageProfile*>(W->Definition()->HitDamageProfile.Get());
        const float Original=Profile->Multipliers.Head; Profile->Multipliers.Head=9;
        Health(100); Pawn->ApplyDamageEffectTo(Target,P.Shot,EFPSHitRegion::Head); Profile->Multipliers.Head=Original;
        Check(FMath::IsNearlyEqual(Target->GetHealth(),50.f),TEXT("shot keeps profile snapshot after definition change"));
    });
    At(90, [&]() { Check(FMath::IsNearlyEqual(Target->GetHealth(),50.f),TEXT("snapshot damage replicates")); });
    At(94, [&]() { if(Server){Health(100);Target->GetCharacterMovement()->SetMovementMode(MOVE_Falling);} });
    At(100,[&](){CaptureBody(TEXT("Crouched.png"));});
    At(110, [&]()
    {
        Check(Target->bIsCrouched && Target->GetHitRegions()->FindBody(TEXT("Head"))->GetComponentLocation().Z<P.StandingHead.Z-10,TEXT("crouch updates head pose on both ends"));
        Fire(TEXT("Head"));
    });
    At(120,[&]() { Check(FMath::IsNearlyEqual(Target->GetHealth(),50.f),TEXT("crouched head shot resolves authoritative pose")); });
    At(130,[&]()
    {
        if(!Server)return;
        Target->GetCharacterMovement()->bWantsToCrouch=false;
        Target->GetCharacterMovement()->UnCrouch(false); Target->ForceNetUpdate(); Health(100);
        // Real CMC movement under server authority, no actor-position-only pose fixture.
        Target->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
        Target->GetCharacterMovement()->Velocity=FVector(0,100,0);
    });
    if(Server && Time>=13 && Time<14.5) Target->GetCharacterMovement()->Velocity=FVector(0,100,0);
    At(140,[&]() { Check(Target->GetHitRegions()->FindBody(TEXT("Head"))->GetComponentLocation().Y>20,TEXT("moving target hit bodies follow replicated actor")); });
    At(145,[&]() { if(Server)Target->GetCharacterMovement()->DisableMovement(); });
    At(160,[&]()
    {
        if(Server) { Health(25); return; }
        // A client-created damage payload cannot mutate health.
        const float Before=Target->GetHealth(); FFPSWeaponShotContext Forged=P.Shot;
        Forged.Source=Pawn;Forged.InstanceId=W->Equipped().InstanceId;Forged.WeaponId=W->Equipped().WeaponId;Forged.BaseDamage=999;
        Pawn->ApplyDamageEffectTo(Target,Forged,EFPSHitRegion::Head);
        Check(Target->GetHealth()==Before,TEXT("client forged region damage rejected"));
    });
    At(175,[&]() { Fire(TEXT("Head")); });
    At(183,[&]()
    {
        Check(Target->IsEliminated(),TEXT("head kill reaches replicated life state"));
        Check(Target->GetHitRegions()->FindBody(TEXT("Head"))->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("death disables hit bodies"));
        if(!Server)
        {
            const auto* PC=CastChecked<AFPSDemoPlayerController>(Pawn->GetController());
            Check(PC->WasHeadshot()&&PC->WasElimination(),TEXT("head kill retains both confirmation meanings"));
            FString Path; if(FParse::Value(FCommandLine::Get(),TEXT("FPSHitCapture="),Path)) FScreenshotRequest::RequestScreenshot(Path,false,false);
        }
        Check(!P.Failed,TEXT("COMPLETE"));
    });
#endif
}
