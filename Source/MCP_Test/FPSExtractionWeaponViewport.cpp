#include "FPSExtractionWeaponViewport.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
const FVector2D SlotPositions[]={{.68,.17},{.13,.73},{.61,.83}};
const TCHAR* SlotLabels[]={TEXT("瞄具"),TEXT("枪口"),TEXT("弹匣")};
const FVector2D CardSize(156,104);
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
    // A matte emissive backdrop keeps the scene capture within the terminal's dark palette.
    Backdrop=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Backdrop"));Backdrop->SetupAttachment(RootComponent);
    ConstructorHelpers::FObjectFinder<UStaticMesh> BackdropMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    ConstructorHelpers::FObjectFinder<UMaterialInterface> BackdropMaterial(TEXT("/Game/FPSDemo/Art/Industrial/Materials/M_IND_Surface.M_IND_Surface"));
    Backdrop->SetStaticMesh(BackdropMesh.Object);Backdrop->SetMaterial(0,BackdropMaterial.Object);
    Backdrop->SetRelativeLocation(FVector(0,100,0));Backdrop->SetRelativeScale3D(FVector(20,.01,20));
    Backdrop->SetCollisionEnabled(ECollisionEnabled::NoCollision);Backdrop->SetCastShadow(false);
    for(int32 I=0;I<3;++I)
    {
        auto* Part=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Part%d"),I));Part->SetupAttachment(Core);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCastShadow(false);Parts.Add(Part);
    }
    Capture=CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));Capture->SetupAttachment(RootComponent);
    Capture->SetRelativeLocation(FVector(0,-190,12));Capture->SetRelativeRotation((-Capture->GetRelativeLocation()).Rotation());
    Capture->ProjectionType=ECameraProjectionMode::Orthographic;Capture->OrthoWidth=128;
    Capture->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR;
    Capture->bCaptureEveryFrame=false;Capture->bCaptureOnMovement=false;
    Capture->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Capture->ShowFlags.SetAtmosphere(false);Capture->ShowFlags.SetFog(false);Capture->ShowFlags.SetBloom(false);
    Capture->ShowFlags.SetDynamicShadows(false);Capture->ShowFlags.SetTemporalAA(false);
    Capture->ShowFlags.SetGrain(false);Capture->ShowFlags.SetVignette(false);Capture->ShowFlags.SetAmbientOcclusion(false);
    Capture->PostProcessSettings.bOverride_DynamicGlobalIlluminationMethod=true;Capture->PostProcessSettings.DynamicGlobalIlluminationMethod=EDynamicGlobalIlluminationMethod::None;
    Capture->PostProcessSettings.bOverride_ReflectionMethod=true;Capture->PostProcessSettings.ReflectionMethod=EReflectionMethod::None;
    Capture->PostProcessSettings.bOverride_AutoExposureMinBrightness=true;Capture->PostProcessSettings.AutoExposureMinBrightness=1;
    Capture->PostProcessSettings.bOverride_AutoExposureMaxBrightness=true;Capture->PostProcessSettings.AutoExposureMaxBrightness=1;
    for(int32 I=0;I<2;++I)
    {
        auto* Light=CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("Light%d"),I));Light->SetupAttachment(RootComponent);
        Light->SetRelativeLocation(I==0?FVector(-35,-65,100):FVector(50,45,40));
        Light->SetIntensity(I==0?6500:4000);Light->SetAttenuationRadius(450);Light->SetCastShadows(false);
    }
    SetActorEnableCollision(false);
}
bool AFPSExtractionPreviewStage::HasModel() const {return Core && Core->GetStaticMesh() && Target;}
FName AFPSExtractionPreviewStage::WeaponVisualName() const {return Core&&Core->GetStaticMesh()?Core->GetStaticMesh()->GetFName():NAME_None;}
FName AFPSExtractionPreviewStage::VisualFor(EFPSAttachmentCategory Category) const
{
    const int32 Index=int32(Category);
    return Parts.IsValidIndex(Index)&&Parts[Index]->GetStaticMesh()?Parts[Index]->GetStaticMesh()->GetFName():NAME_None;
}
bool AFPSExtractionPreviewStage::Present(const FFPSWeaponInstance& Instance)
{
    FString Key=Instance.WeaponId.ToString()+Instance.InstanceId.ToString()+TEXT(":")+FString::FromInt(Instance.ConfigurationRevision);
    for(FName Id:Instance.AttachmentIds)Key+=Id.ToString();
    if(Key==Configuration)return false;
    Configuration=Key;bIconsDirty=true;
    if(!Cast<UMaterialInstanceDynamic>(Backdrop->GetMaterial(0)))if(auto* Material=Backdrop->CreateDynamicMaterialInstance(0))
    {
        Material->SetVectorParameterValue(TEXT("BaseColor"),FLinearColor::Black);
        Material->SetScalarParameterValue(TEXT("Metallic"),1.f);Material->SetScalarParameterValue(TEXT("Roughness"),1.f);
        Material->SetVectorParameterValue(TEXT("Emissive"),FLinearColor(.029f,.037f,.041f,1));
    }
    const bool M4=Instance.WeaponId==TEXT("Weapon.M4");
    UStaticMesh* Mesh=M4?PreviewMeshes[0].Get():nullptr;
    if(!M4)if(const auto* Catalog=GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>())if(const auto* Definition=Catalog->Find(Instance.WeaponId))Mesh=Definition->Pickup.Mesh.LoadSynchronous();
    Core->SetStaticMesh(Mesh);Core->SetRelativeLocation(M4?FVector(-10,0,6):FVector::ZeroVector);
    ModelWidth=M4?142.f:Mesh?FMath::Max(40.f,Mesh->GetBoundingBox().GetSize().X*1.65f):128.f;
    Parts[0]->SetStaticMesh(M4&&Instance.AttachmentIds.Contains(TEXT("Attachment.M4_OpticFixed"))?PreviewMeshes[3]:nullptr);
    Parts[1]->SetStaticMesh(M4?(Instance.AttachmentIds.Contains(TEXT("Attachment.M4_MuzzleBrake"))?PreviewMeshes[4]:PreviewMeshes[2]):nullptr);
    Parts[2]->SetStaticMesh(M4?(Instance.AttachmentIds.Contains(TEXT("Attachment.M4_MagazineExtended"))?PreviewMeshes[5]:PreviewMeshes[1]):nullptr);
    PartLabels={TEXT("机械瞄具"),TEXT("标准枪口"),TEXT("标准弹匣")};
    if(Instance.AttachmentIds.Contains(TEXT("Attachment.M4_OpticFixed")))PartLabels[0]=TEXT("固定倍率瞄具");
    if(Instance.AttachmentIds.Contains(TEXT("Attachment.M4_MuzzleBrake")))PartLabels[1]=TEXT("枪口制退器");
    if(Instance.AttachmentIds.Contains(TEXT("Attachment.M4_MagazineExtended")))PartLabels[2]=TEXT("扩容弹匣");
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
    // Thumbnails show the actual installed mesh, captured separately from the orbiting assembly.
    if(bIconsDirty)
    {
        ModelRoot->SetRelativeRotation(FRotator::ZeroRotator);
        Icons.SetNum(3);
        for(int32 I=0;I<3;++I)
        {
            if(!Parts[I]->GetStaticMesh()){Icons[I]=nullptr;continue;}
            if(!Icons[I]){Icons[I]=NewObject<UTextureRenderTarget2D>(this);Icons[I]->RenderTargetFormat=RTF_RGBA8;Icons[I]->ClearColor=FLinearColor(.01f,.017f,.02f,1);Icons[I]->InitAutoFormat(192,112);Icons[I]->UpdateResourceImmediate(true);}
            Parts[I]->UpdateBounds();const auto Bounds=Parts[I]->Bounds;
            const FVector Center=Bounds.Origin;Capture->SetWorldLocation(Center+FVector(0,-120,8));Capture->SetWorldRotation((Center-Capture->GetComponentLocation()).Rotation());
            Capture->OrthoWidth=FMath::Max(8.f,FMath::Max(Bounds.BoxExtent.X*2,Bounds.BoxExtent.Z*2*192.f/112.f)*1.25f);
            Capture->TextureTarget=Icons[I];Capture->ShowOnlyComponents.Empty();Capture->ShowOnlyComponent(Backdrop);Capture->ShowOnlyComponent(Parts[I]);Capture->CaptureScene();
        }
        bIconsDirty=false;
    }
    ModelRoot->SetRelativeRotation(FRotator(Pitch,Yaw,0));Core->UpdateBounds();const FVector Center=Core->Bounds.Origin;
    Capture->SetWorldLocation(Center+FVector(0,-190,12));Capture->SetWorldRotation((Center-Capture->GetComponentLocation()).Rotation());
    Capture->TextureTarget=Target;Capture->ShowOnlyComponents.Empty();Capture->ShowOnlyComponent(Backdrop);Capture->ShowOnlyComponent(Core);for(const auto& Part:Parts)if(Part->GetStaticMesh())Capture->ShowOnlyComponent(Part);
    Capture->OrthoWidth=ModelWidth*Zoom;
    Capture->CaptureScene();
}
UTextureRenderTarget2D* AFPSExtractionPreviewStage::IconFor(EFPSAttachmentCategory Category) const
{return Icons.IsValidIndex(int32(Category))?Icons[int32(Category)].Get():nullptr;}
FString AFPSExtractionPreviewStage::PartLabel(EFPSAttachmentCategory Category) const
{return PartLabels.IsValidIndex(int32(Category))?PartLabels[int32(Category)]:TEXT("");}
FVector2D UFPSExtractionWeaponViewport::SlotCenter(EFPSAttachmentCategory Category)
{return SlotPositions[FMath::Clamp(int32(Category),0,2)];}
void UFPSExtractionWeaponViewport::ResetView(){Yaw=bOverview?-18.f:0.f;Pitch=0;Zoom=1;bFrameDirty=true;}
void UFPSExtractionWeaponViewport::SetOverviewMode(bool Overview)
{if(bOverview!=Overview){bOverview=Overview;ResetView();}bSlotsVisible=!Overview;}
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
    for(int32 I=0;I<3;++I){IconBrushes[I].SetResourceObject(Stage->IconFor(EFPSAttachmentCategory(I)));IconBrushes[I].ImageSize=FVector2D(192,112);}
    bFrameDirty=false;
}
void UFPSExtractionWeaponViewport::NativeTick(const FGeometry& G,float DeltaTime)
{
    Super::NativeTick(G,DeltaTime);if(!bActive)return;
    const auto Size=G.GetLocalSize();const float Scale=FMath::Min(1.5f,FMath::Min(2048.f/FMath::Max(1.f,float(Size.X)),1152.f/FMath::Max(1.f,float(Size.Y))));
    const FIntPoint Desired(FMath::Max(32,FMath::RoundToInt32(Size.X*Scale)),FMath::Max(32,FMath::RoundToInt32(Size.Y*Scale)));
    if(Desired!=FrameSize){FrameSize=Desired;bFrameDirty=true;}
    if(bFrameDirty || WarmupFrames>0){UpdateFrame();WarmupFrames=FMath::Max(0,WarmupFrames-1);}
}
FReply UFPSExtractionWeaponViewport::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{
    if(!bActive)return FReply::Unhandled();
    if(E.GetEffectingButton()==EKeys::RightMouseButton){bOrbiting=true;return FReply::Handled().CaptureMouse(TakeWidget());}
    if(bSlotsVisible&&E.GetEffectingButton()==EKeys::LeftMouseButton)
    {
        const auto P=G.AbsoluteToLocal(E.GetScreenSpacePosition());
        for(int32 I=0;I<3;++I){const auto Delta=P-SlotPositions[I]*G.GetLocalSize();if(FMath::Abs(Delta.X)<CardSize.X/2 && FMath::Abs(Delta.Y)<CardSize.Y/2)
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
    if(!bSlotsVisible)return L+1;
    const auto* White=FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    for(int32 I=0;I<3;++I)
    {
        const auto Label=SlotPositions[I]*G.GetLocalSize(),Mount=Stage->ProjectMount(EFPSAttachmentCategory(I))*G.GetLocalSize();
        const FLinearColor Color=Selected==EFPSAttachmentCategory(I)?FLinearColor(.94f,.68f,.08f,1):FLinearColor(.62f,.70f,.70f,1);
        const FVector2D Start=Label+FVector2D(0,Mount.Y>Label.Y?CardSize.Y/2:-CardSize.Y/2),Top=Label-CardSize/2;
        TArray<FVector2D> Points={Start,Mount};FSlateDrawElement::MakeLines(E,L+2,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color.CopyWithNewOpacity(.6f),true,1.f);
        FSlateDrawElement::MakeBox(E,L+3,G.ToPaintGeometry(CardSize,FSlateLayoutTransform(Top)),White,ESlateDrawEffect::None,FLinearColor(.018f,.027f,.031f,.97f));
        TArray<FVector2D> Border={Top,Top+FVector2D(CardSize.X,0),Top+CardSize,Top+FVector2D(0,CardSize.Y),Top};FSlateDrawElement::MakeLines(E,L+4,G.ToPaintGeometry(),Border,ESlateDrawEffect::None,Color.CopyWithNewOpacity(.65f),true,1.f);
        FSlateDrawElement::MakeText(E,L+4,G.ToPaintGeometry(FVector2D(156,28),FSlateLayoutTransform(Top-FVector2D(0,28))),FString(SlotLabels[I]),FSlateFontInfo(LabelFont.Get(),17),ESlateDrawEffect::None,Color);
        if(IconBrushes[I].GetResourceObject())FSlateDrawElement::MakeBox(E,L+4,G.ToPaintGeometry(FVector2D(132,66),FSlateLayoutTransform(Top+FVector2D(12,5))),&IconBrushes[I]);
        else FSlateDrawElement::MakeText(E,L+4,G.ToPaintGeometry(FVector2D(140,40),FSlateLayoutTransform(Top+FVector2D(18,25))),TEXT("+"),FSlateFontInfo(LabelFont.Get(),24),ESlateDrawEffect::None,Color);
        FSlateDrawElement::MakeText(E,L+5,G.ToPaintGeometry(FVector2D(148,25),FSlateLayoutTransform(Top+FVector2D(7,77))),Stage->PartLabel(EFPSAttachmentCategory(I)),FSlateFontInfo(LabelFont.Get(),14),ESlateDrawEffect::None,FLinearColor(.8f,.84f,.83f,1));
        TArray<FVector2D> MountBox={Mount-FVector2D(4,4),Mount+FVector2D(4,-4),Mount+FVector2D(4,4),Mount+FVector2D(-4,4),Mount-FVector2D(4,4)};FSlateDrawElement::MakeLines(E,L+5,G.ToPaintGeometry(),MountBox,ESlateDrawEffect::None,Color,true,1.f);
    }
    return L+5;
}
