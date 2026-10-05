#include "FPSTeachingSmoke.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "FPSDemoHealStation.h"
#include "FPSWeaponComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"

void FFPSTeachingSmoke::Tick(AFPSDemoCharacter* Pawn)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSTeachingSmoke")) || !Pawn->IsLocallyControlled() || Pawn->GetWeapons()->IsEnabled()) return;
    auto* World = Pawn->GetWorld();
    auto* GS = World->GetGameState<AFPSDemoGameState>();
    if (!GS || GS->PlayerArray.Num() != 2) return;
    const bool Server = Pawn->HasAuthority();
    if (Server && GS->RoundEndsAt == 0) { GS->RoundEndsAt = GS->GetServerWorldTimeSeconds() + 30; GS->ForceNetUpdate(); }
    if (GS->RoundEndsAt == 0) return;
    const double Time = GS->GetServerWorldTimeSeconds() - (GS->RoundEndsAt - 30);
    struct FProgress { TSet<int32> Done; bool Failed = false; };
    static TMap<TWeakObjectPtr<UWorld>, FProgress> Progress;
    auto& P = Progress.FindOrAdd(World);
    auto Check = [&](bool Pass, const TCHAR* What)
    {
        P.Failed |= !Pass;
        UE_LOG(LogTemp, Display, TEXT("FPS_TEACHING_SMOKE %s %s %s"), Pass ? TEXT("PASS") : TEXT("FAIL"), Server ? TEXT("SERVER") : TEXT("CLIENT"), What);
    };
    auto At = [&](int32 Tenth, TFunction<void()> Action) { if (Time >= Tenth * .1 && !P.Done.Contains(Tenth)) { P.Done.Add(Tenth); Action(); } };
    AFPSDemoCharacter* Other = nullptr;
    for (TActorIterator<AFPSDemoCharacter> It(World); It; ++It) if (*It != Pawn && (!Server || It->GetController())) { Other = *It; break; }
    if (!Other) return;
    if (Server)
    {
        At(5, [&]() { Pawn->SetActorLocation(FVector(-400,0,100)); Other->SetActorLocation(FVector(400,0,100)); Check(!Other->GetWeapons()->IsEnabled(), TEXT("original map retains teaching rules")); });
        At(25, [&]() { Check(Pawn->GetHealth() == 75, TEXT("remote teaching shot applies 25 damage")); });
        At(30, [&]() { Pawn->GetController()->SetControlRotation((Other->GetPawnViewLocation()-Pawn->GetPawnViewLocation()).Rotation()); Pawn->FireBotShot(); });
        At(40, [&]() { Pawn->FireBotShot(); });
        At(45, [&]() { Check(Other->GetHealth() == 50, TEXT("two teaching shots leave 50 health")); for (TActorIterator<AFPSDemoHealStation> It(World); It; ++It) { Other->SetActorLocation(It->GetActorLocation() + FVector(0,0,100)); break; } });
        At(60, [&]() { Check(Other->GetHealth() == 85, TEXT("heal adds 35 and immediate repeat is rejected")); Other->SetActorLocation(FVector(400,0,100)); Pawn->GetController()->SetControlRotation((Other->GetPawnViewLocation()-Pawn->GetPawnViewLocation()).Rotation()); });
        for (int32 T : {70,80,90,100}) At(T, [&]() { Pawn->GetController()->SetControlRotation((Other->GetPawnViewLocation()-Pawn->GetPawnViewLocation()).Rotation()); Pawn->FireBotShot(); });
        At(105, [&]() { Check(Other->IsEliminated() && Pawn->GetPlayerState<AFPSDemoPlayerState>()->GetEliminationCount() == 1, TEXT("teaching elimination and score")); });
        At(135, [&]() { Check(!Other->IsEliminated() && Other->GetHealth() == 100, TEXT("teaching respawn restores health")); });
    }
    else
    {
        At(15, [&]() { Pawn->GetController()->SetControlRotation((Other->GetPawnViewLocation()-Pawn->GetPawnViewLocation()).Rotation()); });
        At(20, [&]() { Pawn->PressFire(); });
        At(48, [&]() { Check(Pawn->GetHealth() == 50, TEXT("teaching damage replicates to owner")); });
        At(52, [&]() { Pawn->PressHeal(); Pawn->PressHeal(); });
        At(62, [&]() { Check(Pawn->GetHealth() == 85, TEXT("teaching heal replicates to owner")); });
        At(108, [&]() { Check(Pawn->IsEliminated(), TEXT("teaching death replicates to owner")); });
        At(138, [&]() { Check(!Pawn->IsEliminated() && Pawn->GetHealth() == 100, TEXT("teaching respawn reaches owner")); });
    }
    At(150, [&]() { Check(!P.Failed, TEXT("COMPLETE")); });
#endif
}
