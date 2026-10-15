#include "FPSAttachmentSmoke.h"
#include "FPSWeaponComponent.h"
#include "FPSDemoCharacter.h"
#include "FPSMatchFlow.h"
#include "FPSDemoGameState.h"
#include "FPSWeaponShot.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"

void FFPSAttachmentSmoke::ModularTick(UFPSWeaponComponent* W)
{
#if !UE_BUILD_SHIPPING
    if (!W->IsEnabled()) return;
    auto* Pawn=W->Character(); const bool Server=Pawn->HasAuthority();
    if (Server==Pawn->IsLocallyControlled() || !Cast<APlayerController>(Pawn->GetController())) return;
    auto* GS=W->GetWorld()->GetGameState<AFPSDemoGameState>();if(!GS || !GS->bTraining)return;
    struct FRun { TSet<int32> Done; bool Failed=false; FGuid Gun; FFPSWeaponShotContext Shot; };
    static TMap<TWeakObjectPtr<UFPSWeaponComponent>,FRun> Runs;auto& R=Runs.FindOrAdd(W);
    auto Check=[&](bool Good,const TCHAR* Label){R.Failed|=!Good;UE_LOG(LogTemp,Display,TEXT("FPS_MODULAR_NET %s %s %s rev=%u"),Good?TEXT("PASS"):TEXT("FAIL"),Server?TEXT("SERVER"):TEXT("CLIENT"),Label,W->Equipped().ConfigurationRevision);};
    if(Server&&!R.Done.Contains(-1))
    {
        if(!W->CanAct())return;R.Done.Add(-1);GS->RoundEndsAt=W->ServerTime()+300;GS->ForceNetUpdate();
        W->StopAllActions();W->Loadout.Primary=W->Catalog()->CreateInstance(TEXT("Weapon.M4"));W->Loadout.ActiveSlot=EFPSWeaponSlot::Primary;
        W->EquippedMutable().Magazine=7;W->EquippedMutable().Reserve=61;W->Changed();
    }
    if(GS->RoundEndsAt<=0)return;
    const double Time=W->ServerTime()-(GS->RoundEndsAt-300);
    auto At=[&](int32 Second,auto Fn){if(Time>=Second&&!R.Done.Contains(Second)){R.Done.Add(Second);Fn();}};
    auto Modify=[&](EFPSAttachmentCategory C,const TCHAR* Id){if(!Server)W->ModifyAttachment(C,FName(Id));};
    auto HasMesh=[&](const TCHAR* Name){TArray<UStaticMeshComponent*> Parts;Pawn->GetComponents(Parts);return Parts.ContainsByPredicate([&](const auto* P){return P->IsVisible()&&P->GetStaticMesh()&&P->GetStaticMesh()->GetFName()==FName(Name);});};
    At(1,[&]{R.Gun=W->Equipped().InstanceId;if(!Server)Cast<AFPSFrontController>(Pawn->GetController())->ToggleCustomization();});
    At(2,[&]{Modify(EFPSAttachmentCategory::RearGrip,TEXT("Attachment.M4_RearGripErgo"));});
    At(3,[&]{Check(W->Equipped().AttachmentIds.Contains(TEXT("Attachment.M4_RearGripErgo"))&&W->EffectiveStats().AimTransitionTime<W->Definition()->AimTransitionTime,TEXT("GAS rear grip replicated and ADS consumed"));Check(HasMesh(TEXT("SM_M4_ModularRearGripErgo")),TEXT("rear grip real component updated"));});
    At(4,[&]{Modify(EFPSAttachmentCategory::Stock,TEXT("Attachment.M4_StockStable"));});
    At(5,[&]{Check(W->Equipped().AttachmentIds.Contains(TEXT("Attachment.M4_StockStable"))&&W->EffectiveStats().VerticalRecoilMultiplier<1,TEXT("GAS stock replicated and recoil consumed"));Check(HasMesh(TEXT("SM_M4_ModularStockStable")),TEXT("stock real component updated"));});
    At(6,[&]{Modify(EFPSAttachmentCategory::Barrel,TEXT("Attachment.M4_BarrelHeavy"));});
    At(7,[&]{Check(W->EffectiveStats().Range>W->Definition()->Range&&W->Equipped().Magazine==7&&W->Equipped().Reserve==61,TEXT("barrel effective range and ammo preserved"));Check(HasMesh(TEXT("SM_M4_ModularBarrelHeavy")),TEXT("barrel real component updated"));if(Server)R.Shot=FFPSWeaponShotContext::Capture(Pawn,W->Equipped(),*W->Definition());});
    At(8,[&]{Modify(EFPSAttachmentCategory::RearGrip,TEXT("Attachment.M4_StockLight"));});
    At(9,[&]{Check(W->Equipped().ConfigurationRevision==4&&W->Equipped().AttachmentIds.Contains(TEXT("Attachment.M4_RearGripErgo")),TEXT("wrong category rejected without corrupting assembly"));});
    At(10,[&]{Modify(EFPSAttachmentCategory::RearGrip,TEXT("Attachment.M4_RearGripControl"));});
    At(11,[&]{Check(W->Equipped().AttachmentIds.Contains(TEXT("Attachment.M4_RearGripControl"))&&!W->Equipped().AttachmentIds.Contains(TEXT("Attachment.M4_RearGripErgo")),TEXT("same slot replaces prior grip"));});
    At(12,[&]{Modify(EFPSAttachmentCategory::Stock,TEXT(""));});
    At(13,[&]{Check(!W->Equipped().AttachmentIds.Contains(TEXT("Attachment.M4_StockStable"))&&HasMesh(TEXT("SM_M4_ModularStockStandard")),TEXT("uninstall restores builtin stock"));});
    At(14,[&]{Modify(EFPSAttachmentCategory::Stock,TEXT("Attachment.M4_StockLight"));});
    At(15,[&]{Check(W->Equipped().InstanceId==R.Gun&&W->Equipped().Magazine==7&&W->Equipped().Reserve==61,TEXT("identity and ammunition survive repeated actions"));});
    At(16,[&]{if(!Server)Cast<AFPSFrontController>(Pawn->GetController())->HideCustomization();});
    At(17,[&]{if(!Server){Check(!W->IsCustomizationOpen()&&!Cast<APlayerController>(Pawn->GetController())->IsMoveInputIgnored()&&W->CanAct(),TEXT("menu closed and gameplay input restored"));W->StartFire();W->StopFire();}});
    At(18,[&]{Check(W->Equipped().Magazine==6&&W->Equipped().ShotSerial>0,TEXT("modified weapon fires through real client GAS input"));if(!Server){FString Capture;if(FParse::Value(FCommandLine::Get(),TEXT("FPSModularCapture="),Capture))FScreenshotRequest::RequestScreenshot(Capture,true,false);}});
    At(19,[&]{if(!Server)Cast<AFPSFrontController>(Pawn->GetController())->ToggleCustomization();});
    At(20,[&]{Modify(EFPSAttachmentCategory::Barrel,TEXT(""));});
    At(21,[&]{Check(FMath::IsNearlyEqual(W->EffectiveStats().Range,W->Definition()->Range)&&HasMesh(TEXT("SM_M4_ModularBarrelStandard")),TEXT("barrel removal restores range and builtin mesh"));if(Server)Check(R.Shot.Range>W->EffectiveStats().Range,TEXT("old authority shot retains captured modified range"));});
    At(22,[&]{Check(!R.Failed,TEXT("COMPLETE"));});
#endif
}
