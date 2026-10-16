#include "FPSInterface.h"
#if WITH_EDITOR
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/EditableTextBox.h"
#include "Components/ProgressBar.h"
#include "Components/ButtonSlot.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Engine/Texture2D.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "FPSUIStyle.h"
#include "FPSExtractionUI.h"

namespace
{
const FString BaseRoot=TEXT("/Game/FPSDemo/Art/Industrial/UI/");
FString Root=BaseRoot;
FString FontSourceDirectory;
UFPSUITheme* Theme;
UFont* TextFont;
UFont* NumberFont;
FLinearColor RGB(uint8 R,uint8 G,uint8 B,uint8 A=255){return FLinearColor::FromSRGBColor(FColor(R,G,B,A));}
UTexture2D* Texture(const FString& Name){return LoadObject<UTexture2D>(nullptr,*(BaseRoot+TEXT("T_UI_")+Name+TEXT(".T_UI_")+Name));}
bool Save(UObject* Object)
{
    Object->MarkPackageDirty();FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    return UPackage::SavePackage(Object->GetOutermost(),Object,*FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),Args);
}
bool ApplyMinimalStamina(UWidgetBlueprint* BP)
{
    auto* Panel=CastChecked<UCanvasPanel>(BP->WidgetTree->FindWidget(TEXT("StaminaPanel")));
    auto* Bar=CastChecked<UProgressBar>(BP->WidgetTree->FindWidget(TEXT("StaminaBar")));
    auto* PanelSlot=CastChecked<UCanvasPanelSlot>(Panel->Slot);
    auto* BarSlot=CastChecked<UCanvasPanelSlot>(Bar->Slot);
    const FVector2D Position(672.5f,740),Size(255,9);
    auto* Delay=Cast<UProgressBar>(BP->WidgetTree->FindWidget(TEXT("StaminaDelayBar")));
    bool Changed=PanelSlot->GetPosition()!=Position||PanelSlot->GetSize()!=Size||
        BarSlot->GetPosition()!=FVector2D::ZeroVector||BarSlot->GetSize()!=Size||
        Bar->GetFillColorAndOpacity()!=FLinearColor::White||!Delay||
        !BP->WidgetVariableNameToGuidMap.Contains(TEXT("StaminaDelayBar"));
    for(const TCHAR* Name:{TEXT("StaminaBG"),TEXT("StaminaLabel"),TEXT("StaminaValue"),TEXT("StaminaStatus")})
        Changed|=BP->WidgetTree->FindWidget(Name)->GetVisibility()!=ESlateVisibility::Collapsed;
    if(!Changed)return false;
    BP->Modify();BP->WidgetTree->Modify();PanelSlot->Modify();BarSlot->Modify();Bar->Modify();
    PanelSlot->SetPosition(Position);PanelSlot->SetSize(Size);
    BarSlot->SetPosition(FVector2D::ZeroVector);BarSlot->SetSize(Size);
    // Keep old named widgets collapsed so incremental extension remains compatible.
    for(const TCHAR* Name:{TEXT("StaminaBG"),TEXT("StaminaLabel"),TEXT("StaminaValue"),TEXT("StaminaStatus")})
    {auto* Widget=BP->WidgetTree->FindWidget(Name);Widget->Modify();Widget->SetVisibility(ESlateVisibility::Collapsed);}
    if(!Delay)Delay=BP->WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(),TEXT("StaminaDelayBar"));
    if(!BP->WidgetVariableNameToGuidMap.Contains(Delay->GetFName()))BP->OnVariableAdded(Delay->GetFName());
    auto* DelaySlot=Cast<UCanvasPanelSlot>(Delay->Slot);
    if(!DelaySlot)DelaySlot=Panel->AddChildToCanvas(Delay);
    DelaySlot->SetPosition(FVector2D::ZeroVector);DelaySlot->SetSize(Size);DelaySlot->SetZOrder(0);BarSlot->SetZOrder(1);
    FProgressBarStyle Trail;Trail.BackgroundImage=FSlateColorBrush(FLinearColor(0,0,0,.28f));
    Trail.FillImage=FSlateColorBrush(FLinearColor::White);
    Delay->SetWidgetStyle(Trail);Delay->SetFillColorAndOpacity(FLinearColor(.45f,.45f,.45f,1));
    Delay->SetPercent(1);Delay->SetBarFillType(EProgressBarFillType::LeftToRight);Delay->SetBorderPadding(FVector2D::ZeroVector);
    FProgressBarStyle Flat;Flat.BackgroundImage=FSlateColorBrush(FLinearColor::Transparent);
    Flat.FillImage=FSlateColorBrush(FLinearColor::White);
    Bar->SetWidgetStyle(Flat);Bar->SetFillColorAndOpacity(FLinearColor::White);
    Bar->SetBarFillType(EProgressBarFillType::LeftToRight);Bar->SetBorderPadding(FVector2D::ZeroVector);
    return true;
}
UFont* BuildFont(const TCHAR* Name,const TArray<TPair<FName,FString>>& Sources)
{
    const FString Path=Root+TEXT("Fonts/")+Name;
    auto* Font=LoadObject<UFont>(nullptr,*(Path+TEXT(".")+Name));
    if(!Font){Font=NewObject<UFont>(CreatePackage(*Path),Name,RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(Font);}
    Font->FontCacheType=EFontCacheType::Runtime;Font->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.Empty();
    for(const auto& Source:Sources)
    {
        const FString FaceName=FString(Name)+TEXT("_")+Source.Key.ToString(),FacePath=Root+TEXT("Fonts/")+FaceName;
        auto* Face=LoadObject<UFontFace>(nullptr,*(FacePath+TEXT(".")+FaceName));
        if(!Face){Face=NewObject<UFontFace>(CreatePackage(*FacePath),*FaceName,RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(Face);}
        const FString File=FPaths::ConvertRelativePathToFull(FontSourceDirectory/Source.Value);
        TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*File))return nullptr;
        Face->SourceFilename=File;Face->Hinting=EFontHinting::Default;Face->FontFaceData->SetData(MoveTemp(Bytes));Face->CacheSubFaces();Face->LoadingPolicy=EFontLoadingPolicy::Inline;
        if(!Save(Face))return nullptr;
        FTypefaceEntry Entry(Source.Key);Entry.Font=FFontData(Face);Font->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.Add(Entry);
    }
    return Save(Font)?Font:nullptr;
}
struct FPage
{
    UWidgetBlueprint* BP;
    UWidgetTree* Tree;
    UCanvasPanel* Canvas;
    explicit FPage(UWidgetBlueprint* Existing)
        : BP(Existing), Tree(Existing ? Existing->WidgetTree : nullptr),
          Canvas(Tree ? Cast<UCanvasPanel>(Tree->FindWidget(TEXT("Layout"))) : nullptr) {}
    explicit FPage(const TCHAR* Page, const FString& Directory=TEXT(""))
    {
        const FString Name=FString(TEXT("WBP_FPS"))+Page,Path=(Directory.IsEmpty()?Root+TEXT("Widgets/"):Directory)+Name;
        BP=LoadObject<UWidgetBlueprint>(nullptr,*(Path+TEXT(".")+Name));
        if(!BP){auto* Factory=NewObject<UWidgetBlueprintFactory>();Factory->ParentClass=UFPSInterface::StaticClass();BP=CastChecked<UWidgetBlueprint>(Factory->FactoryCreateNew(UWidgetBlueprint::StaticClass(),CreatePackage(*Path),*Name,RF_Public|RF_Standalone,nullptr,GWarn));FAssetRegistryModule::AssetCreated(BP);}
        if(BP->WidgetTree)BP->WidgetTree->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
        Tree=NewObject<UWidgetTree>(BP,TEXT("WidgetTree"),RF_Transactional);BP->WidgetTree=Tree;
        auto* Scale=Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(),TEXT("ViewportFit"));Scale->SetStretch(EStretch::ScaleToFit);
        auto* Size=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("DesignSize"));Size->SetWidthOverride(1600);Size->SetHeightOverride(900);Scale->AddChild(Size);
        Canvas=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("Layout"));Size->AddChild(Canvas);
        auto* Overlay=Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(),TEXT("Viewport"));Tree->RootWidget=Overlay;
        auto* Fill=Tree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("ViewportFill"));
        Fill->SetBrush(FSlateColorBrush(FLinearColor::White));Fill->SetColorAndOpacity(FString(Page)==TEXT("HUD")?FLinearColor::Transparent:FString(Page)==TEXT("Loadout")?FLinearColor(0,0,0,.28f):Theme->Background);Fill->SetVisibility(ESlateVisibility::HitTestInvisible);
        for(UWidget* Child:{static_cast<UWidget*>(Fill),static_cast<UWidget*>(Scale)}){auto* S=Overlay->AddChildToOverlay(Child);S->SetHorizontalAlignment(HAlign_Fill);S->SetVerticalAlignment(VAlign_Fill);}
    }
    template<class T>T* Make(const FString& Name){checkf(!Tree->FindWidget(FName(*Name)),TEXT("Duplicate UI widget: %s"),*Name);return Tree->ConstructWidget<T>(T::StaticClass(),FName(*Name));}
    void Place(UCanvasPanel* Parent,UWidget* W,float X,float Y,float Width,float Height)
    {auto* S=Parent->AddChildToCanvas(W);S->SetPosition({X,Y});S->SetSize({Width,Height});}
    UCanvasPanel* Group(const FString& Name,float X,float Y,float W,float H,UCanvasPanel* Parent=nullptr)
    {auto* C=Make<UCanvasPanel>(Name);C->SetVisibility(ESlateVisibility::SelfHitTestInvisible);Place(Parent?Parent:Canvas,C,X,Y,W,H);return C;}
    UImage* Rect(const FString& Name,float X,float Y,float W,float H,FLinearColor Color,UCanvasPanel* Parent=nullptr,bool Cut=false)
    {
        auto* I=Make<UImage>(Name);FSlateBrush Brush;
        if(Cut&&Theme->PanelTexture){Brush.SetResourceObject(Theme->PanelTexture);Brush.DrawAs=ESlateBrushDrawType::Box;Brush.Margin=FMargin(.20f);Brush.ImageSize=FVector2D(128,128);}
        else Brush.DrawAs=ESlateBrushDrawType::Image;
        I->SetBrush(Brush);I->SetColorAndOpacity(Color);I->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Parent?Parent:Canvas,I,X,Y,W,H);return I;
    }
    UImage* Surface(const FString& Name,const FString& Asset,float X,float Y,float W,float H,UCanvasPanel* Parent=nullptr,float Alpha=1)
    {auto* I=Rect(Name,X,Y,W,H,FLinearColor(1,1,1,Alpha),Parent);I->SetBrushFromTexture(Texture(Asset));return I;}
    void Line(const FString& Name,float X,float Y,float X2,float Y2,FLinearColor Color,UCanvasPanel* Parent=nullptr,float Thickness=1)
    {auto* I=Rect(Name,X,Y,FVector2D(X2-X,Y2-Y).Size(),Thickness,Color,Parent);I->SetRenderTransformPivot(FVector2D::ZeroVector);I->SetRenderTransformAngle(FMath::RadiansToDegrees(FMath::Atan2(Y2-Y,X2-X)));}
    void Frame(const FString& Name,float X,float Y,float W,float H,FLinearColor Color,UCanvasPanel* Parent=nullptr,float Thickness=1.25f)
    {Rect(Name+TEXT("Top"),X,Y,W,Thickness,Color,Parent);Rect(Name+TEXT("Bottom"),X,Y+H-Thickness,W,Thickness,Color,Parent);Rect(Name+TEXT("Left"),X,Y,Thickness,H,Color,Parent);Rect(Name+TEXT("Right"),X+W-Thickness,Y,Thickness,H,Color,Parent);}
    void RoundFrame(const FString& Name,float X,float Y,float W,float H,float Radius,FLinearColor Color,UCanvasPanel* Parent=nullptr,float Thickness=1.5f)
    {auto* I=Rect(Name,X,Y,W,H,FLinearColor::White,Parent);I->SetBrush(FSlateRoundedBoxBrush(FLinearColor::Transparent,Radius,Color,Thickness));}
    void Glyph(const FString& Name,const FString& Shape,float X,float Y,float W,float H,FLinearColor Color,UCanvasPanel* Parent=nullptr)
    {Icon(Name,Texture(Shape),X,Y,W,H,Color,Parent);}
    UTextBlock* Label(const FString& Name,const FString& Value,float X,float Y,float W,float H,int Size,FLinearColor Color,UCanvasPanel* Parent=nullptr,bool Bold=false)
    {
        auto* T=Make<UTextBlock>(Name);T->SetText(FText::FromString(Value));
        const bool Numeric=Name==TEXT("PreviewEnglish")||Name==TEXT("Health")||Name==TEXT("Ammo")||Name==TEXT("Reserve")||Name==TEXT("Timer")||Name.EndsWith(TEXT("Score"))||Name.StartsWith(TEXT("NavNumber"))||Name==TEXT("DurationValue")||Name==TEXT("Kills")||Name==TEXT("Deaths");
        FSlateFontInfo Font(Numeric?NumberFont:TextFont,Size,Numeric?FName("Regular"):Bold?FName("Bold"):FName("Regular"));
        if(Name==TEXT("Subtitle")||(Name==TEXT("BotsHint")||Name==TEXT("DifficultyHint")||Name==TEXT("DurationHint"))||Name==TEXT("TerminalCode")){Font=FSlateFontInfo(NumberFont,Size,TEXT("Regular"));Font.LetterSpacing=150;}
        if(Name==TEXT("Title")&&Size>=44)Font.LetterSpacing=140;
        T->SetFont(Font);
        T->SetColorAndOpacity(Color);T->SetAutoWrapText(true);T->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Parent?Parent:Canvas,T,X,Y,W,H);return T;
    }
    void Icon(const FString& Name,UTexture2D* Texture,float X,float Y,float W,float H,FLinearColor Color,UCanvasPanel* Parent=nullptr)
    {
        auto* S=Make<UScaleBox>(Name+TEXT("Fit"));S->SetStretch(EStretch::ScaleToFit);S->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* I=Make<UImage>(Name);FSlateBrush Brush;Brush.DrawAs=ESlateBrushDrawType::Image;Brush.SetResourceObject(Texture);
        // Source dimensions also exist in NullRHI authoring commandlets; GetSizeX can be zero there.
        Brush.ImageSize=Texture?FVector2D(Texture->Source.GetSizeX(),Texture->Source.GetSizeY()):FVector2D(512,256);
        I->SetBrush(Brush);I->SetColorAndOpacity(Color);S->AddChild(I);Place(Parent?Parent:Canvas,S,X,Y,W,H);
    }
    UFPSActionButton* Button(const FString& Name,const FString& Value,FName Action,float X,float Y,float W,float H,int Style=0,UCanvasPanel* Parent=nullptr)
    {
        auto* B=Make<UFPSActionButton>(Name);B->Action=Action;B->SetStyle(Style==1?Theme->LightButton:Style==2?Theme->AccentButton:Theme->DarkButton);
        auto* C=Make<UCanvasPanel>(Name+TEXT("Content"));B->AddChild(C);auto* Slot=CastChecked<UButtonSlot>(C->Slot);Slot->SetPadding(FMargin(0));Slot->SetHorizontalAlignment(HAlign_Fill);Slot->SetVerticalAlignment(VAlign_Fill);
        auto* L=Label(Name+TEXT("Label"),Value,22,(H-38)/2-3,W-40,48,20,Style?Theme->Ink:Theme->Paper,C,true);L->SetAutoWrapText(false);
        Place(Parent?Parent:Canvas,B,X,Y,W,H);return B;
    }
    void Edit(const FString& Name,const FString& Value,float X,float Y,float W,float H,UCanvasPanel* Parent)
    {
        auto* E=Make<UEditableTextBox>(Name);E->SetText(FText::FromString(Value));
        FEditableTextBoxStyle S;S.BackgroundImageNormal=Theme->DarkButton.Normal;S.BackgroundImageHovered=Theme->DarkButton.Hovered;S.BackgroundImageFocused=Theme->DarkButton.Pressed;S.BackgroundImageReadOnly=S.BackgroundImageNormal;S.ForegroundColor=Theme->Paper;S.FocusedForegroundColor=Theme->Paper;S.Padding=FMargin(14,2);S.TextStyle.SetFont(FSlateFontInfo(Name==TEXT("BotInput")?NumberFont:TextFont,Name==TEXT("BotInput")?30:20,FName("Regular")));E->SetWidgetStyle(S);if(Name==TEXT("BotInput"))E->SetJustification(ETextJustify::Center);Place(Parent,E,X,Y,W,H);
    }
    void Backdrop()
    {
        Surface(TEXT("Backdrop"),TEXT("Backdrop"),0,0,1600,900);
        const auto Fine=RGB(65,76,77,65);
        for(int I=0;I<14;++I)Rect(FString::Printf(TEXT("GridV%d"),I),38+I*117,0,.65f,900,Fine);
        const float Rows[]={64,355,450,571,637,847};
        for(int I=0;I<6;++I)Rect(FString::Printf(TEXT("GridH%d"),I),0,Rows[I],1600,.65f,Fine);
        Line(TEXT("BackgroundDiagonal1"),28,730,175,583,Fine);Line(TEXT("BackgroundDiagonal2"),245,748,368,625,Fine);
        Rect(TEXT("TopRegistrationRule"),38,43,343,1,RGB(102,112,112,95));
        Label(TEXT("TerminalCode"),TEXT("//   OPERATIONS TERMINAL"),1385,35,190,22,9,Theme->Muted);
    }
    void Help()
    {
        auto* G=Group(TEXT("HelpPanel"),409,485,1148,350);Rect(TEXT("HelpBG"),0,0,1148,350,Theme->Ink,G)->SetVisibility(ESlateVisibility::Visible);Rect(TEXT("HelpAccent"),0,0,5,350,Theme->Accent,G);
        Label(TEXT("HelpTitle"),TEXT("操作帮助  /  FIELD GUIDE"),30,24,1000,50,28,Theme->Paper,G,true);
        Label(TEXT("HelpText"),TEXT("WASD 移动    Space 跳跃    Shift 冲刺    Ctrl 蹲伏\n左键射击    右键瞄准    R 换弹    Q 检视    1 / 2 切枪\nG 丢弃    F 拾取    P 配装    Esc 对局菜单\n\n点击配装卡片立即装备；首次离开基地后，本次生命不能再次选枪。"),30,90,1080,220,22,Theme->Paper,G);
        G->SetVisibility(ESlateVisibility::Collapsed);
    }
    bool Finish(const TCHAR* Page)
    {
        TSet<FName> CurrentNames;Tree->ForEachWidget([&](UWidget* W){CurrentNames.Add(W->GetFName());});
        for(auto It=BP->WidgetVariableNameToGuidMap.CreateIterator();It;++It)if(!CurrentNames.Contains(It.Key()))It.RemoveCurrent();
        Tree->ForEachWidget([&](UWidget* W){if(!BP->WidgetVariableNameToGuidMap.Contains(W->GetFName()))BP->OnVariableAdded(W->GetFName());});
        FKismetEditorUtilities::CompileBlueprint(BP);
        if(BP->Status==BS_Error||!BP->GeneratedClass)return false;
        auto* Default=CastChecked<UFPSInterface>(BP->GeneratedClass->GetDefaultObject());Default->Screen=Page;Default->Theme=Theme;
        return Save(BP);
    }
    void AlignMiSans(const FString& Page)
    {
        // Optical offsets measured in the 1600x900 design canvas from MiSans ink,
        // not from TextBlock line boxes. Keep numeric score/timer slots mirrored.
        auto Shift=[&](const FString& Name,float Y)
        {
            if(auto* T=Cast<UTextBlock>(Tree->FindWidget(FName(*Name))))
                if(auto* S=Cast<UCanvasPanelSlot>(T->Slot))S->SetPosition(S->GetPosition()+FVector2D(0,Y));
        };
        if(Page==TEXT("Lobby"))
        {
            for(int I=0;I<3;++I){Shift(FString::Printf(TEXT("Nav%dTitle"),I),5);Shift(FString::Printf(TEXT("NavNumber%d"),I),3);}
            Shift(TEXT("HelpNavTitle"),2);Shift(TEXT("QuitNavTitle"),2);
            Shift(TEXT("DifficultyLabel"),5.5f);Shift(TEXT("StartLabel"),3.5f);
            Shift(TEXT("HostLabel"),5);Shift(TEXT("JoinLabel"),5);
            Shift(TEXT("SectionTitle"),3);Shift(TEXT("PreviewTitle"),1);
            Label(TEXT("FontNotice"),TEXT("本软件使用 MiSans 字体"),1110,875,460,20,10,Theme->Muted)->SetJustification(ETextJustify::Right);
        }
        else if(Page==TEXT("Loadout"))
        {
            Shift(TEXT("Title"),3);Shift(TEXT("CloseKeys"),3.5f);Shift(TEXT("CloseCaption"),1);
            Shift(TEXT("BaseStatus"),1);Shift(TEXT("PrimaryTitle"),1);Shift(TEXT("SecondaryTitle"),1);
            for(const TCHAR* Id:{TEXT("M4"),TEXT("AK"),TEXT("MP5"),TEXT("BR3"),TEXT("UMP45"),TEXT("M9"),TEXT("HeavyPistol")})
                Shift(FString(TEXT("Card"))+Id+TEXT("BadgeText"),1.5f);
            Shift(TEXT("EquippedState"),1);Shift(TEXT("LoadoutRule"),1.5f);Shift(TEXT("LiveNotice"),1.5f);
        }
        else if(Page==TEXT("HUD"))
        {
            Shift(TEXT("BlueLabel"),2.5f);Shift(TEXT("RedLabel"),2.5f);Shift(TEXT("TrainingText"),2.5f);
            Shift(TEXT("Team"),2.5f);Shift(TEXT("KeyR"),2);Shift(TEXT("GunAction"),2);Shift(TEXT("GunStatus"),2);
            Shift(TEXT("KillText"),2);Shift(TEXT("KillValue"),2);
            Shift(TEXT("PickupKeyF"),2);
            if(auto* T=Cast<UTextBlock>(Tree->FindWidget(TEXT("PickupText"))))T->SetRenderTranslation(FVector2D(0,0));
        }
        else
        {
            Shift(TEXT("Title"),3);
            for(const TCHAR* Name:{TEXT("ReturnLabel"),TEXT("HelpLabel"),TEXT("SwitchTeamLabel"),TEXT("StartRoundLabel"),TEXT("ResumeLabel")})Shift(Name,5);
        }
    }
};
void Lobby(FPage& P)
{
    P.Backdrop();P.Rect(TEXT("SidebarRule"),381,0,1,900,RGB(92,102,103,105));
    P.Button(TEXT("Modes"),TEXT("←  模式选择"),TEXT("Modes"),51,565,305,54);
    P.Label(TEXT("Title"),TEXT("多人对战"),57,77,320,79,44,Theme->Paper,nullptr,true);
    P.Label(TEXT("Subtitle"),TEXT("O P E R A T I O N S"),63,151,290,36,19,Theme->Muted);
    const TCHAR* Names[]={TEXT("单机训练"),TEXT("联机对局"),TEXT("靶场")};const TCHAR* Actions[]={TEXT("Solo"),TEXT("LAN"),TEXT("Range")};
    for(int I=0;I<3;++I)
    {
        const float Y=219+I*105;const FString N=FString::Printf(TEXT("Nav%d"),I);
        auto* B=P.Button(N,TEXT(""),Actions[I],41,Y,328,87);auto* C=CastChecked<UCanvasPanel>(B->GetContent());
        FButtonStyle Style=Theme->DarkButton;Style.Normal=FSlateColorBrush(FLinearColor::Transparent);Style.Hovered=FSlateColorBrush(RGB(75,84,87,70));B->SetStyle(Style);
        auto* Selection=P.Group(FString::Printf(TEXT("NavAccent%d"),I),0,0,328,87,C);
        P.Surface(N+TEXT("Surface"),TEXT("Surface"),0,0,328,87,Selection);
        P.Rect(N+TEXT("Stripe"),0,0,8,87,Theme->Accent,Selection);
        P.Line(N+TEXT("Diagonal"),276,87,327,36,RGB(169,177,177,190),Selection);
        P.Glyph(N+TEXT("CutMark"),TEXT("Corner"),310,0,18,18,RGB(168,174,178,120),Selection);
        P.Label(FString::Printf(TEXT("NavNumber%d"),I),FString::Printf(TEXT("%02d"),I+1),22,10,74,70,38,Theme->Paper,C);
        P.Rect(N+TEXT("Divider"),94,24,1,38,Theme->Muted,C);
        P.Label(N+TEXT("Title"),Names[I],119,17,208,60,25,Theme->Paper,C,true);
        P.Rect(N+TEXT("Rule"),9,91,310,1,RGB(64,76,77,90),C);
    }
    for(int I=0;I<2;++I)
    {
        const float Y=739+I*61;const FString N=I==0?TEXT("Help"):TEXT("Quit");
        auto* B=P.Button(N,TEXT(""),FName(*N),51,Y,305,48);FButtonStyle S=Theme->DarkButton;S.Normal=FSlateColorBrush(FLinearColor::Transparent);B->SetStyle(S);
        auto* C=CastChecked<UCanvasPanel>(B->GetContent());P.Glyph(N+TEXT("Icon"),I==0?TEXT("Book"):TEXT("Exit"),11,11,25,25,Theme->Paper,C);
        P.Label(N+TEXT("NavTitle"),I==0?TEXT("操作帮助"):TEXT("退出游戏"),57,8,225,40,17,Theme->Paper,C);
        P.Rect(N+TEXT("Rule"),0,52,305,1,RGB(106,117,117,125),C);P.Rect(N+TEXT("Dash"),8,0,16,1,Theme->Muted,C);
    }
    P.Rect(TEXT("HelpTopRule"),76,730,280,1,RGB(75,90,90,110));
    P.Surface(TEXT("FoundryPreview"),TEXT("Foundry"),410,90,1014,387);
    P.Surface(TEXT("PreviewTextShade"),TEXT("PreviewShade"),410,90,1014,170);
    P.Surface(TEXT("PreviewRail"),TEXT("Backdrop"),1424,90,133,387);
    P.Frame(TEXT("PreviewFrame"),409,89,1148,389,RGB(140,151,151,185));
    P.Label(TEXT("PreviewTitle"),TEXT("铸造场 /"),432,101,210,52,29,Theme->Paper,nullptr,true);
    P.Label(TEXT("PreviewEnglish"),TEXT("FOUNDRY"),598,102,620,52,31,Theme->Paper);
    P.Label(TEXT("PreviewCaption"),TEXT("训练场景"),432,146,650,34,17,Theme->Paper);
    P.Label(TEXT("PreviewID"),TEXT("TRAINING FACILITY\nFOUNDRY"),1444,134,114,52,8,Theme->Muted);
    P.Rect(TEXT("PreviewRailDash"),1444,187,41,2,Theme->Muted);
    P.Line(TEXT("PreviewRailDiagonal1"),1435,409,1557,287,RGB(65,77,79,100));
    P.Line(TEXT("PreviewRailDiagonal2"),1464,477,1557,384,RGB(113,125,125,105));
    P.Line(TEXT("PreviewRailDiagonal3"),1462,390,1547,475,RGB(65,77,79,80));
    P.Frame(TEXT("PreviewRailSquare"),1508,427,28,28,Theme->Muted);
    P.Line(TEXT("PreviewRailSquareCut"),1508,455,1536,427,Theme->Muted);
    P.Glyph(TEXT("PreviewCorner"),TEXT("Corner"),1541,90,16,16,RGB(168,174,178,100));
    P.Rect(TEXT("SectionAccent"),409,519,8,41,Theme->Accent);
    P.Label(TEXT("SectionTitle"),TEXT("单机训练"),442,506,870,70,36,Theme->Paper,nullptr,true);
    P.Label(TEXT("SectionNote"),TEXT("本地对局"),443,572,1080,39,18,Theme->Muted);
    auto* Solo=P.Group(TEXT("SoloPanel"),409,622,1148,98);
    P.Surface(TEXT("BotsBG"),TEXT("Paper"),0,0,542,98,Solo);P.Frame(TEXT("BotsFrame"),0,0,542,98,RGB(234,235,233,160),Solo);
    P.Label(TEXT("BotsLabel"),TEXT("人机数量"),34,21,201,40,21,Theme->Ink,Solo,true);P.Label(TEXT("BotsHint"),TEXT("BOT COUNT"),35,59,200,25,10,Theme->Ink,Solo);
    P.Button(TEXT("Minus"),TEXT("−"),TEXT("Minus"),231,22,61,56,1,Solo);P.Frame(TEXT("MinusFrame"),231,22,61,56,Theme->Ink,Solo);
    P.Edit(TEXT("BotInput"),TEXT("3"),297,22,135,56,Solo);
    P.Button(TEXT("Plus"),TEXT("+"),TEXT("Plus"),437,22,61,56,1,Solo);P.Frame(TEXT("PlusFrame"),437,22,61,56,Theme->Ink,Solo);
    P.Surface(TEXT("DifficultyBG"),TEXT("Paper"),568,0,580,98,Solo);P.Frame(TEXT("DifficultyFrame"),568,0,580,98,RGB(234,235,233,160),Solo);
    P.Label(TEXT("DifficultyTitle"),TEXT("人机难度"),603,21,220,40,21,Theme->Ink,Solo,true);P.Label(TEXT("DifficultyHint"),TEXT("BOT DIFFICULTY"),604,59,225,24,10,Theme->Ink,Solo);
    P.Button(TEXT("Difficulty"),TEXT("普通"),TEXT("Difficulty"),798,19,332,61,0,Solo);
    auto* Difficulty=CastChecked<UTextBlock>(P.Tree->FindWidget(TEXT("DifficultyLabel")));Difficulty->SetJustification(ETextJustify::Center);
    P.Glyph(TEXT("DifficultyChevron"),TEXT("Chevron"),1084,39,23,23,Theme->Paper,Solo);
    auto* LAN=P.Group(TEXT("LANPanel"),409,610,1148,201);
    P.Label(TEXT("RoomPrompt"),TEXT("房间名称"),0,0,510,35,20,Theme->Paper,LAN);P.Edit(TEXT("RoomInput"),TEXT("FPS Room"),0,43,550,52,LAN);P.Button(TEXT("Host"),TEXT("创建房间    →"),TEXT("Host"),0,115,550,62,2,LAN);
    P.Label(TEXT("AddressPrompt"),TEXT("房主 IPv4 地址 · 可带端口"),590,0,550,35,20,Theme->Paper,LAN);P.Edit(TEXT("AddressInput"),TEXT("127.0.0.1"),590,43,558,52,LAN);P.Button(TEXT("Join"),TEXT("加入房间    →"),TEXT("Join"),590,115,558,62,1,LAN);
    auto* Range=P.Group(TEXT("RangePanel"),409,622,1148,98);P.Rect(TEXT("RangeBG"),0,0,1148,98,Theme->Panel,Range);P.Label(TEXT("RangeText"),TEXT("自由熟悉七种武器、配装、换弹与移动。\n无对局时限，随时返回行动终端。"),28,12,1080,80,22,Theme->Paper,Range);
    auto* Duration=P.Group(TEXT("DurationPanel"),409,744,761,73);
    P.Rect(TEXT("FooterRule"),0,0,761,1,Theme->Muted,Duration);P.Rect(TEXT("FooterBottomRule"),0,72,761,1,Theme->Muted,Duration);
    P.Label(TEXT("DurationTitle"),TEXT("对局时长"),34,16,170,32,16,Theme->Paper,Duration);P.Label(TEXT("DurationHint"),TEXT("MATCH DURATION"),35,43,160,22,9,Theme->Muted,Duration);
    P.Label(TEXT("DurationValue"),TEXT("05:00"),165,10,190,58,29,Theme->Paper,Duration);
    auto* Start=P.Button(TEXT("Start"),TEXT("开始训练"),TEXT("Start"),1198,743,359,74,2);
    auto* C=CastChecked<UCanvasPanel>(Start->GetContent());auto* L=CastChecked<UTextBlock>(P.Tree->FindWidget(TEXT("StartLabel")));L->SetFont(FSlateFontInfo(TextFont,26,TEXT("Bold")));P.Place(C,L,42,11,245,55);
    P.Glyph(TEXT("StartArrow"),TEXT("Arrow"),204,23,35,30,Theme->Ink,C);
    P.Line(TEXT("StartCorner"),322,74,359,37,Theme->Ink,C);
    P.Frame(TEXT("StartFrame"),0,0,359,74,RGB(255,222,100),C);
    P.Label(TEXT("Error"),TEXT(""),410,847,1130,44,17,Theme->Red);P.Help();
}
void Loadout(FPage& P)
{
    auto* G=P.Group(TEXT("LoadoutPanel"),168,75,1265,718);
    P.Surface(TEXT("LoadoutBG"),TEXT("Surface"),0,0,1265,718,G,.98f);P.Frame(TEXT("LoadoutFrame"),0,0,1265,718,RGB(132,143,143,150),G);
    P.Rect(TEXT("HeaderAccent"),16,19,5,47,Theme->Accent,G);
    P.Label(TEXT("Title"),TEXT("本局配装"),50,9,430,70,34,Theme->Paper,G,true);P.Label(TEXT("Subtitle"),TEXT("LOADOUT"),246,33,180,33,17,Theme->Muted,G);
    for(int I=0;I<3;++I)P.Line(FString::Printf(TEXT("HeaderSlash%d"),I),380+I*29,66,415+I*29,19,RGB(80,91,93,24),G,12);
    auto* Close=P.Button(TEXT("Close"),TEXT(""),TEXT("Close"),1091,25,153,36,0,G);FButtonStyle S=Theme->DarkButton;S.Normal=FSlateColorBrush(FLinearColor::Transparent);Close->SetStyle(S);
    // Center the visible Latin glyphs optically within the keycap, accounting for font ascent.
    auto* CC=CastChecked<UCanvasPanel>(Close->GetContent());P.RoundFrame(TEXT("CloseKeyFrame"),0,0,99,35,3,Theme->Muted,CC);P.Label(TEXT("CloseKeys"),TEXT("P / Esc"),12,-0.5f,75,33,17,Theme->Paper,CC)->SetJustification(ETextJustify::Center);P.Label(TEXT("CloseCaption"),TEXT("关闭"),111,3,52,35,16,Theme->Paper,CC);
    P.Rect(TEXT("HeaderRule"),0,78,1265,1,RGB(122,133,133,130),G);P.Glyph(TEXT("BaseShield"),TEXT("Shield"),34,93,23,26,Theme->Paper,G);
    P.Label(TEXT("BaseStatus"),TEXT("基地内 · 可更换装备"),73,88,1000,45,19,Theme->Paper,G);P.Rect(TEXT("StatusRule"),0,132,1265,1,RGB(122,133,133,100),G);
    P.Rect(TEXT("PrimaryAccent"),34,149,4,20,Theme->Accent,G);P.Label(TEXT("PrimaryTitle"),TEXT("主武器"),50,140,350,43,22,Theme->Paper,G,true);P.Rect(TEXT("PrimaryRule"),34,181,310,1,Theme->Muted,G);
    P.Rect(TEXT("SecondaryAccent"),395,149,4,20,Theme->Accent,G);P.Label(TEXT("SecondaryTitle"),TEXT("副武器"),410,140,300,43,22,Theme->Paper,G,true);P.Rect(TEXT("SecondaryRule"),395,181,291,1,Theme->Muted,G);
    P.Rect(TEXT("DetailRule"),709,132,1,519,RGB(122,133,133,130),G);
    auto* Scroll=P.Make<UScrollBox>(TEXT("PrimaryScroll"));Scroll->SetScrollbarThickness(FVector2D(6,6));Scroll->SetScrollbarPadding(FMargin(16,25,9,25));Scroll->SetAlwaysShowScrollbar(true);Scroll->SetAlwaysShowScrollbarTrack(true);Scroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::InstantScroll);
    FScrollBarStyle ScrollStyle;ScrollStyle.NormalThumbImage=FSlateColorBrush(RGB(176,183,182));ScrollStyle.HoveredThumbImage=FSlateColorBrush(Theme->Paper);ScrollStyle.DraggedThumbImage=FSlateColorBrush(Theme->Accent);ScrollStyle.VerticalBackgroundImage=FSlateColorBrush(RGB(49,58,60));Scroll->SetWidgetBarStyle(ScrollStyle);P.Place(G,Scroll,34,192,345,449);
    P.Glyph(TEXT("ScrollUp"),TEXT("Triangle"),363,200,10,8,Theme->Muted,G);P.Glyph(TEXT("ScrollDown"),TEXT("Triangle"),363,623,10,8,Theme->Muted,G);P.Tree->FindWidget(TEXT("ScrollDownFit"))->SetRenderTransformAngle(180);
    const TCHAR* IDs[]={TEXT("M4"),TEXT("AK"),TEXT("MP5"),TEXT("BR3"),TEXT("UMP45"),TEXT("M9"),TEXT("HeavyPistol")};
    for(int I=0;I<7;++I)
    {
        const FString Key=FString(TEXT("Card"))+IDs[I];const FName Id(*(FString(TEXT("Weapon."))+IDs[I]));
        const float W=I<5?310:291,H=I==0?155:I<5?140:159;
        UCanvasPanel* Parent=G;
        if(I<5){auto* Size=P.Make<USizeBox>(Key+TEXT("Size"));Size->SetHeightOverride(H+10);auto* Slot=CastChecked<UScrollBoxSlot>(Scroll->AddChild(Size));Slot->SetHorizontalAlignment(HAlign_Fill);Parent=P.Make<UCanvasPanel>(Key+TEXT("Layout"));Size->AddChild(Parent);}
        auto* B=P.Button(Key,TEXT(""),TEXT("Equip"),I<5?0:398,I<5?0:192+(I-5)*169,W,H,1,Parent);B->WeaponId=Id;auto* C=CastChecked<UCanvasPanel>(B->GetContent());
        FButtonStyle CardStyle=Theme->LightButton;
        for(FSlateBrush* Brush:{&CardStyle.Normal,&CardStyle.Hovered,&CardStyle.Pressed}){Brush->SetResourceObject(Texture(TEXT("Paper")));Brush->DrawAs=ESlateBrushDrawType::RoundedBox;Brush->OutlineSettings=FSlateBrushOutlineSettings(6.f);Brush->ImageSize=FVector2D(512,256);Brush->Margin=FMargin(0);}
        B->SetStyle(CardStyle);
        P.Label(Key+TEXT("Name"),I==6?TEXT("重型手枪"):FString(IDs[I]),20,9,230,40,20,Theme->Ink,C,true);
        P.Icon(Key+TEXT("Icon"),FPSUI::WeaponPreviewTexture(Id),18,47,W-38,H-51,RGB(44,50,51),C);
        auto* Edge=P.Group(Key+TEXT("Accent"),0,0,W,H,C);P.RoundFrame(Key+TEXT("Outline"),0,0,W,H,6,Theme->Accent,Edge,2);P.Rect(Key+TEXT("LeftStrip"),1,6,5,H-12,Theme->Accent,Edge);
        auto* Badge=P.Group(Key+TEXT("Badge"),W-100,11,86,29,C);P.Rect(Key+TEXT("BadgeBG"),0,0,86,29,Theme->Accent,Badge);P.Glyph(Key+TEXT("Check"),TEXT("CheckCircle"),7,7,16,16,Theme->Ink,Badge);P.Label(Key+TEXT("BadgeText"),TEXT("已装备"),32,3,64,29,12,Theme->Ink,Badge,true);
    }
    P.Label(TEXT("WeaponName"),TEXT("M4"),744,138,358,76,43,Theme->Paper,G,true);P.Label(TEXT("WeaponType"),TEXT("主武器  /  步枪"),746,207,490,35,18,Theme->Paper,G);
    P.Frame(TEXT("DetailBadgeFrame"),1130,158,98,33,Theme->Accent,G);P.Rect(TEXT("DetailBadgeIconBG"),1130,158,32,33,Theme->Accent,G);P.Glyph(TEXT("DetailCheck"),TEXT("Check"),1136,164,21,21,Theme->Ink,G);P.Label(TEXT("EquippedState"),TEXT("已装备"),1169,161,71,31,13,Theme->Accent,G,true);
    P.Icon(TEXT("WeaponPreview"),FPSUI::WeaponPreviewTexture(TEXT("Weapon.M4")),727,264,523,213,FLinearColor::White,G);
    P.Rect(TEXT("StatsRule"),746,491,481,2,RGB(91,102,103,135),G);
    P.Label(TEXT("ChooseHint"),TEXT("//   点击武器立即选择"),746,520,490,44,18,Theme->Paper,G);
    P.Label(TEXT("Feedback"),TEXT("//   成功选择后，本局自动保存"),746,561,490,65,17,Theme->Paper,G);
    P.Rect(TEXT("BottomRule"),0,651,1265,1,RGB(122,133,133,130),G);P.Glyph(TEXT("RuleIcon"),TEXT("Alert"),34,671,26,26,Theme->Paper,G);
    P.Label(TEXT("LoadoutRule"),TEXT(""),77,668,940,48,17,Theme->Paper,G);P.Rect(TEXT("RuleTail"),535,684,468,1,RGB(150,162,162,140),G);
    P.Glyph(TEXT("LiveIcon"),TEXT("Signal"),1100,670,29,29,Theme->Paper,G);P.Label(TEXT("LiveNotice"),TEXT("对局仍在进行"),1140,672,129,32,13,Theme->Paper,G);
}
void MatchPage(FPage& P,const FString& Page)
{
    P.Backdrop();P.Label(TEXT("Title"),Page==TEXT("Report")?TEXT("行动报告"):Page==TEXT("Waiting")?TEXT("行动编组"):TEXT("对局菜单"),55,74,650,78,44,Theme->Paper,nullptr,true);
    P.Label(TEXT("RoomTitle"),TEXT("铸造场 / FOUNDRY"),60,166,1000,36,23,Theme->Muted);P.Rect(TEXT("HeaderRule"),55,221,1490,1,Theme->Muted*.5f);
    P.Label(TEXT("SectionCode"),Page==TEXT("Report")?TEXT("AFTER ACTION / REPORT"):TEXT("SQUAD OPERATIONS / LIVE"),1130,108,400,30,15,Theme->Accent);
    P.Label(TEXT("SessionNote"),TEXT(""),60,249,1440,44,21,Theme->Muted);
    if(Page==TEXT("Report"))
    {
        P.Rect(TEXT("PersonalBG"),55,316,540,379,Theme->Paper,nullptr,true);P.Label(TEXT("PersonalTitle"),TEXT("本次行动  /  PERSONAL"),84,342,490,40,22,Theme->Ink,nullptr,true);P.Label(TEXT("PlayerName"),TEXT(""),84,403,480,50,28,Theme->Ink);
        P.Label(TEXT("Kills"),TEXT("00"),84,468,220,104,70,Theme->Ink,nullptr,true);P.Label(TEXT("Deaths"),TEXT("00"),328,468,220,104,70,Theme->Ink,nullptr,true);P.Label(TEXT("KillsLabel"),TEXT("击倒"),89,602,190,40,22,Theme->Ink);P.Label(TEXT("DeathsLabel"),TEXT("死亡"),333,602,190,40,22,Theme->Ink);
        for(int I=0;I<3;++I){float Y=316+I*132;P.Rect(FString::Printf(TEXT("RankBG%d"),I),625,Y,920,115,Theme->Panel,nullptr,true);P.Label(FString::Printf(TEXT("Rank%d"),I),FString::Printf(TEXT("%02d"),I+1),653,Y+24,100,70,44,I==0?Theme->Accent:Theme->Muted);P.Label(FString::Printf(TEXT("RankName%d"),I),TEXT(""),777,Y+17,720,44,26,Theme->Paper,nullptr,true);P.Label(FString::Printf(TEXT("RankScore%d"),I),TEXT(""),780,Y+66,710,34,18,Theme->Muted);}
    }
    else for(int T=0;T<2;++T)
    {
        float X=55+T*762;auto* G=P.Group(FString::Printf(TEXT("Team%dPanel"),T),X,316,728,379);P.Rect(FString::Printf(TEXT("Team%dBG"),T),0,0,728,379,Theme->Panel,G,true);P.Rect(FString::Printf(TEXT("Team%dAccent"),T),0,0,5,379,T==0?Theme->Blue:Theme->Red,G);
        P.Label(FString::Printf(TEXT("Team%dTitle"),T),T==0?TEXT("■  蓝队"):TEXT("▲  红队"),27,14,530,48,28,T==0?Theme->Blue:Theme->Red,G,true);P.Label(FString::Printf(TEXT("Team%dCount"),T),TEXT("0 / 5"),606,23,110,35,22,Theme->Muted,G);
        for(int I=0;I<5;++I){float Y=84+I*57;P.Rect(FString::Printf(TEXT("Team%dRow%d"),T,I),22,Y,683,1,Theme->Muted*.3f,G);P.Label(FString::Printf(TEXT("Team%dName%d"),T,I),TEXT(""),29,Y+12,405,42,21,Theme->Paper,G);P.Label(FString::Printf(TEXT("Team%dState%d"),T,I),TEXT(""),470,Y+15,235,36,16,Theme->Muted,G);}
    }
    P.Button(TEXT("Return"),TEXT("返回大厅   ↗"),TEXT("Return"),55,748,280,65,0);P.Button(TEXT("Help"),TEXT("操作帮助"),TEXT("Help"),355,748,230,65,0);
    if(Page==TEXT("Waiting")){P.Button(TEXT("SwitchTeam"),TEXT("切换阵营"),TEXT("SwitchTeam"),948,748,245,65,1);P.Button(TEXT("StartRound"),TEXT("开始对局    →"),TEXT("StartRound"),1213,748,332,65,2);}
    else if(Page==TEXT("Pause"))P.Button(TEXT("Resume"),TEXT("继续游戏    →"),TEXT("Resume"),1213,748,332,65,2);
    P.Label(TEXT("LeaveNote"),TEXT(""),60,840,1450,32,16,Theme->Muted);P.Label(TEXT("Error"),TEXT(""),600,706,940,32,18,Theme->Red);P.Help();
}
void HUD(FPage& P)
{
    auto* G=P.Group(TEXT("HUDCanvas"),0,0,1600,900);const auto Panel=FLinearColor(.009f,.014f,.017f,.85f);
    auto* Scores=P.Group(TEXT("ScoreGroup"),546,23,508,50,G);P.Rect(TEXT("ScoreBG"),0,0,508,50,Panel,Scores);
    P.Rect(TEXT("BlueMarker"),13,14,22,22,Theme->Blue,Scores);P.Label(TEXT("BlueLabel"),TEXT("蓝队"),53,9.5f,51,32,16,Theme->Paper,Scores)->SetJustification(ETextJustify::Center);P.Label(TEXT("BlueScore"),TEXT("00"),108,6,43,40,24,Theme->Paper,Scores,true)->SetJustification(ETextJustify::Center);
    P.Rect(TEXT("ScoreSep1"),153,10,1,29,RGB(142,152,157,90),Scores);P.Label(TEXT("Timer"),TEXT("05:00"),154,.5f,200,48,31,Theme->Paper,Scores,true)->SetJustification(ETextJustify::Center);P.Rect(TEXT("ScoreSep2"),354,10,1,29,RGB(142,152,157,90),Scores);
    // Mirror score/name slots around the timer center (254), including their widths.
    P.Label(TEXT("RedScore"),TEXT("00"),357,6,43,40,24,Theme->Paper,Scores,true)->SetJustification(ETextJustify::Center);P.Label(TEXT("RedLabel"),TEXT("红队"),404,9.5f,51,32,16,Theme->Paper,Scores)->SetJustification(ETextJustify::Center);P.Glyph(TEXT("RedMarker"),TEXT("Triangle"),472,13,24,24,Theme->Red,Scores);
    auto* Train=P.Group(TEXT("TrainingLabel"),578,23,444,50,G);P.Rect(TEXT("TrainingBG"),0,0,444,50,Panel,Train);P.Label(TEXT("TrainingText"),TEXT("训练靶场  /  自由练习"),0,9,444,34,16,Theme->Paper,Train)->SetJustification(ETextJustify::Center);
    auto* Health=P.Group(TEXT("HealthPanel"),38,763,307,95,G);P.Rect(TEXT("HealthBG"),0,0,307,95,Panel,Health,true);P.Label(TEXT("HealthLabel"),TEXT("生命"),15,7,150,25,10,Theme->Paper,Health);P.Label(TEXT("Health"),TEXT("100"),15,23,176,56,35,Theme->Paper,Health,true);P.Label(TEXT("Team"),TEXT("蓝队"),246,38.5f,46,27,12,Theme->Paper,Health);
    P.Rect(TEXT("TeamBlue"),226,45,12,12,Theme->Blue,Health);P.Glyph(TEXT("TeamRed"),TEXT("Triangle"),226,45,13,13,Theme->Red,Health);
    FProgressBarStyle Flat;Flat.BackgroundImage=FSlateColorBrush(Theme->Panel);Flat.FillImage=FSlateColorBrush(FLinearColor::White);
    auto* Bar=P.Make<UProgressBar>(TEXT("HealthBar"));Bar->SetWidgetStyle(Flat);Bar->SetPercent(1);Bar->SetFillColorAndOpacity(Theme->Paper);P.Place(Health,Bar,15,74,277,9);
    auto* Ammo=P.Group(TEXT("AmmoPanel"),1192,763,370,95,G);P.Rect(TEXT("AmmoBG"),0,0,370,95,Panel,Ammo);P.Rect(TEXT("AmmoAccent"),8,42,3,39,Theme->Accent,Ammo);P.Icon(TEXT("GunIcon"),FPSUI::WeaponTexture(TEXT("Weapon.M4")),14,20,150,60,Theme->Paper,Ammo);
    P.Label(TEXT("GunName"),TEXT("M4"),170,22,190,28,13,Theme->Paper,Ammo,true);P.Label(TEXT("Ammo"),TEXT("30"),170,41,52,52,35,Theme->Paper,Ammo,true)->SetAutoWrapText(false);P.Label(TEXT("Reserve"),TEXT("/120"),222,58,65,29,17,Theme->Muted,Ammo)->SetAutoWrapText(false);
    P.Rect(TEXT("ReloadDivider"),289,50,1,34,RGB(132,141,143,85),Ammo);P.Rect(TEXT("RKeycap"),305,60,16,16,Theme->Paper,Ammo);P.Label(TEXT("KeyR"),TEXT("R"),306.25f,56,16,25,11,Theme->Ink,Ammo,true)->SetJustification(ETextJustify::Center);P.Label(TEXT("GunAction"),TEXT("换弹"),327,58,35,25,10,Theme->Paper,Ammo)->SetAutoWrapText(false);
    P.Label(TEXT("GunStatus"),TEXT(""),299,58,57,25,10,Theme->Paper,Ammo)->SetJustification(ETextJustify::Center);
    auto* Reload=P.Make<UProgressBar>(TEXT("ReloadBar"));Reload->SetWidgetStyle(Flat);Reload->SetFillColorAndOpacity(Theme->Accent);Reload->SetPercent(0);P.Place(Ammo,Reload,15,92,340,3);
    P.Label(TEXT("BaseHint"),TEXT("P   配装  /  基地内"),53,732,350,26,13,Theme->Accent,G);
    auto* Kill=P.Group(TEXT("KillPanel"),1396,106,177,40,G);P.Rect(TEXT("KillBG"),0,0,177,40,Panel,Kill);P.Rect(TEXT("KillAccent"),0,0,5,40,Theme->Accent,Kill);P.Label(TEXT("KillText"),TEXT("击倒确认"),24,7,100,29,14,Theme->Paper,Kill,true);P.Label(TEXT("KillValue"),TEXT("+1"),134,6,36,29,17,Theme->Accent,Kill,true);
    auto* Death=P.Group(TEXT("DeathPanel"),575,392,450,162,G);P.Rect(TEXT("DeathBG"),0,0,450,162,Panel,Death,true);P.Rect(TEXT("DeathAccent"),0,0,5,162,Theme->Red,Death);P.Label(TEXT("DeathTitle"),TEXT("已被击倒"),25,18,405,55,32,Theme->Paper,Death,true);P.Label(TEXT("Killer"),TEXT(""),27,82,400,30,18,Theme->Muted,Death);P.Label(TEXT("Respawn"),TEXT(""),27,119,400,32,19,Theme->Accent,Death);
    auto* Pickup=P.Group(TEXT("PickupPanel"),650,518,300,44,G);P.Rect(TEXT("PickupBG"),0,0,300,44,Panel,Pickup);
    auto* PickupCenter=P.Make<UOverlay>(TEXT("PickupCenter"));P.Place(Pickup,PickupCenter,0,0,300,44);
    auto* PickupRow=P.Make<UHorizontalBox>(TEXT("PickupRow"));auto* PickupRowSlot=PickupCenter->AddChildToOverlay(PickupRow);PickupRowSlot->SetHorizontalAlignment(HAlign_Center);PickupRowSlot->SetVerticalAlignment(VAlign_Center);
    auto* PickupKey=P.Make<USizeBox>(TEXT("PickupKey"));PickupKey->SetWidthOverride(24);PickupKey->SetHeightOverride(24);PickupRow->AddChildToHorizontalBox(PickupKey)->SetVerticalAlignment(VAlign_Center);
    auto* PickupKeyCanvas=P.Make<UCanvasPanel>(TEXT("PickupKeyCanvas"));PickupKey->AddChild(PickupKeyCanvas);P.RoundFrame(TEXT("PickupKeyFrame"),0,0,24,24,2,Theme->Accent,PickupKeyCanvas,1);
    P.Label(TEXT("PickupKeyF"),TEXT("F"),1,-1.5f,24,26,13,Theme->Accent,PickupKeyCanvas)->SetJustification(ETextJustify::Center);
    auto* PickupText=P.Make<UTextBlock>(TEXT("PickupText"));PickupText->SetText(FText::FromString(TEXT("拾取 M4")));PickupText->SetFont(FSlateFontInfo(TextFont,15,TEXT("Regular")));PickupText->SetColorAndOpacity(Theme->Accent);PickupText->SetAutoWrapText(false);PickupText->SetRenderTranslation(FVector2D(0,-1.5f));auto* PickupTextSlot=PickupRow->AddChildToHorizontalBox(PickupText);PickupTextSlot->SetPadding(FMargin(10,0,0,0));PickupTextSlot->SetVerticalAlignment(VAlign_Center);
    auto* Drop=P.Group(TEXT("DropPanel"),1172,706,390,40,G);P.Rect(TEXT("DropBG"),0,0,390,40,Panel,Drop);P.Label(TEXT("DropText"),TEXT("掉落已达上限，最早的枪已回收"),15,9,370,29,16,Theme->Accent,Drop);
}
}
#endif

