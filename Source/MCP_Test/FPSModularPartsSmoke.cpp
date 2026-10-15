#include "FPSExtractionSmoke.h"
#include "FPSExtractionMode.h"
#include "FPSExtractionSubsystem.h"
#include "FPSExtractionUI.h"
#include "FPSExtractionWeaponViewport.h"
#include "FPSFactoryBase.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/WidgetPath.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"

void FFPSExtractionSmoke::ModularPartsTick(AFPSExtractionMode* Mode)
{
#if !UE_BUILD_SHIPPING
    const bool Reload=FParse::Param(FCommandLine::Get(),TEXT("FPSModularPartsReload"));
    if(!Mode||(!Reload&&!FParse::Param(FCommandLine::Get(),TEXT("FPSModularPartsSmoke")))||Mode->GetWorld()->GetTimeSeconds()<3)return;
    auto* S=Mode->GetGameInstance()->GetSubsystem<UFPSExtractionSubsystem>();
    if(!S||!S->Profile||!S->SlotPrefix.Contains(TEXT("Parts_")))return;
    static int32 Stage=0;static double At=0;static FGuid GunId;
    if(Stage>40||FPlatformTime::Seconds()-At<1.1)return;
    FString Dir;FParse::Value(FCommandLine::Get(),TEXT("ModularEvidence="),Dir);if(Dir.IsEmpty())return;
    auto Check=[&](bool OK,const TCHAR* Label){UE_LOG(LogTemp,Display,TEXT("FPS_MODULAR_BASE %s stage=%d %s"),OK?TEXT("PASS"):TEXT("FAIL"),Stage,Label);if(!OK){Stage=99;FPlatformMisc::RequestExitWithStatus(false,1);}return OK;};
    auto State=[&]()
    {
        const auto V=S->GetView();const auto& G=V.Loadout.Primary;
        FString T=FString::Printf(TEXT("%s:%d:%d:%u:%d"),*G.InstanceId.ToString(),G.Magazine,G.Reserve,G.ConfigurationRevision,V.Credits);
        for(FName Id:G.AttachmentIds)T+=TEXT("|")+Id.ToString();
        for(const auto& X:V.Warehouse)T+=FString::Printf(TEXT(";%s:%s:%d:%d:%d:%d"),*X.StackId.ToString(),*X.ItemId.ToString(),X.Quantity,X.X,X.Y,X.bRotated);
        return T;
    };
    if(Reload)
    {
        FString Expected;
        if(Check(FFileHelper::LoadFileToString(Expected,*(Dir/TEXT("expected.txt")))&&Expected==State()&&S->ValidateProfile(S->Profile),TEXT("fresh process restores exact assembly ammo GUID and warehouse")))
        {UE_LOG(LogTemp,Display,TEXT("FPS_MODULAR_BASE RELOAD COMPLETE"));Stage=99;FPlatformMisc::RequestExit(false);}return;
    }
    auto* PC=Cast<AFPSFactoryController>(Mode->GetWorld()->GetFirstPlayerController());auto* UI=PC?PC->GetTerminal():nullptr;if(!UI)return;
    auto* Preview=Cast<UFPSExtractionWeaponViewport>(UI->GetWidgetFromName(TEXT("WeaponViewport")));
    auto* Catalog=Mode->GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    auto Pointer=[&](UWidget* Widget,FVector2D Position)
    {
        if(!Widget||!Widget->GetCachedWidget())return false;
        auto& App=FSlateApplication::Get();FWidgetPath Owner;
        if(!App.GeneratePathToWidgetUnchecked(Widget->GetCachedWidget().ToSharedRef(),Owner))return false;
        const auto Path=App.LocateWindowUnderMouse(Position,{Owner.GetWindow()},false);
        if(!Path.ContainsWidget(Widget->GetCachedWidget().Get()))return false;
        const FPointerEvent Down(0,Position,Position,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());
        const FPointerEvent Up(0,Position,Position,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState());
        App.RoutePointerMoveEvent(Path,Up,false);App.RoutePointerDownEvent(Path,Down);App.RoutePointerUpEvent(Path,Up);return true;
    };
    auto Slot=[&](EFPSAttachmentCategory C)
    {
        if(!Check(Preview&&Preview->HasModel(),TEXT("real assembly render target ready")))return;
        const auto G=Preview->GetCachedGeometry();
        Check(Pointer(Preview,G.LocalToAbsolute(UFPSExtractionWeaponViewport::SlotCenter(C)*G.GetLocalSize()))&&UI->GetSelectedAttachmentCategory()==C&&UI->IsAttachmentDrawerOpen(),TEXT("Slate slot card opens corresponding inventory drawer"));
    };
    auto Action=[&](FName ActionName,FName Id)
    {
        UFPSExtractionButton* Found=nullptr;auto* Rows=Cast<UScrollBox>(UI->GetWidgetFromName(TEXT("AttachmentRows")));
        if(Rows)for(auto* Child:Rows->GetAllChildren())if(auto* Row=Cast<UUserWidget>(Child))
            if(auto* B=Cast<UFPSExtractionButton>(Row->GetWidgetFromName(TEXT("RowAction")));B&&B->Action==ActionName&&B->ItemId==Id)Found=B;
        if(!Check(Found&&Found->GetIsEnabled(),TEXT("compatible actual inventory action enabled")))return false;
        static TWeakObjectPtr<UWidget> Scrolled;
        if(Scrolled.Get()!=Found){Rows->ScrollWidgetIntoView(Found,false,EDescendantScrollDestination::Center);Scrolled=Found;At=FPlatformTime::Seconds();return false;}
        const auto G=Found->GetCachedGeometry();const bool Clicked=Check(Pointer(Found,G.GetAbsolutePosition()+G.GetAbsoluteSize()*.5),TEXT("Slate button commits actual installation or removal"));Scrolled.Reset();return Clicked;
    };
    const TCHAR* Ids[]={TEXT("RearGripErgo"),TEXT("RearGripControl"),TEXT("StockLight"),TEXT("StockStable"),TEXT("BarrelLight"),TEXT("BarrelHeavy")};
    if(Stage==0)
    {
        auto* C=S->CopyProfile();C->Warehouse.Reset();C->Loadout.Primary=Catalog->CreateInstance(TEXT("Weapon.M4"));C->Loadout.Primary.Magazine=7;C->Loadout.Primary.Reserve=61;C->Credits=500;
        for(const TCHAR* Id:Ids)Check(S->TryAdd(C->Warehouse,Id,1,64),TEXT("isolated fixture inventory created"));
        for(const TCHAR* Id:{TEXT("Optic"),TEXT("Brake"),TEXT("Magazine")})Check(S->TryAdd(C->Warehouse,Id,1,64),TEXT("original category fixtures created"));
        Check(S->Commit(C),TEXT("isolated profile saved"));GunId=C->Loadout.Primary.InstanceId;
        UI->Dispatch(TEXT("Modification"));UI->Dispatch(TEXT("EnterWeaponModification"));
    }
    if(Stage>=2&&Stage<26)
    {
        const int32 N=(Stage-2)/4,Step=(Stage-2)%4;const auto Category=EFPSAttachmentCategory(3+N/2);
        const FName Id=Ids[N],PartId(*(FString(TEXT("Attachment.M4_"))+Ids[N]));
        if(Step==0)Slot(Category);
        if(Step==1&&!Action(TEXT("Install"),Id))return;
        if(Step==2)
        {
            const auto V=S->GetView();const auto* Part=Catalog->FindAttachment(PartId);const auto& G=V.Loadout.Primary;
            Check(G.AttachmentIds.Contains(PartId)&&S->Count(V.Warehouse,Id)==0&&G.InstanceId==GunId&&G.Magazine==7&&G.Reserve==61,TEXT("installed part debited once and preserves weapon identity and ammo"));
            Check(Part&&Preview->VisualFor(Category).ToString()==Part->Workbench.Mesh.GetAssetName(),TEXT("preview displays saved installed mesh immediately"));
            if(N%2)Check(!G.AttachmentIds.Contains(FName(*(FString(TEXT("Attachment.M4_"))+Ids[N-1])))&&S->Count(V.Warehouse,Ids[N-1])==1,TEXT("replacement returns exactly one previous part"));
            FFPSResolvedWeaponStats Stats;TArray<FString> Errors;Check(Catalog->Resolve(G.WeaponId,G.AttachmentIds,Stats,Errors)&&Stats.AimTransitionTime>0&&Stats.Range>0,TEXT("effective stats resolve from saved definitions"));
            FScreenshotRequest::RequestScreenshot(Dir/(FString(Ids[N])+TEXT(".png")),true,false);
        }
        if(Step==3)UI->Dispatch(TEXT("CloseAttachmentDrawer"));
    }
    if(Stage==26)
    {
        for(const TCHAR* Id:{TEXT("Optic"),TEXT("Brake"),TEXT("Magazine")})Check(S->InstallAttachment(Id),TEXT("original optic muzzle magazine remain installable"));
        const FString Before=State();Check(!S->InstallAttachment(TEXT("StockStable"))&&State()==Before,TEXT("duplicate request cannot charge inventory twice"));
        FFPSResolvedWeaponStats Stats;TArray<FString> Errors;
        Check(!Catalog->Resolve(TEXT("Weapon.M9"),{TEXT("Attachment.M4_StockStable")},Stats,Errors),TEXT("wrong platform weapon rejects M4 stock"));
        auto* Barrel=const_cast<UFPSAttachmentDefinition*>(Catalog->FindAttachment(TEXT("Attachment.M4_BarrelHeavy")));const float Old=Barrel->RangeMultiplier;
        Barrel->RangeMultiplier=-1;Errors.Reset();Check(!Catalog->Resolve(TEXT("Weapon.M4"),{Barrel->AttachmentId},Stats,Errors),TEXT("negative barrel range rejected"));Barrel->RangeMultiplier=Old;
        UI->Refresh();
    }
    if(Stage==27)
    {
        auto* Backup=S->CopyProfile();auto* Full=S->CopyProfile();Full->Warehouse.Reset();
        for(int32 I=0;I<64;++I)Check(S->TryAdd(Full->Warehouse,TEXT("Wire"),20,64),TEXT("full warehouse fixture"));
        Check(S->Commit(Full),TEXT("full warehouse saved"));const FString Before=State();
        Check(!S->RemoveAttachment(TEXT("Attachment.M4_StockStable"))&&State()==Before,TEXT("no return space rejects uninstall atomically"));
        Backup->Revision=S->Profile->Revision;Check(S->Commit(Backup),TEXT("fixture restored with monotonic save revision"));UI->Refresh();
    }
    if(Stage>=28&&Stage<37)
    {
        const int32 N=(Stage-28)/3,Step=(Stage-28)%3;const auto Category=EFPSAttachmentCategory(3+N);
        const FName PartId(*(FString(TEXT("Attachment.M4_"))+Ids[N*2+1]));
        if(Step==0)Slot(Category);
        if(Step==1&&!Action(TEXT("Remove"),PartId))return;
        if(Step==2){const auto V=S->GetView();Check(!V.Loadout.Primary.AttachmentIds.Contains(PartId)&&S->Count(V.Warehouse,Ids[N*2+1])==1,TEXT("uninstall returns item to warehouse"));Check(Preview->VisualFor(Category).ToString().EndsWith(TEXT("Standard")),TEXT("uninstall restores builtin component"));UI->Dispatch(TEXT("CloseAttachmentDrawer"));}
    }
    if(Stage==37)
    {
        for(const TCHAR* Id:{TEXT("RearGripControl"),TEXT("StockStable"),TEXT("BarrelHeavy")})Check(S->InstallAttachment(Id),TEXT("final six-category assembly saved"));
        Check(S->GetView().Loadout.Primary.AttachmentIds.Num()==6,TEXT("all six categories coexist"));
        Check(FFileHelper::SaveStringToFile(State(),*(Dir/TEXT("expected.txt"))),TEXT("persistence expectation written"));UI->Refresh();
    }
    if(Stage==38)FScreenshotRequest::RequestScreenshot(Dir/TEXT("SixSlots.png"),true,false);
    if(Stage==39){UE_LOG(LogTemp,Display,TEXT("FPS_MODULAR_BASE COMPLETE"));Stage=99;FPlatformMisc::RequestExit(false);return;}
    if(Stage<40)++Stage;At=FPlatformTime::Seconds();
#endif
}
