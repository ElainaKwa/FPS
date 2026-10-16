#include "FPSCharacterPresentation.h"
#include "FPSSystemSettings.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoPlayerState.h"
#include "FPSMovementComponent.h"
#include "FPSWeaponComponent.h"
#include "FPSAvatarVisuals.h"
#include "FPSAttachmentPresentation.h"
#include "FPSAnimationCapture.h"
#include "FPSWeaponReloadRig.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
FPSProceduralAnimation::FActionPose UFPSCharacterPresentation::ActionPoseFor(const UFPSWeaponDefinition* Definition,
    const FFPSWeaponLoadout& State, double Now, float AimAlpha)
{
    FPSProceduralAnimation::FActionPose Pose;
    if (Definition) Pose = ActionBlend.Update(State.Action, State.ActionStartedAt, State.ActionEndsAt, Now);
    // Shared by visible third-person hands and dedicated-server hit bodies.
    if (Definition && Definition->Category == EFPSWeaponCategory::Pistol) Pose.Location.Z *= 1.35f;
    if (State.Action == EFPSWeaponAction::Idle) Pose = FPSProceduralAnimation::FActionPose::Blend({}, Pose, 1.f - AimAlpha);
    return Pose;
}

void UFPSCharacterPresentation::Update(AFPSDemoCharacter* Character, float DeltaSeconds)
{
    if (!Character) return;
    if (Character->GetNetMode() == NM_DedicatedServer)
    {
        if (!Character->Weapons->IsEnabled()) return;
        const auto& State = Character->Weapons->GetLoadout();
        const auto* Def = Character->Weapons->Definition();
        const FGuid Instance = Character->Weapons->Equipped().InstanceId;
        if (Instance != PresentedInstance || Character->IsEliminated()) { ActionBlend = {}; PresentedInstance = Instance; }
        const float AimAlpha = AimVisualBlend.Update(Character->GetWorld()->GetTimeSeconds(), State.bAiming && Def ? 1.f : 0.f,
            Def ? Character->Weapons->EffectiveStats().AimTransitionTime : 0.f);
        const auto Pose = ActionPoseFor(Def, State, Character->Weapons->ServerTime(), AimAlpha);
        Character->AvatarVisuals->UpdatePose(Character, Character->FirstPersonCamera, Character->GunRoot, Character->WorldRifle, DeltaSeconds, Pose);
        return;
    }
	if (const auto* PS = Character->GetPlayerState<AFPSDemoPlayerState>(); PS && PS->TeamId >= 0)
	{
		Character->TeamMarker->SetText(FText::FromString(PS->TeamId == 0 ? TEXT("BLUE") : TEXT("RED")));
		Character->TeamMarker->SetTextRenderColor(PS->TeamId == 0 ? FColor(75, 180, 255) : FColor(255, 80, 60));
		Character->TeamMarker->SetVisibility(!Character->bEliminated);
		if (const auto* Local = Character->GetWorld()->GetFirstPlayerController(); Local && Local->GetPawn())
			Character->TeamMarker->SetWorldRotation((Local->GetPawn()->GetPawnViewLocation() - Character->TeamMarker->GetComponentLocation()).Rotation());
	}
	const float Now = Character->GetWorld()->GetTimeSeconds();
	Recoil = Character->Weapons->IsEnabled()
        ? RecoilKick * (1.f - FPSWeaponVisualTiming::Smooth(FPSWeaponVisualTiming::Progress(Now - RecoilStartedAt, Character->VisualRecoilReturnTime)))
        : FMath::FInterpTo(Recoil, 0.0f, DeltaSeconds, 12.0f);
	const float MoveAlpha = FMath::Clamp(Character->GetVelocity().Size2D() / 570.0f, 0.0f, 1.0f);
	const auto* Settings=Character->IsLocallyControlled()?UFPSSystemSettings::Get(Character):nullptr;
    const float Bob = FMath::Sin(Now * 13.0f) * 1.7f * MoveAlpha * (Settings?Settings->BobScale:1.f);
    if(Settings){Character->FirstPersonCamera->PostProcessSettings.bOverride_MotionBlurAmount=true;Character->FirstPersonCamera->PostProcessSettings.MotionBlurAmount=Settings->MotionBlurEnabled()?.5f:0.f;}
	const float WalkSwing = FMath::Sin(Now * 12.0f) * 23.0f * MoveAlpha;
	Character->LeftLeg->SetRelativeRotation(FRotator(WalkSwing, 0, 0));
	Character->RightLeg->SetRelativeRotation(FRotator(-WalkSwing, 0, 0));
	Character->LeftArm->SetRelativeRotation(FRotator(-WalkSwing * 0.55f, 0, 0));
	Character->RightArm->SetRelativeRotation(FRotator(WalkSwing * 0.55f, 0, 0));
	if (!Character->Weapons->IsEnabled())
	{
		Character->GunRoot->SetRelativeLocation(FVector(42.0f - Recoil * 15.0f, 25.0f, -23.0f + Bob));
		Character->GunRoot->SetRelativeRotation(FRotator(Recoil * 6.0f, 0, Recoil * 2.0f));
	}
	Character->MuzzleLight->SetIntensity(Now < LocalFlashUntil ? 7500.0f : 0.0f);
	Character->RemoteMuzzleLight->SetIntensity(Now < RemoteFlashUntil ? 7500.0f : 0.0f);
    FPSProceduralAnimation::FActionPose ActionPose;
	if (Character->Weapons->IsEnabled())
	{
		const auto& State = Character->Weapons->GetLoadout();
		const auto* Def = Character->Weapons->Definition();
		const auto Stats = Character->Weapons->EffectiveStats();
        const FGuid Instance = Character->Weapons->Equipped().InstanceId;
        if (Instance != PresentedInstance || Character->IsEliminated())
        {
            Locomotion = {}; ActionBlend = {}; PresentedInstance = Instance;
        }
		Character->FirstPersonRifle->SetVisibility(Def && !Character->IsEliminated());
        const float AimDuration = Def ? Stats.AimTransitionTime : 0;
        const float AimAlpha = AimVisualBlend.Update(Now, State.bAiming && Def ? 1.f : 0.f, AimDuration);
        Character->FirstPersonCamera->SetFieldOfView(CameraFOVBlend.Update(Now, State.bAiming && Def ? Stats.AimFOV : (Settings?Settings->BaseFOV():Character->BaseFieldOfView), AimDuration));
        Character->FirstPersonCamera->SetRelativeLocation(FVector(0, 0, Character->BaseEyeHeight));
        FVector Offset = Def ? FMath::Lerp(Def->HipLocation, Stats.AimLocation, AimAlpha) : FVector::ZeroVector;
        FRotator Rotation(Recoil * 6, 0, Recoil * .7f * (1.f - AimAlpha));
        const double ServerNow = Character->Weapons->ServerTime();
        ActionPose = ActionPoseFor(Def, State, ServerNow, AimAlpha);
        Offset += ActionPose.Location; Rotation += ActionPose.Rotation;
        // Equip is an additive offset, never the smoothing speed for ADS, reload or recoil.
        if (Def) Offset.Z -= 28 * (1.f - FPSWeaponVisualTiming::Smooth(FPSWeaponVisualTiming::Progress(ServerNow - State.EquipStartedAt, Def->EquipBlendTime)));
        const auto* Movement=Cast<UFPSMovementComponent>(Character->GetCharacterMovement());
        const bool ActualSprint=Movement&&Movement->IsSprinting()&&!State.bAiming;
        const float SprintBlendDuration=Movement&&Movement->UsesMovementRules()?Movement->GetSprintFirePreparationSeconds():.15f;
        const float SprintAlpha = SprintVisualBlend.Update(Now, ActualSprint ? 1.f : 0.f, FMath::Max(.01f,SprintBlendDuration));
        Offset += FVector(-5, -4, -5) * SprintAlpha;
        Rotation += FRotator(-12, -15, 12) * SprintAlpha;
        const auto Motion = Locomotion.Update(DeltaSeconds, Character->GetActorLocation(), Character->GetVelocity(),
            Character->GetBaseAimRotation(), Movement && Movement->IsMovingOnGround(), Character->bIsCrouched);
        // ADS converges exactly onto the authored sight alignment. BobScale controls all new ambient motion.
        const float MotionScale = (Settings ? Settings->BobScale : 1.f) * FMath::Square(1.f - AimAlpha) *
            AmbientBlend.Update(Now, State.Action == EFPSWeaponAction::Idle ? 1.f : .25f, .12f);
        AmbientOffset = Motion.Location * MotionScale;
        Offset += AmbientOffset; Rotation += Motion.Rotation * MotionScale;
        Character->GunRoot->SetRelativeLocation(Offset - FVector(Recoil * 12, 0, 0));
        Character->GunRoot->SetRelativeRotation(Rotation);
	}
	Character->AvatarVisuals->UpdatePose(Character, Character->FirstPersonCamera, Character->GunRoot, Character->WorldRifle, DeltaSeconds, ActionPose);
    const auto* ReloadDef = Character->Weapons->Definition();
    AnimateFPSReloadParts(ReloadDef ? UFPSWeaponReloadRig::Find(ReloadDef->FirstPerson.Mesh.Get()) : nullptr,
        ActionPose, ReloadDef && !Character->IsEliminated(), FirstPersonReloadParts, FirstPersonAttachments);
    AnimateFPSReloadParts(ReloadDef ? UFPSWeaponReloadRig::Find(ReloadDef->ThirdPerson.Mesh.Get()) : nullptr,
        ActionPose, ReloadDef && !Character->IsEliminated(), ThirdPersonReloadParts, ThirdPersonAttachments);
    TickFPSAnimationCapture(Character, this, Character->GunRoot, Character->FirstPersonCamera);
}