FString UFPSUIAssetBuilder::BuildAssets()
{
#if WITH_EDITOR
    Root=FPSUI::AssetRoot();
    FString Variant;FParse::Value(FCommandLine::Get(),TEXT("FPSUIFontVariant="),Variant);
    const bool FontPreview=Root!=BaseRoot;
    FontSourceDirectory=FPaths::ProjectDir()/(FontPreview?TEXT("ArtSource/FPSDemo/Samples/UI/FontComparisons/Fonts"):TEXT("ArtSource/FPSDemo/Industrial/UI/Fonts"));
    const FString Path=Root+TEXT("DA_FPSUITheme");Theme=LoadObject<UFPSUITheme>(nullptr,*(Path+TEXT(".DA_FPSUITheme")));
    if(!Theme){Theme=NewObject<UFPSUITheme>(CreatePackage(*Path),TEXT("DA_FPSUITheme"),RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(Theme);}
    if(FontPreview)
    {
        const FString Prefix=Variant==TEXT("MiSans")?TEXT("MiSans-"):TEXT("HarmonyOS_Sans_SC_");
        TextFont=BuildFont(TEXT("F_UI_Text"),{{TEXT("Regular"),Prefix+TEXT("Regular.ttf")},{TEXT("Bold"),Prefix+TEXT("Bold.ttf")}});
        NumberFont=LoadObject<UFont>(nullptr,*(BaseRoot+TEXT("Fonts/F_UI_Number.F_UI_Number")));
    }
    else
    {
        TextFont=BuildFont(TEXT("F_UI_Text"),{{TEXT("Regular"),TEXT("MiSans-Regular.ttf")},{TEXT("Bold"),TEXT("MiSans-Bold.ttf")}});
        NumberFont=BuildFont(TEXT("F_UI_Number"),{{TEXT("Regular"),TEXT("Rajdhani-SemiBold.ttf")},{TEXT("Bold"),TEXT("Rajdhani-Bold.ttf")}});
    }
    if(!TextFont||!NumberFont)return TEXT("FAIL font assets");
    FCompositeSubFont Latin;Latin.EditorName=TEXT("Industrial Latin");Latin.CharacterRanges.Add(FInt32Range(0x20,0x7F));Latin.Typeface=NumberFont->GetInternalCompositeFont().DefaultTypeface;Latin.ScalingFactor=1.08f;
    TextFont->GetMutableInternalCompositeFont().SubTypefaces={Latin};TextFont->GetMutableInternalCompositeFont().MakeDirty();
    if(!Save(TextFont))return TEXT("FAIL composite font save");
    Theme->Background=RGB(20,25,26,250);Theme->Panel=RGB(34,41,43,246);Theme->Paper=RGB(232,234,231);Theme->Ink=RGB(17,24,27);Theme->Accent=RGB(248,203,49);Theme->Muted=RGB(158,165,165);Theme->Blue=RGB(61,155,224);Theme->Red=RGB(222,73,72);
    Theme->PanelTexture=LoadObject<UTexture2D>(nullptr,*(BaseRoot+TEXT("T_UI_Panel.T_UI_Panel")));
    auto Style=[&](FLinearColor Normal,FLinearColor Hover,FLinearColor Pressed)
    {
        FButtonStyle S;auto Brush=[&](FLinearColor C){FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;B.SetResourceObject(Texture(TEXT("Surface")));B.ImageSize=FVector2D(512,512);B.TintColor=C;return B;};
        S.Normal=Brush(Normal);S.Hovered=Brush(Hover);S.Pressed=Brush(Pressed);S.Disabled=Brush(FLinearColor(.07f,.08f,.08f));S.NormalPadding=FMargin(0);S.PressedPadding=FMargin(0);return S;
    };
    Theme->DarkButton=Style(FLinearColor::White,RGB(255,255,255),RGB(195,200,201));
    Theme->LightButton=Style(FLinearColor::White,RGB(247,247,244),RGB(227,219,178));
    for(FSlateBrush* B:{&Theme->LightButton.Normal,&Theme->LightButton.Hovered,&Theme->LightButton.Pressed})B->SetResourceObject(Texture(TEXT("Paper")));
    Theme->AccentButton=Style(Theme->Accent,RGB(255,220,88),RGB(225,178,32));
    for(FSlateBrush* B:{&Theme->AccentButton.Normal,&Theme->AccentButton.Hovered,&Theme->AccentButton.Pressed})B->SetResourceObject(nullptr);
    Theme->AccentButton.Disabled=FSlateColorBrush(RGB(137,146,146));Theme->LightButton.Disabled=FSlateColorBrush(RGB(137,146,146));
    if(!Save(Theme))return TEXT("FAIL theme save");
    for(const TCHAR* Name:{TEXT("Lobby"),TEXT("Loadout"),TEXT("Waiting"),TEXT("Pause"),TEXT("Report"),TEXT("HUD")})
    {FPage P(Name);if(FString(Name)==TEXT("Lobby"))Lobby(P);else if(FString(Name)==TEXT("Loadout"))Loadout(P);else if(FString(Name)==TEXT("HUD"))HUD(P);else MatchPage(P,Name);
        if(FontPreview)P.Label(TEXT("FontPreviewCredit"),TEXT("字体预览 / ")+Variant,1120,875,450,20,10,Theme->Muted)->SetJustification(ETextJustify::Right);
        else P.AlignMiSans(Name);
        if(!P.Finish(Name))return FString(TEXT("FAIL "))+Name;}
    if(!FontPreview)
    {
        const FString MovementResult=ExtendMovementAssets();
        if(!MovementResult.StartsWith(TEXT("PASS")))return MovementResult;
    }
    return TEXT("PASS six WidgetBlueprint assets and shared theme saved");
#else
    return TEXT("Editor only");
#endif
}

FString UFPSUIAssetBuilder::ExtendMovementAssets()
{
#if WITH_EDITOR
    // This path edits saved trees in place. It never calls the rebuilding FPage constructor.
    Root=BaseRoot;
    TextFont=LoadObject<UFont>(nullptr,*(Root+TEXT("Fonts/F_UI_Text.F_UI_Text")));
    NumberFont=LoadObject<UFont>(nullptr,*(Root+TEXT("Fonts/F_UI_Number.F_UI_Number")));
    if(!TextFont||!NumberFont)return TEXT("FAIL movement UI existing fonts missing");
    struct FExistingPage { FString Name; UWidgetBlueprint* BP; UFPSUITheme* Palette; bool Extended; };
    TArray<FExistingPage> Pages;
    for(const TCHAR* Name:{TEXT("Lobby"),TEXT("Waiting"),TEXT("Pause"),TEXT("Report"),TEXT("HUD")})
    {
        const FString AssetName=FString(TEXT("WBP_FPS"))+Name;
        auto* BP=LoadObject<UWidgetBlueprint>(nullptr,*(Root+TEXT("Widgets/")+AssetName+TEXT(".")+AssetName));
        if(!BP||!BP->WidgetTree||!BP->GeneratedClass||!BP->ParentClass||!BP->ParentClass->IsChildOf(UFPSInterface::StaticClass()))
            return FString(TEXT("FAIL movement UI unexpected existing asset "))+Name;
        auto* Defaults=Cast<UFPSInterface>(BP->GeneratedClass->GetDefaultObject());
        if(!Defaults||!Defaults->Theme||Defaults->Screen!=FName(Name))return FString(TEXT("FAIL movement UI existing identity/theme "))+Name;
        if(!Cast<UCanvasPanel>(BP->WidgetTree->FindWidget(TEXT("Layout"))))return FString(TEXT("FAIL movement UI existing Layout "))+Name;
        const bool HUDPage=FString(Name)==TEXT("HUD");
        TArray<TPair<FName,UClass*>> Expected;
        if(HUDPage)
        {
            if(!Cast<UCanvasPanel>(BP->WidgetTree->FindWidget(TEXT("HUDCanvas")))||!BP->WidgetTree->FindWidget(TEXT("HealthPanel")))
                return TEXT("FAIL movement UI existing HUD/health layout");
            Expected={{TEXT("StaminaPanel"),UCanvasPanel::StaticClass()},{TEXT("StaminaBG"),UImage::StaticClass()},
                {TEXT("StaminaLabel"),UTextBlock::StaticClass()},{TEXT("StaminaValue"),UTextBlock::StaticClass()},
                {TEXT("StaminaStatus"),UTextBlock::StaticClass()},{TEXT("StaminaBar"),UProgressBar::StaticClass()}};
        }
        else
        {
            Expected={{TEXT("InputSettings"),UFPSActionButton::StaticClass()},{TEXT("InputSettingsContent"),UCanvasPanel::StaticClass()},
                {TEXT("InputSettingsLabel"),UTextBlock::StaticClass()},{TEXT("InputSettingsPanel"),UCanvasPanel::StaticClass()},
                {TEXT("InputSettingsBG"),UImage::StaticClass()},{TEXT("InputSettingsAccent"),UImage::StaticClass()},
                {TEXT("InputSettingsTitle"),UTextBlock::StaticClass()},{TEXT("SprintInputTitle"),UTextBlock::StaticClass()},
                {TEXT("CrouchInputTitle"),UTextBlock::StaticClass()},{TEXT("SprintMode"),UFPSActionButton::StaticClass()},
                {TEXT("SprintModeContent"),UCanvasPanel::StaticClass()},{TEXT("SprintModeLabel"),UTextBlock::StaticClass()},
                {TEXT("CrouchMode"),UFPSActionButton::StaticClass()},{TEXT("CrouchModeContent"),UCanvasPanel::StaticClass()},
                {TEXT("CrouchModeLabel"),UTextBlock::StaticClass()},{TEXT("InputSettingsNote"),UTextBlock::StaticClass()}};
        }
        int32 Present=0;
        for(const auto& Entry:Expected)if(auto* Widget=BP->WidgetTree->FindWidget(Entry.Key))
        {
            if(!Widget->IsA(Entry.Value))return FString(TEXT("FAIL movement UI name/type collision "))+Name+TEXT("/")+Entry.Key.ToString();
            if(auto* Button=Cast<UFPSActionButton>(Widget);Button && Button->Action!=Entry.Key)
                return FString(TEXT("FAIL movement UI existing action changed "))+Name+TEXT("/")+Entry.Key.ToString();
            ++Present;
        }
        if(Present>0&&Present!=Expected.Num())return FString(TEXT("FAIL movement UI partial extension preserved for review "))+Name;
        Pages.Add({Name,BP,Defaults->Theme,Present==Expected.Num()});
    }
    int32 Saved=0,Unchanged=0,Preserved=0;
    for(const auto& Page:Pages)
    {
        if(Page.Extended)
        {
            if(Page.Name==TEXT("HUD")&&ApplyMinimalStamina(Page.BP))
            {
                FKismetEditorUtilities::CompileBlueprint(Page.BP);
                if(Page.BP->Status==BS_Error||!Page.BP->GeneratedClass)return TEXT("FAIL minimal stamina UI compile");
                auto* Defaults=CastChecked<UFPSInterface>(Page.BP->GeneratedClass->GetDefaultObject());
                Defaults->Screen=FName(*Page.Name);Defaults->Theme=Page.Palette;
                if(!Save(Page.BP))return TEXT("FAIL minimal stamina UI save");
                ++Saved;
            }
            else ++Unchanged;
            continue;
        }
        Theme=Page.Palette;
        FPage P(Page.BP);
        struct FWidgetIdentity { UWidget* Widget; UPanelWidget* Parent; UPanelSlot* Slot; };
        TMap<FName,FWidgetIdentity> Before;
        P.Tree->ForEachWidget([&](UWidget* W){Before.Add(W->GetFName(),{W,W->GetParent(),W->Slot});});
        Page.BP->Modify();P.Tree->Modify();P.Canvas->Modify();
        if(Page.Name==TEXT("HUD"))
        {
            auto* HUDCanvas=CastChecked<UCanvasPanel>(P.Tree->FindWidget(TEXT("HUDCanvas")));
            auto* Stamina=P.Group(TEXT("StaminaPanel"),361,763,315,95,HUDCanvas);
            P.Rect(TEXT("StaminaBG"),0,0,315,95,FLinearColor(.009f,.014f,.017f,.85f),Stamina,true);
            P.Label(TEXT("StaminaLabel"),TEXT("体力"),15,7,130,25,10,Theme->Paper,Stamina);
            auto* Value=P.Label(TEXT("StaminaValue"),TEXT("100"),15,26,75,45,28,Theme->Paper,Stamina,true);
            Value->SetFont(FSlateFontInfo(NumberFont,28,TEXT("Regular")));Value->SetAutoWrapText(false);
            P.Label(TEXT("StaminaStatus"),TEXT(""),95,39,205,27,14,Theme->Paper,Stamina)->SetAutoWrapText(false);
            FProgressBarStyle Flat;Flat.BackgroundImage=FSlateColorBrush(Theme->Panel);Flat.FillImage=FSlateColorBrush(FLinearColor::White);
            auto* Bar=P.Make<UProgressBar>(TEXT("StaminaBar"));Bar->SetWidgetStyle(Flat);Bar->SetPercent(1);Bar->SetFillColorAndOpacity(Theme->Paper);
            P.Place(Stamina,Bar,15,74,285,9);Stamina->SetVisibility(ESlateVisibility::Collapsed);
            ApplyMinimalStamina(Page.BP);
        }
        else
        {
            const bool LobbyPage=Page.Name==TEXT("Lobby");
            P.Button(TEXT("InputSettings"),TEXT("操作设置"),TEXT("InputSettings"),LobbyPage?51:605,LobbyPage?670:748,LobbyPage?305:280,LobbyPage?48:65);
            auto* Settings=P.Group(TEXT("InputSettingsPanel"),409,316,1148,380);
            P.Rect(TEXT("InputSettingsBG"),0,0,1148,380,Theme->Ink,Settings)->SetVisibility(ESlateVisibility::Visible);
            P.Rect(TEXT("InputSettingsAccent"),0,0,5,380,Theme->Accent,Settings);
            P.Label(TEXT("InputSettingsTitle"),TEXT("操作设置"),30,21,1070,50,28,Theme->Paper,Settings,true);
            P.Label(TEXT("SprintInputTitle"),TEXT("疾跑  /  Shift"),30,97,360,43,22,Theme->Paper,Settings,true);
            P.Button(TEXT("SprintMode"),TEXT("按住：松开停止"),TEXT("SprintMode"),430,89,675,65,0,Settings);
            P.Label(TEXT("CrouchInputTitle"),TEXT("蹲伏  /  Ctrl"),30,186,360,43,22,Theme->Paper,Settings,true);
            P.Button(TEXT("CrouchMode"),TEXT("按住：松开站起"),TEXT("CrouchMode"),430,178,675,65,0,Settings);
            P.Label(TEXT("InputSettingsNote"),TEXT("更改后自动保存到本机。疾跑中断后须重新触发，菜单不会恢复旧动作。"),30,282,1070,70,18,Theme->Muted,Settings);
            Settings->SetVisibility(ESlateVisibility::Collapsed);
        }
        P.Tree->ForEachWidget([&](UWidget* W){if(!Page.BP->WidgetVariableNameToGuidMap.Contains(W->GetFName()))Page.BP->OnVariableAdded(W->GetFName());});
        FKismetEditorUtilities::CompileBlueprint(Page.BP);
        if(Page.BP->Status==BS_Error||!Page.BP->GeneratedClass)return TEXT("FAIL movement UI compile ")+Page.Name;
        for(const auto& Pair:Before)
        {
            auto* W=P.Tree->FindWidget(Pair.Key);
            if(W!=Pair.Value.Widget||W->GetParent()!=Pair.Value.Parent||W->Slot!=Pair.Value.Slot)
                return TEXT("FAIL movement UI existing tree identity changed ")+Page.Name+TEXT("/")+Pair.Key.ToString();
        }
        auto* Defaults=CastChecked<UFPSInterface>(Page.BP->GeneratedClass->GetDefaultObject());
        Defaults->Screen=FName(*Page.Name);Defaults->Theme=Page.Palette;
        if(!Save(Page.BP))return TEXT("FAIL movement UI save ")+Page.Name;
        Preserved+=Before.Num();++Saved;
        UE_LOG(LogTemp,Display,TEXT("FPS_MOVEMENT_UI PASS asset=%s existing_widgets=%d"),*Page.BP->GetPathName(),Before.Num());
    }
    return FString::Printf(TEXT("PASS movement UI saved=%d unchanged=%d existing_widgets_preserved=%d"),Saved,Unchanged,Preserved);
#else
    return TEXT("Editor only");
#endif
}

FString UFPSUIAssetBuilder::BuildStartMenuAssets()
{
#if WITH_EDITOR
    Root=BaseRoot;
    Theme=LoadObject<UFPSUITheme>(nullptr,*(Root+TEXT("DA_FPSUITheme.DA_FPSUITheme")));
    TextFont=LoadObject<UFont>(nullptr,*(Root+TEXT("Fonts/F_UI_Text.F_UI_Text")));
    NumberFont=LoadObject<UFont>(nullptr,*(Root+TEXT("Fonts/F_UI_Number.F_UI_Number")));
    if(!Theme||!TextFont||!NumberFont)return TEXT("FAIL missing shared UI theme/fonts");
    // Create once. Subsequent upgrades preserve Designer edits in the start screen.
    const FString StartRoot=TEXT("/Game/FPSDemo/Art/UI/StartMenu/");
    if(!LoadObject<UWidgetBlueprint>(nullptr,*(StartRoot+TEXT("WBP_FPSStart.WBP_FPSStart"))))
    {
        FPage P(TEXT("Start"),StartRoot);
        P.Backdrop();
        P.Label(TEXT("Brand"),TEXT("FOUNDRY"),66,62,550,50,29,Theme->Muted);
        P.Label(TEXT("Title"),TEXT("选择模式"),64,120,900,84,48,Theme->Paper,nullptr,true);
        P.Label(TEXT("Intro"),TEXT("选择你的下一场行动。"),68,208,1000,42,19,Theme->Muted);
        for(int32 I=0;I<2;++I)
        {
            const FString Name=I==0?TEXT("Multiplayer"):TEXT("Recovery");
            auto* B=P.Button(Name,TEXT(""),FName(*Name),64+I*752,280,720,480);
            auto* C=CastChecked<UCanvasPanel>(B->GetContent());
            FButtonStyle Style=Theme->DarkButton;
            Style.Normal=FSlateColorBrush(RGB(25,32,35));
            Style.Hovered=FSlateColorBrush(RGB(49,58,60));
            Style.Pressed=FSlateColorBrush(RGB(64,66,53));B->SetStyle(Style);
            auto* Picture=P.Rect(Name+TEXT("Image"),2,2,716,260,FLinearColor::White,C);
            auto* Preview=I==0?Texture(TEXT("Foundry")):LoadObject<UTexture2D>(nullptr,TEXT("/Game/FPSDemo/Art/UI/ExtractionTerminal/Textures/T_LobbyIndustrialCrossing.T_LobbyIndustrialCrossing"));
            if(!Preview)return TEXT("FAIL missing mode preview");
            Picture->SetBrushFromTexture(Preview);
            P.Rect(Name+TEXT("ImageTint"),2,2,716,260,FLinearColor(0,0,0,.14f),C);
            P.Rect(Name+TEXT("NumberBG"),24,24,66,45,Theme->Ink,C);
            auto* Number=P.Label(Name+TEXT("Number"),I==0?TEXT("01"):TEXT("02"),38,21,50,45,28,Theme->Accent,C);
            Number->SetFont(FSlateFontInfo(NumberFont,28,TEXT("Bold")));
            P.Rect(Name+TEXT("Accent"),28,290,4,45,Theme->Accent,C);
            P.Label(Name+TEXT("Title"),I==0?TEXT("多人对战"):TEXT("回收行动"),50,280,590,64,34,Theme->Paper,C,true);
            P.Label(Name+TEXT("Description"),I==0?TEXT("创建或加入房间，与其他玩家交战。\n也可进入单机训练与靶场。 "):TEXT("在个人工厂整备，进入战区搜寻物资。\n成功撤离，把收获带回仓库。"),30,353,650,77,18,Theme->Muted,C);
            P.Label(Name+TEXT("Enter"),I==0?TEXT("进入对战  →"):TEXT("进入工厂  →"),30,433,650,36,18,Theme->Accent,C,true);
            P.Frame(Name+TEXT("Border"),0,0,720,480,RGB(105,118,117,130),C);
        }
        P.Label(TEXT("Error"),TEXT(""),66,766,1468,46,16,Theme->Accent);
        P.Button(TEXT("InputSettings"),TEXT("设置"),TEXT("InputSettings"),64,818,172,54);
        P.Button(TEXT("Quit"),TEXT("退出游戏"),TEXT("Quit"),252,818,206,54);
        if(!P.Finish(TEXT("Start")))return TEXT("FAIL start widget save");
    }
    auto* Lobby=LoadObject<UWidgetBlueprint>(nullptr,*(BaseRoot+TEXT("Widgets/WBP_FPSLobby.WBP_FPSLobby")));
    if(!Lobby)return TEXT("FAIL missing multiplayer lobby");
    FPage L(Lobby);if(!L.Canvas)return TEXT("FAIL missing lobby canvas");
    if(!L.Tree->FindWidget(TEXT("Modes")))L.Button(TEXT("Modes"),TEXT("←  模式选择"),TEXT("Modes"),51,565,305,54);
    if(auto* T=Cast<UTextBlock>(L.Tree->FindWidget(TEXT("Title"))))T->SetText(FText::FromString(TEXT("多人对战")));
    if(!L.Finish(TEXT("Lobby")))return TEXT("FAIL multiplayer widget save");

    auto* Terminal=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal.WBP_ExtractionTerminal"));
    if(!Terminal)return TEXT("FAIL missing factory terminal");
    FPage E(Terminal);auto* Home=Cast<UCanvasPanel>(E.Tree->FindWidget(TEXT("OverviewPanel")));
    if(!Home)return TEXT("FAIL missing factory lobby");
    E.Tree->ForEachWidget([](UWidget* W)
    {
        if(auto* T=Cast<UTextBlock>(W))
        {
            FString S=T->GetText().ToString();
            S.ReplaceInline(TEXT("出征前"),TEXT("行动前"));S.ReplaceInline(TEXT("出征背包"),TEXT("行动背包"));
            S.ReplaceInline(TEXT("出征装备"),TEXT("行动装备"));S.ReplaceInline(TEXT("出征枪械"),TEXT("携带枪械"));
            S.ReplaceInline(TEXT("出征整备"),TEXT("行动整备"));S.ReplaceInline(TEXT("返回基地"),TEXT("返回工厂"));
            S.ReplaceInline(TEXT("出征  →"),TEXT("开始行动  →"));S.ReplaceInline(TEXT("出发  →"),TEXT("开始行动  →"));
            if(S==TEXT("出征"))S=TEXT("行动");
            if(T->GetFName()==TEXT("LobbyPrepareLabel"))S=TEXT("行动整备");
            T->SetText(FText::FromString(S));
        }
    });
    if(auto* B=E.Tree->FindWidget(TEXT("LobbyPrepare")))if(auto* Slot=Cast<UCanvasPanelSlot>(B->Slot))
    {Slot->SetPosition(FVector2D(1144,826));Slot->SetSize(FVector2D(180,52));}
    if(!E.Tree->FindWidget(TEXT("LobbyModes")))
    {
        auto* B=E.Make<UFPSExtractionButton>(TEXT("LobbyModes"));B->Action=TEXT("Modes");B->SetStyle(Theme->DarkButton);
        auto* T=E.Make<UTextBlock>(TEXT("LobbyModesLabel"));T->SetText(FText::FromString(TEXT("模式选择")));
        T->SetFont(FSlateFontInfo(TextFont,18,TEXT("Regular")));T->SetColorAndOpacity(Theme->Paper);T->SetVisibility(ESlateVisibility::HitTestInvisible);
        B->AddChild(T);auto* Slot=CastChecked<UButtonSlot>(T->Slot);Slot->SetHorizontalAlignment(HAlign_Center);Slot->SetVerticalAlignment(VAlign_Center);
        E.Place(Home,B,856,44,200,54);
    }
    E.Tree->ForEachWidget([&](UWidget* W){if(!Terminal->WidgetVariableNameToGuidMap.Contains(W->GetFName()))Terminal->OnVariableAdded(W->GetFName());});
    FKismetEditorUtilities::CompileBlueprint(Terminal);
    if(Terminal->Status==BS_Error||!Save(Terminal))return TEXT("FAIL factory terminal save");
    return TEXT("PASS start menu, multiplayer back entry and factory wording saved");
#else
    return TEXT("Editor only");
#endif
}
