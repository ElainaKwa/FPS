#include "FPSSystemSettings.h"
#include "FPSInputSettings.h"
#include "FPSAudioSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/IConsoleManager.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

namespace FPSSettings
{
const int32 Limits[]={30,60,90,120,144,165,240,0};
const TCHAR* QualityIds[]={TEXT("ViewDistance"),TEXT("AntiAliasing"),TEXT("Shadows"),TEXT("GlobalIllumination"),TEXT("Reflections"),TEXT("Textures"),TEXT("Effects"),TEXT("Foliage")};
const TCHAR* QualityNames[]={TEXT("视距"),TEXT("抗锯齿"),TEXT("阴影"),TEXT("全局光照"),TEXT("反射"),TEXT("纹理"),TEXT("特效"),TEXT("植被")};
TArray<FFPSKeyBinding> DefaultBindings()
{
    return {{"Forward",TEXT("前进"),EKeys::W,{}},{"Backward",TEXT("后退"),EKeys::S,{}},{"Left",TEXT("向左"),EKeys::A,{}},{"Right",TEXT("向右"),EKeys::D,{}},
    {"Jump",TEXT("跳跃"),EKeys::SpaceBar,{}},{"Crouch",TEXT("蹲伏"),EKeys::LeftControl,{}},{"Sprint",TEXT("疾跑"),EKeys::LeftShift,{}},
    {"Fire",TEXT("射击"),EKeys::LeftMouseButton,{}},{"Aim",TEXT("瞄准"),EKeys::RightMouseButton,{}},{"Reload",TEXT("换弹"),EKeys::R,{}},
    {"Inspect",TEXT("检视"),EKeys::Q,{}},{"Primary",TEXT("主武器"),EKeys::One,{}},{"Secondary",TEXT("副武器"),EKeys::Two,{}},
    {"NextWeapon",TEXT("切换武器"),EKeys::MouseScrollUp,EKeys::MouseScrollDown},{"Drop",TEXT("丢弃武器"),EKeys::G,{}},
    {"Interact",TEXT("交互 / 拾取"),EKeys::F,{}},{"Loadout",TEXT("配装"),EKeys::P,{}},{"Customization",TEXT("靶场改装"),EKeys::B,{}},{"Inventory",TEXT("出征背包"),EKeys::Tab,{}}};
}
bool ValidKey(FKey Key){return !Key.IsValid() || (!Key.IsGamepadKey() && !Key.IsAxis1D() && !Key.IsAxis2D() && !Key.IsAxis3D() && Key!=EKeys::Escape && Key!=EKeys::LeftCommand && Key!=EKeys::RightCommand);}
EWindowMode::Type WindowMode(int32 Mode){return Mode==0?EWindowMode::Windowed:Mode==1?EWindowMode::WindowedFullscreen:EWindowMode::Fullscreen;}
FIntPoint Desktop(){return GEngine->GetGameUserSettings()->GetDesktopResolution();}
}
bool FFPSGraphicsSettings::operator==(const FFPSGraphicsSettings& O) const
{return Mode==O.Mode&&Resolution==O.Resolution&&FrameLimit==O.FrameLimit&&VSync==O.VSync&&MotionBlur==O.MotionBlur&&FOV==O.FOV&&Quality==O.Quality;}
UFPSSystemSettings* UFPSSystemSettings::Get(const UObject* Context)
{return Context&&Context->GetWorld()&&Context->GetWorld()->GetGameInstance()?Context->GetWorld()->GetGameInstance()->GetSubsystem<UFPSSystemSettings>():nullptr;}
void UFPSSystemSettings::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);if(IsRunningCommandlet()||IsRunningDedicatedServer())return;
    KeyBindings=FPSSettings::DefaultBindings();
    UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
    const auto Desktop=FPSSettings::Desktop();Resolutions.AddUnique(Desktop);Resolutions.Sort([](const auto& A,const auto& B){return A.X==B.X?A.Y<B.Y:A.X<B.X;});
    Graphics.Resolution=Desktop;
    if(auto* G=GEngine->GetGameUserSettings())Graphics.Quality={G->GetViewDistanceQuality(),G->GetAntiAliasingQuality(),G->GetShadowQuality(),G->GetGlobalIlluminationQuality(),G->GetReflectionQuality(),G->GetTextureQuality(),G->GetVisualEffectQuality(),G->GetFoliageQuality()};
    for(auto& Q:Graphics.Quality)Q=FMath::Clamp(Q,0,3);Defaults=Graphics;
    Load();Draft=Graphics;ApplyImmediate();bInitialized=true;
}
void UFPSSystemSettings::Deinitialize(){if(IsConfirmingDisplay())RevertDisplay();bInitialized=false;Super::Deinitialize();}
TStatId UFPSSystemSettings::GetStatId() const {RETURN_QUICK_DECLARE_CYCLE_STAT(FPSSystemSettings,STATGROUP_Tickables);}
void UFPSSystemSettings::Tick(float)
{
    if(DisplayDeadline>0&&FPlatformTime::Seconds()>=DisplayDeadline){RevertDisplay();Feedback=TEXT("显示确认超时，已恢复此前显示设置。");}
    // Apply after the viewport exists. Test profiles preserve the capture window's requested size.
    if(!bRuntimeApplied&&GetWorld()&&GetWorld()->GetFirstPlayerController())
    {bRuntimeApplied=true;FString Profile;const bool Test=FParse::Value(FCommandLine::Get(),TEXT("FPSSettingsProfile="),Profile);ApplyRuntimeGraphics(Graphics,!Test);}
}
FString UFPSSystemSettings::StoragePath() const
{
#if !UE_BUILD_SHIPPING
    FString Profile;if(FParse::Value(FCommandLine::Get(),TEXT("FPSSettingsProfile="),Profile))
    {for(TCHAR C:Profile)if(!FChar::IsAlnum(C)&&C!='_'&&C!='-')return FPaths::ProjectSavedDir()/TEXT("SettingsTests/InvalidProfile.json");return FPaths::ProjectSavedDir()/TEXT("SettingsTests/Profiles")/(Profile+TEXT(".json"));}
#endif
    return FString(FPlatformProcess::UserSettingsDir())/TEXT("MCP_Test/SystemSettings.json");
}
void UFPSSystemSettings::Load()
{
    FString Text;if(!FFileHelper::LoadFileToString(Text,*StoragePath()))return;
    TSharedPtr<FJsonObject> J;const auto Reader=TJsonReaderFactory<>::Create(Text);
    if(!FJsonSerializer::Deserialize(Reader,J)||!J){Feedback=TEXT("设置文件无法读取，当前使用默认值。");return;}
    auto Number=[&](const TCHAR* Key,double Default,double Min,double Max){double V;return J->TryGetNumberField(Key,V)&&FMath::IsFinite(V)&&V>=Min&&V<=Max?V:Default;};
    auto Bool=[&](const TCHAR* Key,bool Default){bool V;return J->TryGetBoolField(Key,V)?V:Default;};
    Sensitivity=Number(TEXT("Sensitivity"),1,.1,3);ADSSensitivity=Number(TEXT("ADSSensitivity"),1,.1,2);
    HUDScale=Number(TEXT("HUDScale"),1,.75,1.2);BobScale=Number(TEXT("BobScale"),1,0,1);
    MasterVolume=Number(TEXT("MasterVolume"),1,0,1);GameVolume=Number(TEXT("GameVolume"),1,0,1);
    InvertY=Bool(TEXT("InvertY"),false);AimToggle=Bool(TEXT("AimToggle"),false);BackgroundMute=Bool(TEXT("BackgroundMute"),true);
    auto* Input=UFPSInputSettings::Get();Input->bSprintToggle=Bool(TEXT("SprintToggle"),Input->bSprintToggle);Input->bCrouchToggle=Bool(TEXT("CrouchToggle"),Input->bCrouchToggle);
    Graphics.Mode=Number(TEXT("DisplayMode"),1,0,2);Graphics.Resolution=FIntPoint(Number(TEXT("Width"),Graphics.Resolution.X,640,16384),Number(TEXT("Height"),Graphics.Resolution.Y,480,16384));
    if(Graphics.Mode==1||!Resolutions.Contains(Graphics.Resolution))Graphics.Resolution=FPSSettings::Desktop();
    Graphics.FrameLimit=Number(TEXT("FrameLimit"),120,0,240);if(!MakeArrayView(FPSSettings::Limits).Contains(Graphics.FrameLimit))Graphics.FrameLimit=120;
    Graphics.VSync=Bool(TEXT("VSync"),false);Graphics.MotionBlur=Bool(TEXT("MotionBlur"),false);Graphics.FOV=Number(TEXT("FOV"),92,85,100);
    for(int32 I=0;I<8;++I)Graphics.Quality[I]=Number(FPSSettings::QualityIds[I],Graphics.Quality[I],0,3);
    const TSharedPtr<FJsonObject>* Bindings=nullptr;
    if(J->TryGetObjectField(TEXT("Bindings"),Bindings))for(auto& B:KeyBindings)for(int32 Slot=0;Slot<2;++Slot)
    {FString Key;if((*Bindings)->TryGetStringField(B.Id.ToString()+FString::FromInt(Slot),Key))
    {const FKey K(Key.IsEmpty()?NAME_None:FName(*Key));if((Key.IsEmpty()||K.IsValid())&&FPSSettings::ValidKey(K))(Slot?B.Secondary:B.Primary)=K;}}
    // Damaged duplicate assignments recover only the later conflicting slot.
    TSet<FKey> Seen;for(auto& B:KeyBindings)for(FKey* K:{&B.Primary,&B.Secondary})if(K->IsValid()){if(Seen.Contains(*K))*K=FKey();else Seen.Add(*K);}
}
bool UFPSSystemSettings::Save()
{
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("Version"),1);
    for(const auto& Pair:TArray<TPair<FString,double>>{{"Sensitivity",Sensitivity},{"ADSSensitivity",ADSSensitivity},{"HUDScale",HUDScale},{"BobScale",BobScale},{"MasterVolume",MasterVolume},{"GameVolume",GameVolume}})J->SetNumberField(Pair.Key,Pair.Value);
    J->SetBoolField(TEXT("InvertY"),InvertY);J->SetBoolField(TEXT("AimToggle"),AimToggle);J->SetBoolField(TEXT("BackgroundMute"),BackgroundMute);
    J->SetBoolField(TEXT("SprintToggle"),UFPSInputSettings::Get()->bSprintToggle);J->SetBoolField(TEXT("CrouchToggle"),UFPSInputSettings::Get()->bCrouchToggle);
    J->SetNumberField(TEXT("DisplayMode"),IsConfirmingDisplay()?PreviousDisplay.Mode:Graphics.Mode);
    const auto Size=IsConfirmingDisplay()?PreviousDisplay.Resolution:Graphics.Resolution;J->SetNumberField(TEXT("Width"),Size.X);J->SetNumberField(TEXT("Height"),Size.Y);
    J->SetNumberField(TEXT("FrameLimit"),Graphics.FrameLimit);J->SetNumberField(TEXT("FOV"),Graphics.FOV);J->SetBoolField(TEXT("VSync"),Graphics.VSync);J->SetBoolField(TEXT("MotionBlur"),Graphics.MotionBlur);
    for(int32 I=0;I<8;++I)J->SetNumberField(FPSSettings::QualityIds[I],Graphics.Quality[I]);
    auto Bind=MakeShared<FJsonObject>();for(const auto& B:KeyBindings){Bind->SetStringField(B.Id.ToString()+TEXT("0"),B.Primary.IsValid()?B.Primary.ToString():TEXT(""));Bind->SetStringField(B.Id.ToString()+TEXT("1"),B.Secondary.IsValid()?B.Secondary.ToString():TEXT(""));}J->SetObjectField(TEXT("Bindings"),Bind);
    FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));const FString Path=StoragePath(),Temp=Path+TEXT(".tmp");
    bSaved=!bForceSaveFailure&&IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true)&&FFileHelper::SaveStringToFile(Text,*Temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)&&IFileManager::Get().Move(*Path,*Temp,true,true);
    Feedback=bSaved?TEXT("设置已保存到本机。"):TEXT("保存失败：当前会话已生效，退出后可能丢失；请检查文件权限或磁盘空间。");return bSaved;
}
void UFPSSystemSettings::ApplyImmediate()
{
    FApp::SetUnfocusedVolumeMultiplier(BackgroundMute?0.f:1.f);
    // Enhanced input keeps the existing .07 axis response; no temporal smoothing or FOV multiplier.
    auto* Input=GetMutableDefault<UInputSettings>();Input->bEnableMouseSmoothing=false;Input->bEnableFOVScaling=false;
    if(auto* Audio=UFPSAudioSubsystem::Get(this))Audio->RefreshSettings();
}
void UFPSSystemSettings::ApplyRuntimeGraphics(const FFPSGraphicsSettings& V,bool Display)
{
    auto* G=GEngine?GEngine->GetGameUserSettings():nullptr;if(!G)return;
    G->SetFrameRateLimit(V.FrameLimit);G->SetVSyncEnabled(V.VSync);
    G->SetViewDistanceQuality(V.Quality[0]);G->SetAntiAliasingQuality(V.Quality[1]);G->SetShadowQuality(V.Quality[2]);G->SetGlobalIlluminationQuality(V.Quality[3]);G->SetReflectionQuality(V.Quality[4]);G->SetTextureQuality(V.Quality[5]);G->SetVisualEffectQuality(V.Quality[6]);G->SetFoliageQuality(V.Quality[7]);
    G->ApplyNonResolutionSettings();
    if(auto* C=IConsoleManager::Get().FindConsoleVariable(TEXT("r.MotionBlurQuality")))C->Set(V.MotionBlur?4:0,ECVF_SetByGameSetting);
    if(Display){G->SetFullscreenMode(FPSSettings::WindowMode(V.Mode));G->SetScreenResolution(V.Mode==1?FPSSettings::Desktop():V.Resolution);G->ApplyResolutionSettings(false);}
}
bool UFPSSystemSettings::ApplyGraphics()
{
    if(IsConfirmingDisplay())return false;
    if(Draft.Mode==1)Draft.Resolution=FPSSettings::Desktop();
    if(!Resolutions.Contains(Draft.Resolution)){Feedback=TEXT("该分辨率已不可用，请重新选择。");return false;}
    PreviousDisplay=Graphics;const bool Change=Draft.Mode!=Graphics.Mode||Draft.Resolution!=Graphics.Resolution;Graphics=Draft;
    if(Change)DisplayDeadline=FPlatformTime::Seconds()+15;
    ApplyRuntimeGraphics(Graphics,Change);Save();return true;
}
void UFPSSystemSettings::DiscardGraphics(){if(IsConfirmingDisplay())RevertDisplay();Draft=Graphics;}
int32 UFPSSystemSettings::DisplaySecondsLeft() const{return DisplayDeadline>0?FMath::Max(0,FMath::CeilToInt(DisplayDeadline-FPlatformTime::Seconds())):0;}
void UFPSSystemSettings::ConfirmDisplay(){if(!IsConfirmingDisplay())return;DisplayDeadline=0;Save();}
void UFPSSystemSettings::RevertDisplay(){if(!IsConfirmingDisplay())return;DisplayDeadline=0;Graphics.Mode=PreviousDisplay.Mode;Graphics.Resolution=PreviousDisplay.Resolution;Draft=Graphics;ApplyRuntimeGraphics(Graphics,true);Save();}
const FFPSKeyBinding* UFPSSystemSettings::Binding(FName Id) const{return KeyBindings.FindByPredicate([&](const auto& B){return B.Id==Id;});}
FString UFPSSystemSettings::BindingText(FName Id) const
{const auto* B=Binding(Id);if(!B)return TEXT("未绑定");FString Text=B->Primary.IsValid()?B->Primary.GetDisplayName().ToString():TEXT("");if(B->Secondary.IsValid())Text+=(Text.IsEmpty()?TEXT(""):TEXT(" / "))+B->Secondary.GetDisplayName().ToString();return Text.IsEmpty()?TEXT("未绑定"):Text;}
void UFPSSystemSettings::BeginCapture(FName OptionId){CaptureId=OptionId;Conflicts.Empty();PendingKey=FKey();Feedback=TEXT("");}
FString UFPSSystemSettings::CapturePrompt() const{return Conflicts.IsEmpty()?TEXT("按下键盘或鼠标按键；Esc 取消。支持侧键与滚轮，不支持组合键。"):TEXT("此按键已用于：")+FString::Join(Conflicts,TEXT("、"))+TEXT("。替换后将清空原绑定。");}
void UFPSSystemSettings::CancelCapture(){CaptureId=NAME_None;Conflicts.Empty();PendingKey=FKey();}
void UFPSSystemSettings::CaptureKey(FKey Key)
{
    if(!IsCapturing())return;if(Key==EKeys::Escape){CancelCapture();return;}
    if(!Key.IsValid()||!FPSSettings::ValidKey(Key)){Feedback=TEXT("不支持该输入；请选择键盘、鼠标按键或滚轮。");return;}
    PendingKey=Key;Conflicts.Empty();
    for(const auto& B:KeyBindings)for(int32 I=0;I<2;++I)if((I?B.Secondary:B.Primary)==Key&&CaptureId!=FName(*(TEXT("Bind.")+B.Id.ToString()+TEXT(".")+FString::FromInt(I))))Conflicts.Add(B.Label+(I?TEXT("（副）"):TEXT("（主）")));
    if(Conflicts.IsEmpty())InstallBinding(false);
}
void UFPSSystemSettings::ClearCapturedBinding(){PendingKey=FKey();InstallBinding(false);}
void UFPSSystemSettings::ConfirmBindingReplacement(){if(IsCapturing()&&!Conflicts.IsEmpty())InstallBinding(true);}
void UFPSSystemSettings::InstallBinding(bool Replace)
{
    TArray<FString> Parts;CaptureId.ToString().ParseIntoArray(Parts,TEXT("."));if(Parts.Num()!=3)return;
    auto* B=KeyBindings.FindByPredicate([&](const auto& X){return X.Id==FName(*Parts[1]);});if(!B)return;
    if(Replace)for(auto& X:KeyBindings)for(FKey* Key:{&X.Primary,&X.Secondary})if(*Key==PendingKey)*Key=FKey();
    (Parts[2]==TEXT("1")?B->Secondary:B->Primary)=PendingKey;++InputRevision;CancelCapture();Save();
}
void UFPSSystemSettings::PreviewSound(){if(auto* A=UFPSAudioSubsystem::Get(this))A->Play(TEXT("Hit"));}
void UFPSSystemSettings::ResetPage(int32 Page)
{
    if(Page==0){if(!IsConfirmingDisplay())Draft=Defaults;return;}
    if(Page==1){MasterVolume=GameVolume=1;BackgroundMute=true;}
    if(Page==2){Sensitivity=ADSSensitivity=1;InvertY=AimToggle=false;UFPSInputSettings::Get()->bSprintToggle=false;UFPSInputSettings::Get()->bCrouchToggle=false;}
    if(Page==3){KeyBindings=FPSSettings::DefaultBindings();++InputRevision;}
    if(Page==4){HUDScale=BobScale=1;}
    ApplyImmediate();Save();
}
TArray<FFPSSettingOption> UFPSSystemSettings::Options(int32 Page) const
{
    TArray<FFPSSettingOption> Out;
    auto Choice=[&](FName Id,const TCHAR* Name,int32 Value,TArray<FString> Choices,const TCHAR* Help=TEXT("")){FFPSSettingOption O;O.Id=Id;O.Label=Name;O.Value=Value;O.Choices=Choices;O.Description=Help;Out.Add(O);};
    auto Toggle=[&](FName Id,const TCHAR* Name,bool Value){Choice(Id,Name,Value,{TEXT("关闭"),TEXT("开启")});};
    auto Slider=[&](FName Id,const TCHAR* Name,double Value,double Min,double Max,double Step,const TCHAR* Help=TEXT("")){FFPSSettingOption O;O.Id=Id;O.Label=Name;O.Value=Value;O.Min=Min;O.Max=Max;O.Step=Step;O.Kind=EFPSSettingKind::Slider;O.Description=Help;Out.Add(O);};
    if(Page==0)
    {
        Choice("DisplayMode",TEXT("显示模式"),Draft.Mode,{TEXT("窗口"),TEXT("无边框"),TEXT("全屏")});TArray<FString> Sizes;for(auto Size:Resolutions)Sizes.Add(FString::Printf(TEXT("%d × %d"),Size.X,Size.Y));
        Choice("Resolution",TEXT("分辨率"),Resolutions.IndexOfByKey(Draft.Resolution),Sizes,Draft.Mode==1?TEXT("无边框跟随桌面分辨率"):TEXT("设备支持的显示尺寸"));
        Choice("FrameLimit",TEXT("帧率上限"),MakeArrayView(FPSSettings::Limits).Find(Draft.FrameLimit),{TEXT("30"),TEXT("60"),TEXT("90"),TEXT("120"),TEXT("144"),TEXT("165"),TEXT("240"),TEXT("不限")});Toggle("VSync",TEXT("垂直同步"),Draft.VSync);
        int32 Preset=Draft.Quality[0];for(int32 Q:Draft.Quality)if(Q!=Draft.Quality[0])Preset=4;
        Choice("Preset",TEXT("画质预设"),Preset,{TEXT("低"),TEXT("中"),TEXT("高"),TEXT("极高"),TEXT("自定义")});
        for(int32 I=0;I<8;++I)Choice(FPSSettings::QualityIds[I],FPSSettings::QualityNames[I],Draft.Quality[I],{TEXT("低"),TEXT("中"),TEXT("高"),TEXT("极高")});
        Toggle("MotionBlur",TEXT("动态模糊"),Draft.MotionBlur);Slider("FOV",TEXT("基础视野 FOV"),Draft.FOV,85,100,1,TEXT("开镜放大仍由武器规则决定"));
    }
    if(Page==1){Slider("MasterVolume",TEXT("总音量"),MasterVolume,0,1,.01);Slider("GameVolume",TEXT("游戏音效"),GameVolume,0,1,.01,TEXT("枪声、脚步与战斗反馈共用"));Toggle("BackgroundMute",TEXT("后台静音"),BackgroundMute);FFPSSettingOption O;O.Id="PreviewSound";O.Label=TEXT("试听游戏音效");O.Kind=EFPSSettingKind::Action;Out.Add(O);}
    if(Page==2){Slider("Sensitivity",TEXT("鼠标灵敏度"),Sensitivity,.1,3,.05);Slider("ADSSensitivity",TEXT("瞄准灵敏度倍率"),ADSSensitivity,.1,2,.05);Toggle("InvertY",TEXT("垂直反转"),InvertY);
    Choice("AimToggle",TEXT("瞄准方式"),AimToggle,{TEXT("按住"),TEXT("切换")});Choice("SprintToggle",TEXT("疾跑方式"),UFPSInputSettings::Get()->bSprintToggle,{TEXT("按住"),TEXT("切换")});Choice("CrouchToggle",TEXT("蹲伏方式"),UFPSInputSettings::Get()->bCrouchToggle,{TEXT("按住"),TEXT("切换")});}
    if(Page==3)for(const auto& B:KeyBindings)for(int32 I=0;I<2;++I){FFPSSettingOption O;O.Id=FName(*(TEXT("Bind.")+B.Id.ToString()+TEXT(".")+FString::FromInt(I)));O.Label=B.Label+(I?TEXT(" · 副绑定"):TEXT(" · 主绑定"));O.Kind=EFPSSettingKind::Binding;const FKey Key=I?B.Secondary:B.Primary;O.ValueText=Key.IsValid()?Key.GetDisplayName().ToString():TEXT("未绑定");O.Description=(!B.Primary.IsValid()&&!B.Secondary.IsValid())?TEXT("此动作没有可用按键"):TEXT("");Out.Add(O);}
    if(Page==4){Slider("HUDScale",TEXT("HUD 缩放"),HUDScale,.75,1.2,.05,TEXT("只调整对局 HUD，菜单尺寸保持可读"));Slider("BobScale",TEXT("装饰性晃动"),BobScale,0,1,.05,TEXT("不改变实际射击方向或后坐力"));}
    return Out;
}
bool UFPSSystemSettings::SetValue(FName Id,double V)
{
    if(!FMath::IsFinite(V))return false;
    for(int32 Page=0;Page<5;++Page)for(const auto& O:Options(Page))if(O.Id==Id)
    {
        if(Page==0&&IsConfirmingDisplay())return false;
        if(O.Kind==EFPSSettingKind::Slider)V=FMath::Clamp(FMath::RoundToDouble(V/O.Step)*O.Step,O.Min,O.Max);
        else if(O.Kind==EFPSSettingKind::Choice){if(V<0||V>=O.Choices.Num()||V!=FMath::FloorToDouble(V))return false;}
        else if(Id=="PreviewSound"){PreviewSound();return true;}else return false;
        if(Id=="DisplayMode"){Draft.Mode=V;if(Draft.Mode==1)Draft.Resolution=FPSSettings::Desktop();}
        else if(Id=="Resolution"){if(Draft.Mode!=1)Draft.Resolution=Resolutions[int32(V)];}
        else if(Id=="FrameLimit")Draft.FrameLimit=FPSSettings::Limits[int32(V)];
        else if(Id=="VSync")Draft.VSync=V!=0;else if(Id=="MotionBlur")Draft.MotionBlur=V!=0;else if(Id=="FOV")Draft.FOV=V;
        else if(Id=="Preset"){if(V<4)for(auto& Q:Draft.Quality)Q=V;}
        else if(Id=="MasterVolume")MasterVolume=V;else if(Id=="GameVolume")GameVolume=V;else if(Id=="BackgroundMute")BackgroundMute=V!=0;
        else if(Id=="Sensitivity")Sensitivity=V;else if(Id=="ADSSensitivity")ADSSensitivity=V;else if(Id=="InvertY")InvertY=V!=0;
        else if(Id=="AimToggle")AimToggle=V!=0;else if(Id=="SprintToggle")UFPSInputSettings::Get()->bSprintToggle=V!=0;else if(Id=="CrouchToggle")UFPSInputSettings::Get()->bCrouchToggle=V!=0;
        else if(Id=="HUDScale")HUDScale=V;else if(Id=="BobScale")BobScale=V;
        else for(int32 I=0;I<8;++I)if(Id==FPSSettings::QualityIds[I])Draft.Quality[I]=V;
        if(Page!=0){ApplyImmediate();Save();}else Feedback=TEXT("画面修改待应用。");return true;
    }
    return false;
}
