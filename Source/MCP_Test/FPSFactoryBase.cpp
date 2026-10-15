#include "FPSFactoryBase.h"
#include "FPSExtractionSubsystem.h"
#include "FPSExtractionSmoke.h"
#include "FPSExtractionUI.h"
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
    Super::BeginPlay();bShowMouseCursor=false;SetInputMode(FInputModeGameOnly());SetControlRotation(FRotator(-12,90,0));
}
bool AFPSFactoryController::KeyHeld(FName Action) const
{
    const auto* Settings=UFPSSystemSettings::Get(this);const auto* Binding=Settings?Settings->Binding(Action):nullptr;
    return Binding&&((Binding->Primary.IsValid()&&IsInputKeyDown(Binding->Primary))||(Binding->Secondary.IsValid()&&IsInputKeyDown(Binding->Secondary)));
}
bool AFPSFactoryController::KeyPressed(FName Action) const
{
    const auto* Settings=UFPSSystemSettings::Get(this);const auto* Binding=Settings?Settings->Binding(Action):nullptr;
    return Binding&&((Binding->Primary.IsValid()&&WasInputKeyJustPressed(Binding->Primary))||(Binding->Secondary.IsValid()&&WasInputKeyJustPressed(Binding->Secondary)));
}
void AFPSFactoryController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);if(!IsLocalController())return;
    if(!bLobbyStartupHandled&&GetPawn())
    {bLobbyStartupHandled=true;if(FParse::Param(FCommandLine::Get(),TEXT("FactoryLobby")))OpenLobby();}
    if(IsSettingsOpen()){if(WasInputKeyJustPressed(EKeys::Escape))SettingsBack();return;}
    if(Shell)
    {
        auto* Work=Shell->Terminal?Shell->Terminal->GetWidgetFromName(TEXT("ModificationPanel")):nullptr;
        if(auto* Close=Shell->GetWidgetFromName(TEXT("FactoryClose")))Close->SetVisibility(Work&&Work->IsVisible()?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
        return;
    }
    if(WasInputKeyJustPressed(EKeys::Tab)){OpenLobby();return;}
    if(WasInputKeyJustPressed(EKeys::Escape)){OpenSettings();return;}
    auto* Body=Cast<AFPSFactoryRobot>(GetPawn());if(!Body)return;
    const auto* Settings=UFPSSystemSettings::Get(this);float DX=0,DY=0;GetInputMouseDelta(DX,DY);
    if(!IsLookInputIgnored())
    {auto Rot=GetControlRotation();Rot.Yaw+=DX*(Settings?Settings->Sensitivity:1.f);Rot.Pitch=FMath::Clamp(FRotator::NormalizeAxis(Rot.Pitch)+DY*(Settings&&Settings->InvertY?1.f:-1.f)*(Settings?Settings->Sensitivity:1.f),-55.f,20.f);SetControlRotation(Rot);}
    if(!IsMoveInputIgnored())
    {const FRotator Yaw(0,GetControlRotation().Yaw,0);Body->AddMovementInput(Yaw.Vector(),float(KeyHeld(TEXT("Forward")))-float(KeyHeld(TEXT("Backward"))));Body->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y),float(KeyHeld(TEXT("Right")))-float(KeyHeld(TEXT("Left"))));}
    if(KeyPressed(TEXT("Interact")))InteractWithStation();
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
    SetIgnoreMoveInput(true);SetIgnoreLookInput(true);bShowMouseCursor=true;FInputModeGameAndUI Mode;Mode.SetWidgetToFocus(Shell->TakeWidget());Mode.SetHideCursorDuringCapture(false);SetInputMode(Mode);return true;
}
UFPSExtractionUI* AFPSFactoryController::GetTerminal() const{return Shell?Shell->Terminal.Get():nullptr;}
void AFPSFactoryController::CloseTerminal()
{
    if(IsSettingsOpen())return;if(Shell)Shell->RemoveFromParent();Shell=nullptr;ResetIgnoreMoveInput();ResetIgnoreLookInput();bShowMouseCursor=false;SetInputMode(FInputModeGameOnly());
}
void AFPSFactoryController::EndPlay(const EEndPlayReason::Type Reason)
{if(Shell)Shell->RemoveFromParent();Shell=nullptr;Super::EndPlay(Reason);}

