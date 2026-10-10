#include "FPSCharacterTravelSmoke.h"
#include "FPSMatchFlow.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "FPSBaseMembership.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponData.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "InputKeyEventArgs.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "HAL/PlatformTime.h"
#include "UObject/UObjectHash.h"

#if !UE_BUILD_SHIPPING
namespace
{
struct FTravelProgress
{
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<AFPSDemoCharacter> OldPawn;
    TWeakObjectPtr<UAbilitySystemComponent> OldASC;
    TWeakObjectPtr<APlayerState> OldPS;
    TWeakObjectPtr<AFPSFrontController> OldController;
    TArray<TWeakObjectPtr<const UInputMappingContext>> OldContexts;
    TArray<FActiveGameplayEffectHandle> OldEffects;
    TSet<FGuid> SeenGuns;
    FGuid InitialPrimary, InitialSecondary;
    int32 Phase = 0, Action = 0;
    double Started = 0, NextAction = 0;
    bool PendingTravel = false, Complete = false, Failed = false;
};

UEnhancedInputLocalPlayerSubsystem* InputSubsystem(AFPSFrontController* PC)
{
    return PC->GetLocalPlayer() ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr;
}
TArray<const UInputMappingContext*> AppliedOwnedContexts(AFPSFrontController* PC)
{
    const auto* Subsystem = InputSubsystem(PC);
    TArray<const UInputMappingContext*> Contexts;
    if (!Subsystem) return Contexts;
    auto Collect = [&](UObject* Object)
    {
        if (const auto* Context = Cast<UInputMappingContext>(Object); Context && Subsystem->HasMappingContext(Context))
            Contexts.AddUnique(Context);
    };
    ForEachObjectWithOuter(PC, Collect, EGetObjectsFlags::IncludeNestedObjects);
    if (PC->GetPawn()) ForEachObjectWithOuter(PC->GetPawn(), Collect, EGetObjectsFlags::IncludeNestedObjects);
    return Contexts;
}
int32 OwnedContexts(AFPSFrontController* PC)
{
    return AppliedOwnedContexts(PC).Num();
}
bool HasKey(AFPSFrontController* PC, const FKey& Key)
{
    const auto* Subsystem = InputSubsystem(PC);
    if (Subsystem)
        for (const auto* Context : AppliedOwnedContexts(PC))
            for (const auto& Mapping : Context->GetMappings())
                if (Mapping.Action && Subsystem->QueryKeysMappedToAction(Mapping.Action.Get()).Contains(Key)) return true;
    return false;
}
void Key(AFPSFrontController* PC, const FKey& Value, EInputEvent Event)
{
    PC->InputKey(FInputKeyEventArgs(nullptr, FInputDeviceId::CreateFromInternalId(0), Value, Event,
        Event == IE_Pressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}
bool Capture(FTravelProgress& P, AFPSFrontController* PC)
{
    auto* Body = Cast<AFPSDemoCharacter>(PC->GetPawn());
    P.OldPawn = Body; P.OldASC = Body ? Body->GetAbilitySystemComponent() : nullptr;
    P.OldPS = PC->GetPlayerState<APlayerState>(); P.OldController = PC; P.OldContexts.Reset(); P.OldEffects.Reset();
    if (P.OldASC.IsValid())
        P.OldEffects = P.OldASC->GetActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAllOwningTags(
            FGameplayTagContainer(FGameplayTag::RequestGameplayTag(TEXT("Character.Protection.HostileDamage")))));
    for (const auto* Context : AppliedOwnedContexts(PC)) P.OldContexts.Add(Context);
    UE_LOG(LogTemp, Display, TEXT("FPS_CHARACTER_TRAVEL CAPTURE SERVER phase=%d pawn=%d asc=%d ps=%d regionEffects=%d contexts=%d"),
        P.Phase, Body != nullptr, P.OldASC.IsValid(), P.OldPS.IsValid(), P.OldEffects.Num(), P.OldContexts.Num());
    return Body && P.Phase != 2 ? !P.OldEffects.IsEmpty() : P.OldEffects.IsEmpty();
}
bool Cleanup(const FTravelProgress& P, AFPSFrontController* PC)
{
    bool EffectsRemoved = true, ContextsRemoved = true;
    if (P.OldASC.IsValid())
        for (const auto Handle : P.OldEffects) EffectsRemoved &= P.OldASC->GetActiveGameplayEffect(Handle) == nullptr;
    if (const auto* Subsystem = InputSubsystem(PC))
    {
        for (const auto& Context : P.OldContexts) if (Context.IsValid()) ContextsRemoved &= !Subsystem->HasMappingContext(Context.Get());
    }
    else ContextsRemoved = false;
    const bool PawnEnded = !P.OldPawn.IsValid() || !P.OldPawn->HasActorBegunPlay();
    const bool ControllerEnded = !P.OldController.IsValid() || !P.OldController->HasActorBegunPlay();
    const bool PSEnded = !P.OldPS.IsValid() || !P.OldPS->HasActorBegunPlay();
    UE_LOG(LogTemp, Display, TEXT("FPS_CHARACTER_TRAVEL CLEANUP SERVER phase=%d pawnEnded=%d controllerEnded=%d psEnded=%d oldASCAlive=%d effectsRemoved=%d contextsRemoved=%d contextsNow=%d"),
        P.Phase, PawnEnded, ControllerEnded, PSEnded, P.OldASC.IsValid(), EffectsRemoved, ContextsRemoved, OwnedContexts(PC));
    return PawnEnded && ControllerEnded && PSEnded && EffectsRemoved && ContextsRemoved;
}
}
#endif

void FFPSCharacterTravelSmoke::Tick(AFPSFrontController* PC)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("FPSCharacterTravelSmoke")) || !PC->IsLocalController() || !PC->HasAuthority()) return;
    auto* World = PC->GetWorld(); auto* GI = PC->GetGameInstance();
    if (!World || !GI) return;
    static TMap<TWeakObjectPtr<UGameInstance>, FTravelProgress> Progress;
    auto& P = Progress.FindOrAdd(GI);
    if (P.Complete) return;
    const double Now = FPlatformTime::Seconds();
    if (P.Started == 0) P.Started = Now;
    auto Check = [&](bool Pass, const TCHAR* Name)
    {
        P.Failed |= !Pass;
        UE_LOG(LogTemp, Display, TEXT("FPS_CHARACTER_TRAVEL %s SERVER phase=%d %s"), Pass ? TEXT("PASS") : TEXT("FAIL"), P.Phase, Name);
    };
    auto Finish = [&]()
    {
        P.Complete = true;
        UE_LOG(LogTemp, Display, TEXT("FPS_CHARACTER_TRAVEL %s SERVER COMPLETE phases=%d standalone=1 elapsed=%.2f"), P.Failed ? TEXT("FAIL") : TEXT("PASS"), P.Phase + 1, Now - P.Started);
    };
    if (Now - P.Started > 145) { Check(false, TEXT("same-process travel deadline")); Finish(); return; }
    if (P.PendingTravel && P.World.Get() == World) return;
    if (P.World.Get() != World)
    {
        P.World = World; P.Action = 0; P.NextAction = Now + 2; P.PendingTravel = false;
        UE_LOG(LogTemp, Display, TEXT("FPS_CHARACTER_TRAVEL ENTER SERVER phase=%d map=%s netmode=%d injection=OpenLevel-and-native-input"), P.Phase, *World->GetMapName(), int32(World->GetNetMode()));
    }
    if (Now < P.NextAction) return;
    const bool Lobby = P.Phase == 1 || P.Phase == 3;
    auto* Body = Cast<AFPSDemoCharacter>(PC->GetPawn());
    auto* GS = World->GetGameState<AFPSDemoGameState>();
    auto* PS = Body ? Body->GetPlayerState<AFPSDemoPlayerState>() : nullptr;
    auto* Membership = Body ? Body->FindComponentByClass<UFPSBaseMembership>() : nullptr;
    auto* W = Body ? Body->GetWeapons() : nullptr;
    if ((!Lobby && (!GS || !PS || !Membership || !W || !PC->IsWeaponContentVerified() || !W->GetLoadout().Primary.IsValid() || !W->GetLoadout().Secondary.IsValid())) || !InputSubsystem(PC)) return;
    auto Advance = [&](double Delay = .25) { ++P.Action; P.NextAction = Now + Delay; };
    auto Travel = [&](const TCHAR* Map, const TCHAR* Options)
    {
        Check(Capture(P, PC), TEXT("match captures live granted-tag protection effects and training lobby capture none"));
        if (P.Failed) { Finish(); return; }
        ++P.Phase; P.PendingTravel = true;
        UGameplayStatics::OpenLevel(PC, FName(Map), true, FString(Options));
    };
    if (P.Action == 0)
    {
        Check(World->GetNetMode() == NM_Standalone, TEXT("same-process standalone OpenLevel path"));
        Check(HasKey(PC, EKeys::P) && HasKey(PC, EKeys::Escape), TEXT("new controller P and Esc mappings are applied"));
        Check(OwnedContexts(PC) == (Lobby ? 1 : 2), TEXT("exactly one menu context plus current character context when possessed"));
        if (P.Phase > 0) Check(Cleanup(P, PC), TEXT("prior pawn ASC region effects PS controller and input contexts ended"));
        if (Lobby)
        {
            Check(World->GetGameState<AFPSLobbyState>() && !Body && PC->bShowMouseCursor && PC->IsMoveInputIgnored(), TEXT("Entry lobby has no gameplay pawn and owns local menu input"));
            Check(GI->GetSubsystem<UFPSWeaponCatalog>()->RequiredIds().IsEmpty(), TEXT("old match content session released in lobby"));
        }
        else
        {
            Check(GS->bTraining == (P.Phase == 2) && GS->IsCombatActive() && Membership->CanSelect(), TEXT("new map mode and current-life permission are valid"));
            Check(W->GetLoadout().Primary.WeaponId == TEXT("Weapon.M4") && W->GetLoadout().Secondary.WeaponId == TEXT("Weapon.M9") &&
                PS->SpawnPrimary() == TEXT("Weapon.M4") && PS->SpawnSecondary() == TEXT("Weapon.M9"), TEXT("new PlayerState and actual inventory start with M4 M9 defaults"));
            Check(!P.SeenGuns.Contains(W->GetLoadout().Primary.InstanceId) && !P.SeenGuns.Contains(W->GetLoadout().Secondary.InstanceId), TEXT("new world receives fresh primary and secondary GUIDs"));
            P.InitialPrimary = W->GetLoadout().Primary.InstanceId; P.InitialSecondary = W->GetLoadout().Secondary.InstanceId;
            P.SeenGuns.Add(P.InitialPrimary); P.SeenGuns.Add(P.InitialSecondary);
            Check(Membership->IsProtected() == (P.Phase != 2), TEXT("current world protection follows gameplay mode"));
        }
        Advance();
    }
    else if (P.Action == 1) { Key(PC, EKeys::P, IE_Pressed); Advance(); }
    else if (P.Action == 2) { Key(PC, EKeys::P, IE_Released); Advance(.35); }
    else if (P.Action == 3)
    {
        Check(PC->IsLoadoutOpen() == !Lobby, TEXT("injected P invokes only the current controller selection mapping"));
        if (!Lobby) Check(PC->bShowMouseCursor && PC->IsMoveInputIgnored() && PC->IsLookInputIgnored() && !W->GetLoadout().bFireActive, TEXT("selection menu owns mouse and clears combat input"));
        Advance();
    }
    else if (Lobby)
    {
        if (P.Failed) { Finish(); return; }
        Travel(P.Phase == 1 ? TEXT("/Game/FPSDemo/Maps/LV_FPSTraining") : TEXT("/Game/FPSDemo/Maps/LV_FPSFoundry"),
            P.Phase == 1 ? TEXT("game=/Script/MCP_Test.FPSMatchMode?Mode=Training?Bots=0") : TEXT("game=/Script/MCP_Test.FPSMatchMode?Mode=Solo?Bots=0"));
    }
    else if (P.Action == 4)
    {
        if (P.Phase == 0) PC->RequestSelection(TEXT("Weapon.AK"));
        else if (P.Phase == 2) PC->RequestSelection(TEXT("Weapon.HeavyPistol"));
        Advance(.4);
    }
    else if (P.Action == 5)
    {
        if (P.Phase == 0)
        {
            Check(PC->LastSelectionSucceeded() && W->GetLoadout().Primary.WeaponId == TEXT("Weapon.AK") &&
                W->GetLoadout().Primary.InstanceId != P.InitialPrimary && PS->SpawnPrimary() == TEXT("Weapon.AK"), TEXT("first match public selection saves a distinct non-default local preference"));
            P.SeenGuns.Add(W->GetLoadout().Primary.InstanceId);
        }
        else if (P.Phase == 2)
        {
            Check(PC->LastSelectionSucceeded() && W->Equipped().WeaponId == TEXT("Weapon.HeavyPistol") &&
                W->GetLoadout().Primary.InstanceId == P.InitialPrimary && W->GetLoadout().Secondary.InstanceId != P.InitialSecondary,
                TEXT("training selection equips new other-slot instance without old-match preference"));
            P.SeenGuns.Add(W->GetLoadout().Secondary.InstanceId);
        }
        Advance();
    }
    else if (P.Action == 6) { Key(PC, EKeys::Escape, IE_Pressed); Advance(); }
    else if (P.Action == 7) { Key(PC, EKeys::Escape, IE_Released); Advance(.35); }
    else if (P.Action == 8)
    {
        Check(!PC->IsLoadoutOpen() && !PC->bShowMouseCursor && !PC->IsMoveInputIgnored() && !PC->IsLookInputIgnored(), TEXT("injected Esc closes current selection and restores local input"));
        if (P.Failed || P.Phase == 4) { Finish(); return; }
        Advance();
    }
    else if (P.Action == 9) { Key(PC, EKeys::P, IE_Pressed); Advance(); }
    else if (P.Action == 10) { Key(PC, EKeys::P, IE_Released); Advance(.35); }
    else
    {
        Check(PC->IsLoadoutOpen(), TEXT("leaving map with a live selection menu exercises EndPlay cleanup"));
        if (P.Failed) { Finish(); return; }
        Travel(TEXT("/Engine/Maps/Entry"), TEXT("game=/Script/MCP_Test.FPSLobbyMode"));
    }
#endif
}