void UFPSCharacterPresentation::Initialize(AFPSDemoCharacter* Character)
{
    if (!Character || Character->GetNetMode() == NM_DedicatedServer) return;
    if (!Character->GetWeapons()->IsEnabled()) return;
    CameraFOVBlend = FPSWeaponVisualTiming::FTransition(Character->BaseFieldOfView);
    Character->FirstPersonCamera->SetFieldOfView(Character->BaseFieldOfView);
}

void UFPSCharacterPresentation::Refresh(AFPSDemoCharacter* Character)
{
    if (!Character || Character->GetNetMode() == NM_DedicatedServer) return;
	if (!Character->Weapons->IsEnabled()) return;
	const auto* Def = Character->Weapons->Definition();
    const bool Visible = Def && !Character->IsEliminated();
	Character->GunRoot->SetVisibility(Visible, true);
	Character->WorldRifle->SetVisibility(Visible, true);
	if (!Visible)
    {
        for (auto& Part : FirstPersonAttachments) if (Part) Part->SetVisibility(false);
        for (auto& Part : ThirdPersonAttachments) if (Part) Part->SetVisibility(false);
        for (auto& Part : FirstPersonReloadParts) if (Part) Part->SetVisibility(false);
        for (auto& Part : ThirdPersonReloadParts) if (Part) Part->SetVisibility(false);
        return;
    }
    RefreshFPSReloadParts(Character->FirstPersonRifle, Def->FirstPerson.Mesh.Get(), true, FirstPersonReloadParts);
    RefreshFPSReloadParts(Character->WorldRifle, Def->ThirdPerson.Mesh.Get(), false, ThirdPersonReloadParts);
	Character->FirstPersonRifle->SetRelativeTransform(Def->FirstPerson.Transform);
    RefreshFPSAttachmentMeshes(Character->FirstPersonRifle, Character->Weapons->Catalog(), Def, Character->Weapons->Equipped().AttachmentIds, EFPSAttachmentView::FirstPerson, FirstPersonAttachments);
    RefreshFPSAttachmentMeshes(Character->WorldRifle, Character->Weapons->Catalog(), Def, Character->Weapons->Equipped().AttachmentIds, EFPSAttachmentView::ThirdPerson, ThirdPersonAttachments);
    if (UFPSWeaponReloadRig::Find(Def->FirstPerson.Mesh.Get()) && FirstPersonAttachments.IsValidIndex(2) && FirstPersonAttachments[2] && FirstPersonAttachments[2]->GetStaticMesh())
    {
        FirstPersonAttachments[2]->AttachToComponent(FirstPersonReloadParts[0], FAttachmentTransformRules::KeepRelativeTransform);
        // Refresh may run after the animation tick; never expose both magazines for one frame.
        FirstPersonReloadParts[0]->SetVisibility(false);
    }
    if (UFPSWeaponReloadRig::Find(Def->ThirdPerson.Mesh.Get()) && ThirdPersonAttachments.IsValidIndex(2) && ThirdPersonAttachments[2] && ThirdPersonAttachments[2]->GetStaticMesh())
    {
        ThirdPersonAttachments[2]->AttachToComponent(ThirdPersonReloadParts[0], FAttachmentTransformRules::KeepRelativeTransform);
        ThirdPersonReloadParts[0]->SetVisibility(false);
    }
	Character->MuzzleLight->AttachToComponent(Character->FirstPersonRifle, FAttachmentTransformRules::KeepRelativeTransform);
	Character->MuzzleLight->SetRelativeLocation(Def->MuzzleLocation);
	Character->RemoteMuzzleLight->AttachToComponent(Character->WorldRifle, FAttachmentTransformRules::KeepRelativeTransform);
	Character->RemoteMuzzleLight->SetRelativeLocation(Def->MuzzleLocation);
}

