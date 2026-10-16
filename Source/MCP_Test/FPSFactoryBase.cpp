#include "FPSFactoryBase.h"
#include "FPSExtractionSubsystem.h"
#include "FPSExtractionSmoke.h"
#include "FPSExtractionUI.h"
#include "FPSDemoCharacter.h"
#include "FPSWeaponComponent.h"
#include "FPSSystemSettings.h"
#include "FPSDemoGameState.h"
#include "FPSWeaponData.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/CommandLine.h"

AFPSFactoryStation::AFPSFactoryStation()
{RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));}

AFPSFactoryRobot::AFPSFactoryRobot()
{
    PrimaryActorTick.bCanEverTick=true;bUseControllerRotationYaw=false;
    GetCapsuleComponent()->InitCapsuleSize(34,90);
    GetCharacterMovement()->bOrientRotationToMovement=true;GetCharacterMovement()->RotationRate=FRotator(0,540,0);
    GetCharacterMovement()->MaxWalkSpeed=330;GetCharacterMovement()->MaxAcceleration=1200;GetCharacterMovement()->BrakingDecelerationWalking=1600;
    Boom=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));Boom->SetupAttachment(RootComponent);
    Boom->TargetArmLength=310;Boom->SocketOffset=FVector(0,45,45);Boom->bUsePawnControlRotation=true;
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));Camera->SetupAttachment(Boom);Camera->FieldOfView=80;
    for(const TCHAR* Name:{TEXT("Torso"),TEXT("Helmet"),TEXT("Pelvis"),TEXT("LBoot"),TEXT("RBoot"),TEXT("LUpperArm"),TEXT("RUpperArm"),TEXT("LForearm"),TEXT("RForearm"),TEXT("LThigh"),TEXT("RThigh"),TEXT("LShin"),TEXT("RShin"),TEXT("LGlove"),TEXT("RGlove")})
    {
        auto* Part=CreateDefaultSubobject<UStaticMeshComponent>(Name);Part->SetupAttachment(RootComponent);
        const FString Path=FString::Printf(TEXT("/Game/FPSDemo/Art/Industrial/Character/Runtime/SM_IND_RT_%s.SM_IND_RT_%s"),Name,Name);
        ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(*Path);Part->SetStaticMesh(MeshAsset.Object);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCanEverAffectNavigation(false);Parts.Add(FName(Name),Part);
    }
}
void AFPSFactoryRobot::Segment(FName Name,const FVector& From,const FVector& To)
{
    const FVector Span=To-From;auto* Part=Parts[Name].Get();Part->SetRelativeLocation(From);Part->SetRelativeRotation(Span.Rotation());Part->SetRelativeScale3D(FVector(Span.Size(),1,1));
}
void AFPSFactoryRobot::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Speed=GetVelocity().Size2D(),Blend=FMath::Clamp(Speed/330.f,0.f,1.f);
    Gait=FMath::Fmod(Gait+Speed*DeltaSeconds*.023f,2*PI);
    const float Bob=FMath::Abs(FMath::Sin(Gait))*Blend*1.6f;
    Parts[TEXT("Pelvis")]->SetRelativeLocation(FVector(0,0,-17+Bob));
    Parts[TEXT("Torso")]->SetRelativeLocation(FVector(0,0,-1+Bob));
    Parts[TEXT("Helmet")]->SetRelativeLocation(FVector(0,0,53+Bob));
    for(int32 Side:{-1,1})
    {
        const FString P=Side<0?TEXT("L"):TEXT("R");const float Phase=Gait+(Side>0?PI:0);
        const float Stride=FMath::Sin(Phase)*20*Blend,Lift=FMath::Max(0.f,FMath::Cos(Phase))*8*Blend;
        const FVector Hip(0,Side*12,-17+Bob),Ankle(Stride,Side*12,-80+Lift),Knee=(Hip+Ankle)*.5f+FVector(8,0,0);
        Segment(FName(*(P+TEXT("Thigh"))),Hip,Knee);Segment(FName(*(P+TEXT("Shin"))),Knee,Ankle);Parts[FName(*(P+TEXT("Boot")))]->SetRelativeLocation(Ankle);
        const FVector Shoulder(0,Side*26,31+Bob),Elbow(-Stride*.45f,Side*29,5+Bob),Hand(5-Stride*.65f,Side*28,-19+Bob);
        Segment(FName(*(P+TEXT("UpperArm"))),Shoulder,Elbow);Segment(FName(*(P+TEXT("Forearm"))),Elbow,Hand);Parts[FName(*(P+TEXT("Glove")))]->SetRelativeLocation(Hand);
    }
}

