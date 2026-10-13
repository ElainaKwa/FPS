#include "FPSExtractionWeaponViewport.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/Font.h"
#include "UObject/ConstructorHelpers.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
const FVector2D SlotPositions[]={{.40,.11},{.84,.32},{.48,.88}};
const TCHAR* SlotLabels[]={TEXT("瞄具"),TEXT("枪口"),TEXT("弹匣")};
}
AFPSExtractionPreviewStage::AFPSExtractionPreviewStage()
{
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    ModelRoot=CreateDefaultSubobject<USceneComponent>(TEXT("ModelRoot"));ModelRoot->SetupAttachment(RootComponent);
    Core=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Core"));Core->SetupAttachment(ModelRoot);
    Core->SetRelativeLocation(FVector(-10,0,6));Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);Core->SetCastShadow(false);
    const TCHAR* Names[]={TEXT("Core"),TEXT("MagazineStandard"),TEXT("MuzzleStandard"),TEXT("Optic"),TEXT("Brake"),TEXT("MagazineExtended")};
    for(const TCHAR* Name:Names)
    {
        const FString Path=FString(TEXT("/Game/FPSDemo/Art/Weapons/M4/ExtractionPreview/SM_EXP_M4_"))+Name;
        ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(*(Path+TEXT(".")+FString(TEXT("SM_EXP_M4_"))+Name));
        PreviewMeshes.Add(Mesh.Object);
    }
    Core->SetStaticMesh(PreviewMeshes[0]);
    for(int32 I=0;I<3;++I)
    {
        auto* Part=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Part%d"),I));Part->SetupAttachment(Core);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCastShadow(false);Parts.Add(Part);
    }
    Capture=CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));Capture->SetupAttachment(RootComponent);
    Capture->SetRelativeLocation(FVector(0,190,34));Capture->SetRelativeRotation((-Capture->GetRelativeLocation()).Rotation());
    Capture->ProjectionType=ECameraProjectionMode::Orthographic;Capture->OrthoWidth=128;
    Capture->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR;
    Capture->bCaptureEveryFrame=false;Capture->bCaptureOnMovement=false;
    Capture->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Capture->ShowFlags.SetAtmosphere(false);Capture->ShowFlags.SetFog(false);Capture->ShowFlags.SetBloom(false);
    Capture->ShowFlags.SetDynamicShadows(false);Capture->ShowFlags.SetTemporalAA(false);
    Capture->PostProcessSettings.bOverride_AutoExposureMinBrightness=true;Capture->PostProcessSettings.AutoExposureMinBrightness=1;
    Capture->PostProcessSettings.bOverride_AutoExposureMaxBrightness=true;Capture->PostProcessSettings.AutoExposureMaxBrightness=1;
    for(int32 I=0;I<2;++I)
    {
        auto* Light=CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("Light%d"),I));Light->SetupAttachment(RootComponent);
        Light->SetRelativeLocation(I==0?FVector(-35,65,100):FVector(50,-45,40));
        Light->SetIntensity(I==0?6500:4000);Light->SetAttenuationRadius(450);Light->SetCastShadows(false);
    }
    SetActorEnableCollision(false);
}
bool AFPSExtractionPreviewStage::HasModel() const {return Core && Core->GetStaticMesh() && Target;}
FName AFPSExtractionPreviewStage::VisualFor(EFPSAttachmentCategory Category) const
{
    const int32 Index=int32(Category);
    return Parts.IsValidIndex(Index)&&Parts[Index]->GetStaticMesh()?Parts[Index]->GetStaticMesh()->GetFName():NAME_None;
}
bool AFPSExtractionPreviewStage::Present(const FFPSWeaponInstance& Instance)
{
    if(Instance.WeaponId!=TEXT("Weapon.M4"))return false;
    FString Key=Instance.InstanceId.ToString()+TEXT(":")+FString::FromInt(Instance.ConfigurationRevision);
    for(FName Id:Instance.AttachmentIds)Key+=Id.ToString();
    if(Key==Configuration)return false;
    Configuration=Key;
    Parts[0]->SetStaticMesh(Instance.AttachmentIds.Contains(TEXT("Attachment.M4_OpticFixed"))?PreviewMeshes[3]:nullptr);
    Parts[1]->SetStaticMesh(Instance.AttachmentIds.Contains(TEXT("Attachment.M4_MuzzleBrake"))?PreviewMeshes[4]:PreviewMeshes[2]);
    Parts[2]->SetStaticMesh(Instance.AttachmentIds.Contains(TEXT("Attachment.M4_MagazineExtended"))?PreviewMeshes[5]:PreviewMeshes[1]);
    Capture->ShowOnlyComponents.Empty();Capture->ShowOnlyComponent(Core);
    for(const auto& Part:Parts)if(Part->GetStaticMesh())Capture->ShowOnlyComponent(Part.Get());
    return true;
}
void AFPSExtractionPreviewStage::Frame(float Yaw,float Pitch,float Zoom,FIntPoint Size)
{
    if(!Target)
    {
        Target=NewObject<UTextureRenderTarget2D>(this);Target->RenderTargetFormat=RTF_RGBA8;Target->ClearColor=FLinearColor(.01f,.017f,.02f,1);
        Target->InitAutoFormat(Size.X,Size.Y);Target->UpdateResourceImmediate(true);Capture->TextureTarget=Target;
    }
    else if(Target->SizeX!=Size.X || Target->SizeY!=Size.Y)Target->ResizeTarget(Size.X,Size.Y);
    ModelRoot->SetRelativeRotation(FRotator(Pitch,Yaw,0));Capture->OrthoWidth=128*Zoom;
    Capture->CaptureScene();
}
FVector2D AFPSExtractionPreviewStage::ProjectMount(EFPSAttachmentCategory Category) const
{
    const FVector Local[]={FVector(10,0,10),FVector(59,0,0),FVector(15,0,-17)};
    const int32 Index=FMath::Clamp(int32(Category),0,2);
    const FVector Delta=Core->GetComponentTransform().TransformPosition(Local[Index])-Capture->GetComponentLocation();
    const float Aspect=Target?float(Target->SizeX)/FMath::Max(1,Target->SizeY):2;
    return FVector2D(.5+FVector::DotProduct(Delta,Capture->GetRightVector())/Capture->OrthoWidth,
        .5-FVector::DotProduct(Delta,Capture->GetUpVector())/(Capture->OrthoWidth/Aspect));
}
void UFPSExtractionWeaponViewport::Present(const FFPSWeaponInstance& Instance)
{
    PendingInstance=Instance;if(Stage && Stage->Present(Instance)){bFrameDirty=true;WarmupFrames=3;}
}
void UFPSExtractionWeaponViewport::SetPreviewActive(bool Active)
{
    bActive=Active;
    if(!Active){bOrbiting=false;return;}
    if(!Stage && GetWorld())
    {
        FActorSpawnParameters Params;Params.ObjectFlags|=RF_Transient;
        Stage=GetWorld()->SpawnActor<AFPSExtractionPreviewStage>(FVector(0,0,-100000),FRotator::ZeroRotator,Params);
        LabelFont=LoadObject<UFont>(nullptr,TEXT("/Game/FPSDemo/Art/Industrial/UI/Fonts/F_UI_Text.F_UI_Text"));
        if(Stage)Stage->Present(PendingInstance);
        bFrameDirty=true;WarmupFrames=3;
    }
}
void UFPSExtractionWeaponViewport::NativeDestruct()
{
    if(Stage)Stage->Destroy();Stage=nullptr;ImageBrush.SetResourceObject(nullptr);Super::NativeDestruct();
}
void UFPSExtractionWeaponViewport::UpdateFrame()
{
    if(!bActive || !Stage || FrameSize.X<16 || FrameSize.Y<16)return;
    Stage->Frame(Yaw,Pitch,Zoom,FrameSize);ImageBrush.SetResourceObject(Stage->GetImage());ImageBrush.ImageSize=FVector2D(FrameSize);
    bFrameDirty=false;
}
void UFPSExtractionWeaponViewport::NativeTick(const FGeometry& G,float DeltaTime)
{
    Super::NativeTick(G,DeltaTime);if(!bActive)return;
    const auto Size=G.GetLocalSize();const FIntPoint Desired(FMath::Clamp(FMath::RoundToInt32(Size.X*1.5),32,1600),FMath::Clamp(FMath::RoundToInt32(Size.Y*1.5),32,1000));
    if(Desired!=FrameSize){FrameSize=Desired;bFrameDirty=true;}
    if(bFrameDirty || WarmupFrames>0){UpdateFrame();WarmupFrames=FMath::Max(0,WarmupFrames-1);}
}
FReply UFPSExtractionWeaponViewport::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{
    if(!bActive)return FReply::Unhandled();
    if(E.GetEffectingButton()==EKeys::RightMouseButton){bOrbiting=true;return FReply::Handled().CaptureMouse(TakeWidget());}
    if(E.GetEffectingButton()==EKeys::LeftMouseButton)
    {
        const auto P=G.AbsoluteToLocal(E.GetScreenSpacePosition());
        for(int32 I=0;I<3;++I){const auto Delta=P-SlotPositions[I]*G.GetLocalSize();if(FMath::Abs(Delta.X)<54 && FMath::Abs(Delta.Y)<20)
        {Selected=EFPSAttachmentCategory(I);OnSlotSelected.Broadcast(Selected);return FReply::Handled();}}
    }
    return Super::NativeOnMouseButtonDown(G,E);
}
FReply UFPSExtractionWeaponViewport::NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E)
{if(E.GetEffectingButton()==EKeys::RightMouseButton){bOrbiting=false;return FReply::Handled().ReleaseMouseCapture();}return Super::NativeOnMouseButtonUp(G,E);}
FReply UFPSExtractionWeaponViewport::NativeOnMouseMove(const FGeometry& G,const FPointerEvent& E)
{if(bOrbiting){Yaw=FMath::UnwindDegrees(Yaw+E.GetCursorDelta().X*.35);Pitch=FMath::Clamp(Pitch+float(E.GetCursorDelta().Y)*.25f,-40.f,40.f);bFrameDirty=true;return FReply::Handled();}return Super::NativeOnMouseMove(G,E);}
FReply UFPSExtractionWeaponViewport::NativeOnMouseWheel(const FGeometry& G,const FPointerEvent& E)
{if(!bActive)return FReply::Unhandled();Zoom=FMath::Clamp(Zoom-E.GetWheelDelta()*.08f,.75f,1.6f);bFrameDirty=true;return FReply::Handled();}
int32 UFPSExtractionWeaponViewport::NativePaint(const FPaintArgs& A,const FGeometry& G,const FSlateRect& C,FSlateWindowElementList& E,int32 L,const FWidgetStyle& Style,bool Enabled) const
{
    L=Super::NativePaint(A,G,C,E,L,Style,Enabled);if(!bActive || !Stage || !Stage->GetImage())return L;
    FSlateDrawElement::MakeBox(E,L+1,G.ToPaintGeometry(),&ImageBrush);
    const auto* White=FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    for(int32 I=0;I<3;++I)
    {
        const auto Label=SlotPositions[I]*G.GetLocalSize(),Mount=Stage->ProjectMount(EFPSAttachmentCategory(I))*G.GetLocalSize();
        const FLinearColor Color=Selected==EFPSAttachmentCategory(I)?FLinearColor(.94f,.68f,.08f,1):FLinearColor(.62f,.70f,.70f,1);
        TArray<FVector2D> Points={Label,Mount};FSlateDrawElement::MakeLines(E,L+2,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color,true,1.3f);
        FSlateDrawElement::MakeBox(E,L+3,G.ToPaintGeometry(FVector2D(108,38),FSlateLayoutTransform(Label-FVector2D(54,19))),White,ESlateDrawEffect::None,FLinearColor(.025f,.033f,.036f,1));
        FSlateDrawElement::MakeText(E,L+4,G.ToPaintGeometry(FVector2D(100,35),FSlateLayoutTransform(Label-FVector2D(26,16))),FString(SlotLabels[I]),FSlateFontInfo(LabelFont.Get(),19),ESlateDrawEffect::None,Color);
        FSlateDrawElement::MakeBox(E,L+4,G.ToPaintGeometry(FVector2D(6,6),FSlateLayoutTransform(Mount-FVector2D(3,3))),White,ESlateDrawEffect::None,Color);
    }
    return L+4;
}
