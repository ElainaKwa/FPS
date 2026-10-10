#include "FPSCharacterPresentation.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoPlayerState.h"
#include "FPSWeaponComponent.h"
#include "FPSAvatarVisuals.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
void UFPSCharacterPresentation::Update(AFPSDemoCharacter* Character, float DeltaSeconds)
{
	if (!Character || Character->GetNetMode() == NM_DedicatedServer) return;
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
	const float Bob = FMath::Sin(Now * 13.0f) * 1.7f * MoveAlpha;
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
	if (Character->Weapons->IsEnabled())
	{
		const auto& State = Character->Weapons->GetLoadout();
		const auto* Def = Character->Weapons->Definition();
		Character->FirstPersonRifle->SetVisibility(Def && !Character->IsEliminated());
        const float AimDuration = Def ? Def->AimTransitionTime : 0;
        const float AimAlpha = AimVisualBlend.Update(Now, State.bAiming && Def ? 1.f : 0.f, AimDuration);
        Character->FirstPersonCamera->SetFieldOfView(CameraFOVBlend.Update(Now, State.bAiming && Def ? Def->AimFOV : Character->BaseFieldOfView, AimDuration));
        Character->FirstPersonCamera->SetRelativeLocation(FVector(0, 0, Character->BaseEyeHeight));
        FVector Offset = Def ? FMath::Lerp(Def->HipLocation, Def->AimLocation, AimAlpha) : FVector::ZeroVector;
        Offset.Z += Bob * (1.f - AimAlpha);
        FRotator Rotation(Recoil * 6, 0, 0);
        const double ServerNow = Character->Weapons->ServerTime();
        if (Def && State.Action == EFPSWeaponAction::Inspecting)
        {
            const float Phase = FPSWeaponVisualTiming::Progress(Def->InspectDuration - (State.ActionEndsAt - ServerNow), Def->InspectDuration);
            Rotation += FRotator(12, 30, 60 * FMath::Sin(Phase * 4.f)) * FMath::Sin(PI * Phase);
        }
        if (Def && State.Action == EFPSWeaponAction::Reloading)
        {
            const float Phase = FPSWeaponVisualTiming::Progress(Def->ReloadDuration - (State.ActionEndsAt - ServerNow), Def->ReloadDuration);
            const float Reach = FMath::Sin(PI * Phase);
            Offset.Z -= 12 * Reach; Rotation.Roll -= 40 * Reach;
        }
        // Equip is an additive offset, never the smoothing speed for ADS, reload or recoil.
        if (Def) Offset.Z -= 28 * (1.f - FPSWeaponVisualTiming::Smooth(FPSWeaponVisualTiming::Progress(ServerNow - State.EquipStartedAt, Def->EquipBlendTime)));
        const float SprintAlpha = SprintVisualBlend.Update(Now, Character->GetVelocity().Size2D() > 650.f && !State.bAiming ? 1.f : 0.f, .15f);
        Offset += FVector(-5, -4, -5) * SprintAlpha;
        Rotation += FRotator(-12, -15, 12) * SprintAlpha;
        Character->GunRoot->SetRelativeLocation(Offset - FVector(Recoil * 12, 0, 0));
        Character->GunRoot->SetRelativeRotation(Rotation);
	}
	Character->AvatarVisuals->UpdatePose(Character, Character->FirstPersonCamera, Character->GunRoot, Character->WorldRifle, DeltaSeconds);
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
	Character->GunRoot->SetVisibility(Def != nullptr, true);
	Character->WorldRifle->SetVisibility(Def != nullptr);
	if (!Def) return;
	Character->FirstPersonRifle->SetStaticMesh(Def->FirstPerson.Mesh.Get());
	Character->WorldRifle->SetStaticMesh(Def->ThirdPerson.Mesh.Get());
	Character->FirstPersonRifle->SetRelativeTransform(Def->FirstPerson.Transform);
	Character->MuzzleLight->AttachToComponent(Character->FirstPersonRifle, FAttachmentTransformRules::KeepRelativeTransform);
	Character->MuzzleLight->SetRelativeLocation(Def->MuzzleLocation);
	Character->RemoteMuzzleLight->AttachToComponent(Character->WorldRifle, FAttachmentTransformRules::KeepRelativeTransform);
	Character->RemoteMuzzleLight->SetRelativeLocation(Def->MuzzleLocation);
}

