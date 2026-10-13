#include "FPSExtractionMode.h"
#include "FPSExtractionSubsystem.h"
#include "FPSExtractionUI.h"
#include "FPSExtractionSmoke.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoGameState.h"
#include "FPSDemoPlayerState.h"
#include "FPSWeaponComponent.h"
#include "FPSLifeComponent.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/CommandLine.h"
#include "UObject/ConstructorHelpers.h"

AFPSExtractionLoot::AFPSExtractionLoot()
{
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cargo")); SetRootComponent(Mesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Mesh->SetStaticMesh(Cube.Object); Mesh->SetRelativeScale3D(FVector(.45f, .35f, .25f));
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/FPSDemo/MapMaterials/M_Map_Gold.M_Map_Gold"));
    if (Material.Succeeded()) Mesh->SetMaterial(0, Material.Object);
}
AFPSExtractionMode::AFPSExtractionMode()
{
    PlayerControllerClass = AFPSExtractionController::StaticClass(); PrimaryActorTick.bCanEverTick = true;
}
void AFPSExtractionMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    bRaid = UGameplayStatics::ParseOption(Options, TEXT("LabPhase")) == TEXT("Raid");
    if (GetNetMode() != NM_Standalone) ErrorMessage = TEXT("Extraction lab currently supports standalone play only.");
}
void AFPSExtractionMode::InitGameState()
{
    Super::InitGameState();
    auto* GS = GetGameState<AFPSDemoGameState>(); GS->bWaiting = !bRaid; GS->RoundEndsAt = 0;
    if (!GS->WeaponDataError.IsEmpty()) return;
    auto* Loop = GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>();
    if (!Loop->EnterWorld(bRaid))
    {
        bRaid = false; GS->bWaiting = true;
        if (Loop->GetView().Feedback.IsEmpty())
            Loop->SetFeedback(TEXT("实验未启动，请检查存档或先从基地出征。"));
    }
}
void AFPSExtractionMode::BeginPlay()
{
    // Reuse structured character/content rules, but no deathmatch timers, free stands or team balancing.
    AFPSDemoGameMode::BeginPlay();
    if (!GetGameState<AFPSDemoGameState>()->WeaponDataError.IsEmpty()) return;
    if (bRaid)
    {
        SpawnCargo(TEXT("Scrap"), 8, FVector(-1600,-1400,20));
        SpawnCargo(TEXT("Wire"), 2, FVector(-800,-3300,20));
        SpawnCargo(TEXT("Scrap"), 12, FVector(-1800,3500,20));
        SpawnCargo(TEXT("Wire"), 3, FVector(-400,2000,20));
        SpawnCargo(TEXT("Optic"), 1, FVector(3200,2200,20));
        SpawnCargo(TEXT("Scrap"), 12, FVector(3200,-3200,20));
        int32 Bots = 3;
#if !UE_BUILD_SHIPPING
        // Deterministic inventory/settlement tests do not claim to validate enemy combat.
        if (FParse::Param(FCommandLine::Get(), TEXT("FPSExtractionSmoke"))) Bots = 0;
#endif
        for (int32 Index = 0; Index < Bots; ++Index)
        {
            auto* Bot = GetWorld()->SpawnActor<AFPSBotController>();
            if (!Bot) continue;
            if (auto* PS = Bot->GetPlayerState<AFPSDemoPlayerState>())
            { PS->TeamId = 1; PS->SetPlayerName(FString::Printf(TEXT("回收区守卫 %d"), Index + 1)); }
            Super::RestartPlayer(Bot);
        }
        auto* Marker = SpawnCargo(NAME_None, 0, ExitLocation - FVector(0,0,75));
        if (Marker) Marker->Mesh->SetRelativeScale3D(FVector(3.8f,3.8f,.05f));
    }
    else if (auto* PC = GetWorld()->GetFirstPlayerController())
    {
        auto* Camera = GetWorld()->SpawnActor<ACameraActor>(FVector(-3800,-2200,1200), FRotator(-15,35,0));
        if (Camera) PC->SetViewTarget(Camera);
    }
}
void AFPSExtractionMode::RestartPlayer(AController* Player)
{
    if (!bRaid || !Player || Player->GetPawn()) return;
    if (auto* PS = Player->GetPlayerState<AFPSDemoPlayerState>()) PS->TeamId = Player->IsPlayerController() ? 0 : 1;
    Super::RestartPlayer(Player);
    if (Player->IsPlayerController())
        if (auto* Body = Cast<AFPSDemoCharacter>(Player->GetPawn()))
        {
            if (Body->HasActorBegunPlay())
            {
                bRestored = GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>()->RestoreEquipment(Body);
                if (!bRestored) GetGameState<AFPSDemoGameState>()->bWaiting = true;
            }
        }
}
void AFPSExtractionMode::HandleLifeEnded(UFPSLifeComponent* Life, const FFPSDeathContext& Context)
{
    if (!Life || !HasAuthority()) return;
    auto* Body = Cast<AFPSDemoCharacter>(Life->GetOwner());
    if (Body && Body->GetController() && Body->GetController()->IsPlayerController())
        GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>()->FinishExpedition(false);
    else
    {
        if (auto* PS = Cast<AFPSDemoPlayerState>(Context.AttackerState.Get())) PS->AddElimination();
        // Guards stay defeated for this expedition; do not run the deathmatch respawn path.
    }
}
AFPSExtractionLoot* AFPSExtractionMode::SpawnCargo(FName Id, int32 Quantity, const FVector& Location)
{
    auto* Cargo = GetWorld()->SpawnActor<AFPSExtractionLoot>(Location, FRotator::ZeroRotator);
    if (Cargo) { Cargo->ItemId = Id; Cargo->Quantity = Quantity; }
    return Cargo;
}
AFPSExtractionLoot* AFPSExtractionMode::FocusLoot(const AFPSDemoCharacter* Body) const
{
    if (!Body || Body->IsEliminated()) return nullptr;
    AFPSExtractionLoot* Best = nullptr; float Distance = FMath::Square(230.f);
    const FVector Eye = Body->GetPawnViewLocation();
    for (TActorIterator<AFPSExtractionLoot> It(GetWorld()); It; ++It)
    {
        const float D = FVector::DistSquared(Body->GetActorLocation(), It->GetActorLocation());
        if (It->Quantity <= 0 || D > Distance) continue;
        const FVector Direction = (It->GetActorLocation() - Eye).GetSafeNormal();
        if (FVector::DotProduct(Body->GetControlRotation().Vector(), Direction) < .35f) continue;
        FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(ExtractionSearch), false, Body);
        if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, It->GetActorLocation(), ECC_Visibility, Query)) continue;
        Best = *It; Distance = D;
    }
    return Best;
}
void AFPSExtractionMode::Interact(AFPSExtractionController* PC)
{
    auto* Loop = GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>();
    auto* Body = PC ? Cast<AFPSDemoCharacter>(PC->GetPawn()) : nullptr;
    if (!bRaid || !Body || Body->IsEliminated() || Loop->GetPhase() != EFPSExtractionPhase::Raid || PC->IsExtractionPanelOpen()) return;
    if (auto* Loot = FocusLoot(Body))
    { if (Loop->Collect(Loot->ItemId, Loot->Quantity)) Loot->Destroy(); return; }
    if (FVector::Dist2D(Body->GetActorLocation(), ExitLocation) <= 240)
    {
        ExitStarted = ExitStarted < 0 ? GetWorld()->GetTimeSeconds() : -1;
        Loop->SetFeedback(ExitStarted < 0 ? TEXT("已取消撤离。") : TEXT("撤离中，请留在区域内。"));
    }
}
void AFPSExtractionMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bRaid && !bRestored)
        if (auto* Pawn = Cast<AFPSDemoCharacter>(UGameplayStatics::GetPlayerPawn(this,0)); Pawn && Pawn->HasActorBegunPlay())
        {
            bRestored = GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>()->RestoreEquipment(Pawn);
            if (!bRestored) GetGameState<AFPSDemoGameState>()->bWaiting = true;
        }
    FFPSExtractionSmoke::Tick(this);
    FFPSExtractionSmoke::CombatTick(this);
    FFPSExtractionSmoke::CaptureTick(this);
    auto* Loop = GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>();
    if (!bRaid || Loop->GetPhase() != EFPSExtractionPhase::Raid) return;
    auto* Body = Cast<AFPSDemoCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Body) return;
    if (Body->IsEliminated()) { Loop->FinishExpedition(false); return; }
    const float Distance = FVector::Dist2D(Body->GetActorLocation(), ExitLocation);
    FString Hint;
    if (auto* Loot = FocusLoot(Body)) Hint = FString::Printf(TEXT("F 回收 %s × %d"), *Loop->ItemName(Loot->ItemId).ToString(), Loot->Quantity);
    else if (Distance <= 240) Hint = ExitStarted < 0 ? TEXT("F 启动撤离（5 秒）") : TEXT("F 取消撤离");
    if (Distance > 240 && ExitStarted >= 0) { ExitStarted = -1; Loop->SetFeedback(TEXT("已离开撤离区域，倒计时取消。")); }
    const float Remaining = ExitStarted < 0 ? -1 : FMath::Max(0.f, 5.f - (GetWorld()->GetTimeSeconds() - ExitStarted));
    Loop->SetInteraction(Hint, Distance / 100.f, Remaining);
    if (Remaining == 0) { Loop->FinishExpedition(true); ExitStarted = -1; }
}
bool AFPSExtractionMode::DropCargo(AFPSExtractionController* PC, FName Id, int32 Quantity)
{
    auto* Body = PC ? Cast<AFPSDemoCharacter>(PC->GetPawn()) : nullptr;
    if (!bRaid || !Body || Body->IsEliminated()) return false;
    const FVector Start = Body->GetActorLocation() + Body->GetActorForwardVector() * 90;
    FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(ExtractionDrop), false, Body);
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - FVector(0,0,220), ECC_WorldStatic, Query)) return false;
    return SpawnCargo(Id, Quantity, Hit.ImpactPoint + FVector(0,0,15)) != nullptr;
}
void AFPSExtractionController::BeginPlay()
{
    AFPSDemoPlayerController::BeginPlay();
    if (!IsLocalController()) return;
    BackpackAction = NewObject<UInputAction>(this); EscapeAction = NewObject<UInputAction>(this); InteractAction = NewObject<UInputAction>(this);
    ExtractionContext = NewObject<UInputMappingContext>(this);
    ExtractionContext->MapKey(BackpackAction, EKeys::Tab); ExtractionContext->MapKey(EscapeAction, EKeys::Escape);
    ExtractionContext->MapKey(InteractAction, EKeys::F);
    if (auto* Input = Cast<UEnhancedInputComponent>(InputComponent))
    {
        Input->BindAction(BackpackAction, ETriggerEvent::Started, this, &AFPSExtractionController::ToggleBackpack);
        Input->BindAction(EscapeAction, ETriggerEvent::Started, this, &AFPSExtractionController::ToggleBackpack);
        Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AFPSExtractionController::Interact);
    }
    if (GetLocalPlayer()) if (auto* S = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer())) S->AddMappingContext(ExtractionContext, 30);
    ExtractionUI = UFPSExtractionUI::Open(this, true);
    if (ExtractionUI) ExtractionUI->AddToPlayerScreen(40);
}
void AFPSExtractionController::SetupInputComponent() { AFPSDemoPlayerController::SetupInputComponent(); }
bool AFPSExtractionController::IsExtractionPanelOpen() const { return ExtractionUI && ExtractionUI->IsPanelOpen(); }
void AFPSExtractionController::ToggleBackpack()
{
    if (ExtractionUI && GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>()->GetPhase() == EFPSExtractionPhase::Raid)
        ExtractionUI->SetPanelOpen(!ExtractionUI->IsPanelOpen());
}
void AFPSExtractionController::Interact() { if (auto* Mode = GetWorld()->GetAuthGameMode<AFPSExtractionMode>()) Mode->Interact(this); }
void AFPSExtractionController::Tick(float DeltaSeconds)
{
    AFPSDemoPlayerController::Tick(DeltaSeconds);
    const bool Open = IsExtractionPanelOpen();
    if (Open != bPreviousPanel)
    {
        bPreviousPanel = Open; SetIgnoreMoveInput(Open); SetIgnoreLookInput(Open); bShowMouseCursor = Open;
        if (auto* Body = Cast<AFPSDemoCharacter>(GetPawn())) Body->GetWeapons()->CancelForMenu();
        if (Open)
        {
            FInputModeGameAndUI Input; Input.SetWidgetToFocus(ExtractionUI->TakeWidget()); Input.SetHideCursorDuringCapture(false); SetInputMode(Input);
        }
        else SetInputMode(FInputModeGameOnly());
    }
}
void AFPSExtractionController::EndPlay(const EEndPlayReason::Type Reason)
{
    if (ExtractionUI) ExtractionUI->RemoveFromParent(); ExtractionUI = nullptr;
    if (GetLocalPlayer()) if (auto* S = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer())) S->RemoveMappingContext(ExtractionContext);
    AFPSDemoPlayerController::EndPlay(Reason);
}