TSharedRef<SWidget> UFPSFactoryTerminalShell::RebuildWidget()
{
    SetIsFocusable(true);
    auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Root;
    Terminal=UFPSExtractionUI::Open(GetOwningPlayer(),true);
    if(Terminal){auto* TerminalSlot=Root->AddChildToCanvas(Terminal);TerminalSlot->SetAnchors(FAnchors(0,0,1,1));TerminalSlot->SetOffsets(FMargin(0));}
    auto* Close=WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("FactoryClose"));auto* Label=WidgetTree->ConstructWidget<UTextBlock>();
    Label->SetText(FText::FromString(TEXT("返回车间 / Tab")));Label->SetFont(FSlateFontInfo(LoadObject<UFont>(nullptr,TEXT("/Game/FPSDemo/Art/Industrial/UI/Fonts/F_UI_Text.F_UI_Text")),16));
    Label->SetColorAndOpacity(FLinearColor(.9f,.71f,.2f));Close->AddChild(Label);Close->SetBackgroundColor(FLinearColor(.025f,.035f,.04f));
    auto* CloseSlot=Root->AddChildToCanvas(Close);CloseSlot->SetAnchors(FAnchors(0,1));CloseSlot->SetAlignment(FVector2D(0,1));CloseSlot->SetPosition(FVector2D(24,-12));CloseSlot->SetSize(FVector2D(190,28));CloseSlot->SetZOrder(20);
    Close->OnClicked.AddUniqueDynamic(this,&UFPSFactoryTerminalShell::CloseTerminal);
    return Super::RebuildWidget();
}
void UFPSFactoryTerminalShell::CloseTerminal(){if(auto* PC=Cast<AFPSFactoryController>(GetOwningPlayer()))PC->CloseTerminal();}
FReply UFPSFactoryTerminalShell::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(Event.GetKey()==EKeys::Tab){if(!Event.IsRepeat())CloseTerminal();return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
void AFPSFactoryController::BeginPlay()
{
    Super::BeginPlay();UseGameplayInput();SetControlRotation(FRotator(0,90,0));
}
void AFPSFactoryController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);if(!IsLocalController())return;
    if(!bLobbyStartupHandled&&GetPawn())
    {bLobbyStartupHandled=true;if(GetWorld()->URL.HasOption(TEXT("FactoryLobby"))||FParse::Param(FCommandLine::Get(),TEXT("FactoryLobby")))OpenLobby();}
    if(IsSettingsOpen()){if(WasInputKeyJustPressed(EKeys::Escape))SettingsBack();return;}
    if(Shell)
    {
        auto* Work=Shell->Terminal?Shell->Terminal->GetWidgetFromName(TEXT("ModificationPanel")):nullptr;
        if(auto* Close=Shell->GetWidgetFromName(TEXT("FactoryClose")))Close->SetVisibility(Work&&Work->IsVisible()?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
        return;
    }
    if(WasInputKeyJustPressed(EKeys::Tab)){OpenLobby();return;}
    if(WasInputKeyJustPressed(EKeys::Escape)){OpenSettings();return;}

}
AFPSFactoryStation* AFPSFactoryController::FocusStation() const
{
    const APawn* Body=GetPawn();if(!Body||Shell||IsSettingsOpen())return nullptr;
    AFPSFactoryStation* Best=nullptr;double Distance=FMath::Square(235.0);
    for(TActorIterator<AFPSFactoryStation> It(GetWorld());It;++It)
    {
        const FVector Delta=It->GetActorLocation()-Body->GetActorLocation();const double D=Delta.SizeSquared();
        if(D>=Distance||FVector::DotProduct(FRotator(0,GetControlRotation().Yaw,0).Vector(),Delta.GetSafeNormal2D())<.35f)continue;
        FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(FactoryFacility),false,Body);
        if(GetWorld()->LineTraceSingleByChannel(Hit,Body->GetActorLocation()+FVector(0,0,45),It->GetActorLocation(),ECC_Visibility,Query))continue;
        Best=*It;Distance=D;
    }
    return Best;
}
bool AFPSFactoryController::InteractWithStation()
{
    const auto* Station=FocusStation();if(!Station)return false;
    if(Station->Page!=TEXT("Warehouse")&&Station->Page!=TEXT("Production")&&Station->Page!=TEXT("Modification")&&Station->Page!=TEXT("Overview"))return false;
    return OpenTerminal(Station->Page);
}
bool AFPSFactoryController::OpenLobby(){return OpenTerminal(TEXT("Overview"));}
bool AFPSFactoryController::OpenTerminal(FName Page)
{
    if(IsSettingsOpen()||!GetPawn())return false;
    if(Shell){if(Shell->Terminal)Shell->Terminal->Dispatch(Page);return Shell->Terminal!=nullptr;}
    Shell=CreateWidget<UFPSFactoryTerminalShell>(this);Shell->AddToPlayerScreen(40);
    if(!Shell->Terminal){CloseTerminal();return false;}
    Shell->Terminal->Dispatch(Page);GetPawn()->GetMovementComponent()->StopMovementImmediately();
    if(auto* Body=Cast<AFPSDemoCharacter>(GetPawn()))Body->StopCombatInput();
    SetIgnoreMoveInput(true);SetIgnoreLookInput(true);UseMenuInput(Shell);return true;
}
UFPSExtractionUI* AFPSFactoryController::GetTerminal() const{return Shell?Shell->Terminal.Get():nullptr;}
void AFPSFactoryController::CloseTerminal()
{
    if(IsSettingsOpen())return;if(Shell)Shell->RemoveFromParent();Shell=nullptr;ResetIgnoreMoveInput();ResetIgnoreLookInput();UseGameplayInput();
}
void AFPSFactoryController::EndPlay(const EEndPlayReason::Type Reason)
{if(Shell)Shell->RemoveFromParent();Shell=nullptr;Super::EndPlay(Reason);}

