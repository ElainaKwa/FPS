#include "FPSExtractionSmoke.h"
#include "FPSExtractionMode.h"
#include "FPSExtractionSubsystem.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoPlayerState.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "HAL/PlatformMisc.h"

void FFPSExtractionSmoke::CombatTick(AFPSExtractionMode* Mode)
{
#if !UE_BUILD_SHIPPING
    if (!Mode || !FParse::Param(FCommandLine::Get(),TEXT("FPSExtractionCombatCheck")) || Mode->GetWorld()->GetTimeSeconds() < 2) return;
    static int32 Stage = 0;
    static double Started = 0;
    static TArray<FVector> Positions;
    auto* Loop = Mode->GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>();
    auto Check = [](bool Pass,const TCHAR* What)
    {
        UE_LOG(LogTemp,Display,TEXT("FPS_EXTRACTION_COMBAT %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),What);
        if (!Pass) { Stage=99; FPlatformMisc::RequestExitWithStatus(false,1); }
        return Pass;
    };
    if (Stage == 0)
    {
        Stage=1; Check(Loop->StartExpedition(),TEXT("start guard expedition")); return;
    }
    if (!Mode->IsRaid()) return;
    auto* PC = Mode->GetWorld()->GetFirstPlayerController();
    auto* Body = PC ? Cast<AFPSDemoCharacter>(PC->GetPawn()) : nullptr;
    if (!Body || !Mode->bRestored) return;
    TArray<AFPSBotController*> Bots;
    for(TActorIterator<AFPSBotController> It(Mode->GetWorld());It;++It) Bots.Add(*It);
    if (Stage == 1)
    {
        if(!Check(Bots.Num()==3,TEXT("three PvE guards spawned"))) return;
        for(auto* Bot:Bots) if(!Check(Bot->GetPawn()&&Bot->GetPlayerState<AFPSDemoPlayerState>()->TeamId==1,TEXT("guard hostile and possessed")))return;
        // Controlled initial positioning; subsequent perception, navigation and firing are real AI ticks.
        Body->SetActorLocation(FVector(-1600,-1400,100),false,nullptr,ETeleportType::TeleportPhysics);
        Bots[0]->GetPawn()->SetActorLocation(FVector(-1200,-1400,100),false,nullptr,ETeleportType::TeleportPhysics);
        Bots[0]->SetControlRotation(FRotator(0,180,0));
        Positions.Reset();for(auto* Bot:Bots)Positions.Add(Bot->GetPawn()->GetActorLocation());
        Started=FPlatformTime::Seconds();Stage=2;return;
    }
    if(Stage==2)
    {
        bool Moved=false;for(int32 I=0;I<Bots.Num()&&I<Positions.Num();++I)if(Bots[I]->GetPawn())Moved|=FVector::Dist2D(Positions[I],Bots[I]->GetPawn()->GetActorLocation())>50;
        if(Body->GetHealth()<100 && Moved)
        {
            Check(true,TEXT("guards navigate perceive and damage player through weapon GAS"));
            Loop->AbortExpedition();Stage=3;Started=FPlatformTime::Seconds();return;
        }
        if(FPlatformTime::Seconds()-Started>60)Check(false,TEXT("guard navigation/damage timeout"));
    }
    if(Stage==3 && FPlatformTime::Seconds()-Started>3)
    {
        if(!Check(Loop->GetPhase()==EFPSExtractionPhase::Result,TEXT("combat stops after settlement")))return;
        UE_LOG(LogTemp,Display,TEXT("FPS_EXTRACTION_COMBAT COMPLETE"));Stage=99;FPlatformMisc::RequestExit(false);
    }
#endif
}