void AFPSFactoryHUD::DrawHUD()
{
    Super::DrawHUD();auto* PC=Cast<AFPSFactoryController>(PlayerOwner);if(!Canvas||!PC||PC->IsTerminalOpen()||PC->IsSettingsOpen())return;
    if(!Font)Font=LoadObject<UFont>(nullptr,TEXT("/Game/FPSDemo/Art/Industrial/UI/Fonts/F_UI_Text.F_UI_Text"));
    const float Scale=Canvas->ClipX/1600.f;
    DrawRect(FLinearColor(.015f,.022f,.026f,.85f),24*Scale,24*Scale,380*Scale,75*Scale);
    DrawText(TEXT("个人工厂 · 整备车间"),FLinearColor(.9f,.92f,.9f),40*Scale,31*Scale,Font,1.8f*Scale);
    DrawText(TEXT("仓储  /  加工  /  改枪  /  出征"),FLinearColor(.7f,.75f,.75f),40*Scale,65*Scale,Font,1.25f*Scale);
    DrawText(TEXT("Tab 行动大厅 · Esc 设置 · 移动与交互沿用自定义按键"),FLinearColor(.8f,.85f,.85f),30*Scale,Canvas->ClipY-42*Scale,Font,1.25f*Scale);
    if(auto* Station=PC->FocusStation())
    {
        const auto* Settings=UFPSSystemSettings::Get(this);const FString Hint=(Settings?Settings->BindingText(TEXT("Interact")):TEXT("F"))+TEXT("  ")+Station->DisplayName.ToString();
        DrawRect(FLinearColor(.02f,.025f,.03f,.9f),Canvas->ClipX*.5f-200*Scale,Canvas->ClipY*.78f,400*Scale,54*Scale);
        DrawText(Hint,FLinearColor(.95f,.72f,.15f),Canvas->ClipX*.5f-175*Scale,Canvas->ClipY*.78f+10*Scale,Font,1.7f*Scale);
    }
}
AFPSFactoryMode::AFPSFactoryMode()
{DefaultPawnClass=AFPSFactoryRobot::StaticClass();PlayerControllerClass=AFPSFactoryController::StaticClass();HUDClass=AFPSFactoryHUD::StaticClass();}
void AFPSFactoryMode::InitGameState()
{
    AGameModeBase::InitGameState();auto* GS=GetGameState<AFPSDemoGameState>();GS->bWaiting=true;GS->bStructuredMatch=false;GS->RoundEndsAt=0;
}
void AFPSFactoryMode::BeginPlay()
{
    AGameModeBase::BeginPlay();auto* Catalog=GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    if(!Catalog->PrepareDependencies({TEXT("Weapon.M4"),TEXT("Weapon.M9")})){GetGameState<AFPSDemoGameState>()->WeaponDataError=Catalog->ValidationError;return;}
    auto* Loop=GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>();Loop->bFactoryBase=true;Loop->EnterWorld(false);
}
void AFPSFactoryMode::HandleStartingNewPlayer_Implementation(APlayerController* Player){AGameModeBase::HandleStartingNewPlayer_Implementation(Player);}
void AFPSFactoryMode::RestartPlayer(AController* Player){AGameModeBase::RestartPlayer(Player);}
void AFPSFactoryMode::Tick(float DeltaSeconds){AGameModeBase::Tick(DeltaSeconds);SmokeTick();FFPSExtractionSmoke::InventoryTick(this);FFPSExtractionSmoke::ModularPartsTick(this);}
