#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "PlayInEditorDataTypes.h"
#include "FPSDemoCharacter.h"
#include "FPSMatchFlow.h"
#include "FPSDemoGameState.h"
#include "FPSWeaponComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
class FWeaponPIEExit : public IAutomationLatentCommand
{
public:
    FWeaponPIEExit(FAutomationTestBase* InTest, bool InNetwork, EFPSWeaponAction InAction, bool InModification = false)
        : Test(InTest), Network(InNetwork), Modification(InModification), Action(InAction) {}
    bool Update() override
    {
        if (Phase == 0)
        {
            Settings.Reset(DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage()));
            Settings->SetPlayNetMode(Network ? PIE_ListenServer : PIE_Standalone);
            Settings->SetPlayNumberOfClients(Network ? 2 : 1);
            Settings->SetRunUnderOneProcess(true);
            FRequestPlaySessionParams Params;
            Params.EditorPlaySettings = Settings.Get();
            Params.GameModeOverride = AFPSMatchMode::StaticClass();
            Params.GlobalMapOverride = Network && !Modification ? TEXT("/Game/FPSDemo/Maps/LV_FPSFoundry") : TEXT("/Game/FPSDemo/Maps/LV_FPSTraining");
            PreviousURL = GEditor->UserEditedPlayWorldURL;
            GEditor->UserEditedPlayWorldURL = Params.GlobalMapOverride + (Network ? TEXT("?Mode=LAN?Bots=0") : TEXT("?Mode=Training?Bots=0"));
            GEditor->RequestPlaySession(Params);
            Started = FPlatformTime::Seconds(); Phase = 1;
            return false;
        }
        if (Phase == 3)
        {
            for (const auto& Context : GEngine->GetWorldContexts()) if (Context.WorldType == EWorldType::PIE) return false;
            GEditor->UserEditedPlayWorldURL = PreviousURL;
            if (Armed) Test->AddInfo(FString::Printf(TEXT("PIE_EXIT PASS network=%d action=%d modification=%d editor survived normal EndPlayMap"), Network, int32(Action), Modification));
            return true;
        }
        if (FPlatformTime::Seconds() - Started > 45)
        {
            Test->AddError(TEXT("PIE_EXIT timeout waiting for active weapon action"));
            GEditor->RequestEndPlayMap(); Phase = 3; return false;
        }
        int32 LocalReady = 0, AuthorityActive = 0, ClientActive = 0;
        for (const auto& Context : GEngine->GetWorldContexts())
        {
            UWorld* World = Context.World();
            if (Context.WorldType != EWorldType::PIE || !World || World->bIsTearingDown) continue;
            if (auto* Mode = World->GetAuthGameMode<AFPSMatchMode>())
                if (auto* GS = World->GetGameState<AFPSDemoGameState>(); GS && GS->bWaiting && (Modification ? World->GetNumPlayerControllers() >= 2 : GS->PlayerArray.Num() == 2))
                    Mode->StartRound(World->GetFirstPlayerController());
            for (TActorIterator<AFPSDemoCharacter> It(World); It; ++It)
            {
                auto* PC = Cast<APlayerController>(It->GetController());
                if (!PC) continue;
                auto* W = It->GetWeapons();
                if (W->GetLoadout().Action == Action)
                {
                    if (It->HasAuthority()) ++AuthorityActive; else ++ClientActive;
                }
                if (!Positioned.Contains(*It))
                {
                    // Test-only placement away from protected spawn volumes; no asset edits.
                    It->SetActorLocation(FVector(0, 0, 110));
                    PC->SetControlRotation(FRotator(-70, 0, 0));
                    PC->ResetIgnoreMoveInput();
                    Positioned.Add(*It);
                }
                if (!It->IsLocallyControlled() || !W->CanAct() || !W->Definition()) continue;
                if (W->GetLoadout().Action == Action) { ++LocalReady; continue; }
                if (Modification)
                {
                    if (W->SetCustomizationOpen(true)) W->ModifyAttachment(EFPSAttachmentCategory::Magazine, TEXT("Attachment.M4_MagazineExtended"));
                }
                else if (Action == EFPSWeaponAction::Inspecting) W->Inspect();
                else if (W->Equipped().Magazine == W->Definition()->MagazineSize) W->StartFire();
                else { W->StopFire(); W->Reload(); }
            }
        }
        if (LocalReady >= (Network ? 2 : 1) && AuthorityActive >= (Network ? 2 : 1) && (!Network || ClientActive > 0))
        {
            Test->AddInfo(FString::Printf(TEXT("PIE_EXIT armed network=%d action=%d authority=%d client=%d"), Network, int32(Action), AuthorityActive, ClientActive));
            Armed = true;
            GEditor->RequestEndPlayMap(); Phase = 3;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    bool Network;
    bool Modification;
    bool Armed = false;
    EFPSWeaponAction Action;
    int32 Phase = 0;
    double Started = 0;
    FString PreviousURL;
    TStrongObjectPtr<ULevelEditorPlaySettings> Settings;
    TSet<TWeakObjectPtr<AFPSDemoCharacter>> Positioned;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPSWeaponPIEExitTest, "MCP.FPS.Weapons.ExitPIE", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFPSWeaponPIEExitTest::RunTest(const FString&)
{
    for (bool Network : {false, true})
        for (auto Action : {EFPSWeaponAction::Reloading, EFPSWeaponAction::Inspecting})
            ADD_LATENT_AUTOMATION_COMMAND(FWeaponPIEExit(this, Network, Action));
    for (bool Network : {false, true})
        ADD_LATENT_AUTOMATION_COMMAND(FWeaponPIEExit(this, Network, EFPSWeaponAction::Reloading, true));
    return true;
}
#endif