AFPSFactoryHUD::AFPSFactoryHUD(){bShowHUD=false;}
AFPSFactoryMode::AFPSFactoryMode()
{
    // Keep the native first-person character inherited from FPSMatchMode.
    PlayerControllerClass=AFPSFactoryController::StaticClass();HUDClass=nullptr;
}
void AFPSFactoryMode::InitGameState()
{
    AGameModeBase::InitGameState();auto* GS=GetGameState<AFPSDemoGameState>();
    GS->bStructuredMatch=true;GS->bWaiting=false;GS->RoundEndsAt=0;GS->bBaseRegionsEnabled=false;
    auto* Catalog=GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    if(!Catalog->PrepareWorld(GetWorld())){GS->WeaponDataError=Catalog->ValidationError;return;}
    auto* Loop=GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>();Loop->bFactoryBase=true;Loop->EnterWorld(false);
}
void AFPSFactoryMode::BeginPlay(){AGameModeBase::BeginPlay();SyncPreparedEquipment();}
void AFPSFactoryMode::SyncPreparedEquipment()
{
    auto* Loop=GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>();
    auto* PC=GetWorld()->GetFirstPlayerController();auto* Body=PC?Cast<AFPSDemoCharacter>(PC->GetPawn()):nullptr;
    if(!Body||!Body->HasActorBegunPlay()||!Loop->Profile||Loop->GetPhase()!=EFPSExtractionPhase::Base)return;
    if(EquippedPawn.Get()==Body&&PreparedRevision==Loop->Profile->Revision)return;
    if(Loop->RestoreEquipment(Body)){EquippedPawn=Body;PreparedRevision=Loop->Profile->Revision;}
}
void AFPSFactoryMode::HandleStartingNewPlayer_Implementation(APlayerController* Player){AGameModeBase::HandleStartingNewPlayer_Implementation(Player);}
void AFPSFactoryMode::RestartPlayer(AController* Player){AGameModeBase::RestartPlayer(Player);}
void AFPSFactoryMode::Tick(float DeltaSeconds){AGameModeBase::Tick(DeltaSeconds);SyncPreparedEquipment();SmokeTick();FFPSExtractionSmoke::InventoryTick(this);FFPSExtractionSmoke::ModularPartsTick(this);}
