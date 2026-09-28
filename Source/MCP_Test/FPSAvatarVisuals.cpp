#include "FPSAvatarVisuals.h"
#include "FPSDemoCharacter.h"
#include "FPSWeaponComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

UStaticMeshComponent* UFPSAvatarVisuals::Part(const FString& Name) const
{
    const auto* Found = Parts.Find(Name);
    return Found ? Found->Get() : nullptr;
}

bool UFPSAvatarVisuals::Build(AFPSDemoCharacter* Character, USceneComponent* Camera)
{
    // Load every required mesh first, so a missing asset leaves the old avatar intact.
    TMap<FString, UStaticMesh*> Meshes;
    for (const TCHAR* Name : {TEXT("Torso"), TEXT("Helmet"), TEXT("Pelvis"), TEXT("Glove"),
        TEXT("Boot"), TEXT("UpperArm"), TEXT("Forearm"), TEXT("Thigh"), TEXT("Shin")})
    {
        const FString Path = FString::Printf(TEXT("/Game/FPSDemo/Avatar/Meshes/SM_AV_%s.SM_AV_%s"), Name, Name);
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
        if (!Mesh)
        {
            UE_LOG(LogTemp, Error, TEXT("FPS avatar missing mesh: %s"), *Path);
            bBuildFailed = true;
            return false;
        }
        Meshes.Add(Name, Mesh);
    }
    auto Add = [&](const FString& Name, const FString& MeshName, bool bFP)
    {
        auto* Mesh = NewObject<UStaticMeshComponent>(Character, FName(*(TEXT("Avatar_") + Name)));
        Character->AddInstanceComponent(Mesh);
        Mesh->SetupAttachment(bFP ? Camera : Character->GetRootComponent());
        Mesh->SetStaticMesh(Meshes[MeshName]);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCanEverAffectNavigation(false);
        Mesh->SetOnlyOwnerSee(bFP);
        Mesh->SetOwnerNoSee(!bFP);
        Mesh->SetCastShadow(!bFP);
        Mesh->RegisterComponent();
        Parts.Add(Name, Mesh);
    };
    for (const TCHAR* Name : {TEXT("Torso"), TEXT("Helmet"), TEXT("Pelvis")}) Add(Name, Name, false);
    for (const TCHAR* Side : {TEXT("L"), TEXT("R")})
    {
        for (const TCHAR* Name : {TEXT("UpperArm"), TEXT("Forearm"), TEXT("Glove"), TEXT("Thigh"), TEXT("Shin"), TEXT("Boot")})
            Add(FString(Side) + Name, Name, false);
        for (const TCHAR* Name : {TEXT("Forearm"), TEXT("Glove")}) Add(TEXT("FP") + FString(Side) + Name, Name, true);
    }
    const TSet<FName> Legacy = {TEXT("ArmorTorso"), TEXT("Helmet"), TEXT("Faceplate"), TEXT("LeftShoulder"),
        TEXT("RightShoulder"), TEXT("LeftArm"), TEXT("RightArm"), TEXT("LeftLeg"), TEXT("RightLeg"),
        TEXT("Backpack"), TEXT("FPLeftHand"), TEXT("FPRightHand")};
    TInlineComponentArray<UStaticMeshComponent*> Existing(Character);
    for (auto* Mesh : Existing) if (Legacy.Contains(Mesh->GetFName())) Mesh->SetVisibility(false);
    bBuilt = true;
    return true;
}

void UFPSAvatarVisuals::Segment(const FString& Name, FVector Start, FVector End)
{
    if (auto* Mesh = Part(Name))
    {
        const FVector Span = End - Start;
        Mesh->SetRelativeLocation(Start);
        Mesh->SetRelativeRotation(Span.Rotation());
        Mesh->SetRelativeScale3D(FVector(Span.Size(), 1, 1));
    }
}

void UFPSAvatarVisuals::UpdatePose(AFPSDemoCharacter* Character, USceneComponent* Camera,
    USceneComponent* Gun, UStaticMeshComponent* WorldGun, float DeltaSeconds)
{
    if (!Character || GetNetMode() == NM_DedicatedServer || !Character->GetWeapons()->IsEnabled() || bBuildFailed) return;
    if (!bBuilt && !Build(Character, Camera)) return;
    const bool bAlive = !Character->IsEliminated();
    for (const auto& Pair : Parts) Pair.Value->SetVisibility(bAlive);
    if (!bAlive)
    {
        WorldGun->SetVisibility(false);
        Gun->SetVisibility(false, true);
        return;
    }

    const auto* Weapons = Character->GetWeapons();
    const auto* Def = Weapons->Definition();
    const auto& State = Weapons->GetLoadout();
    const bool bPistol = Def && Def->Category == EFPSWeaponCategory::Pistol;
    const bool bAir = Character->GetCharacterMovement()->IsFalling();
    const FVector Velocity = Character->GetActorRotation().UnrotateVector(Character->GetVelocity());
    const float Speed = Velocity.Size2D();
    const float Movement = FMath::Clamp(Speed / 570.f, 0.f, 1.f);
    const FVector Direction = FVector(Velocity.X, Velocity.Y, 0).GetSafeNormal();
    Gait = FMath::Fmod(Gait + Speed * DeltaSeconds * .023f, 2.f * PI);
    CrouchBlend = FMath::FInterpTo(CrouchBlend, Character->bIsCrouched ? 1.f : 0.f, DeltaSeconds, 12.f);
    const float HipZ = FMath::Lerp(-17.f, -30.f, CrouchBlend);
    const float Bob = bAir ? 0.f : FMath::Abs(FMath::Sin(Gait)) * Movement * 1.8f;
    Part(TEXT("Pelvis"))->SetRelativeLocation(FVector(0, 0, HipZ + Bob));
    Part(TEXT("Torso"))->SetRelativeLocation(FVector(CrouchBlend * 6, 0, HipZ + 16 + Bob));
    Part(TEXT("Torso"))->SetRelativeRotation(FRotator(-CrouchBlend * 12, 0, 0));
    Part(TEXT("Helmet"))->SetRelativeLocation(FVector(CrouchBlend * 12, 0, HipZ + FMath::Lerp(70.f, 60.f, CrouchBlend) + Bob));
    Part(TEXT("Helmet"))->SetRelativeRotation(FRotator(FMath::Clamp(FRotator::NormalizeAxis(Character->GetBaseAimRotation().Pitch), -45.f, 45.f), 0, 0));

    const auto* GS = GetWorld()->GetGameState();
    const double Now = GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    const float Duration = Def ? Def->ReloadDuration : 2.f;
    const float ReloadProgress = FMath::Clamp(1.f - float(State.ActionEndsAt - Now) / Duration, 0.f, 1.f);
    // Hand leaves the support, reaches below the magazine well, then returns.
    const float ReloadReach = State.Action == EFPSWeaponAction::Reloading ? FMath::Sin(PI * ReloadProgress) : 0.f;
    const FTransform GunToCamera = Gun->GetComponentTransform().GetRelativeTransform(Camera->GetComponentTransform());
    WorldGun->SetRelativeLocation(FVector(bPistol ? 39 : 43, 8, FMath::Lerp(17.f, 8.f, CrouchBlend)));
    FRotator WorldPose = WorldGun->GetRelativeRotation();
    WorldPose.Pitch += FMath::Clamp(FRotator::NormalizeAxis(Character->GetBaseAimRotation().Pitch), -60.f, 60.f);
    WorldGun->SetRelativeRotation(WorldPose);
    for (int32 Side : {-1, 1})
    {
        const FString Prefix = Side < 0 ? TEXT("L") : TEXT("R");
        const float Phase = Gait + (Side > 0 ? PI : 0.f);
        const float Stride = bAir ? 0.f : FMath::Sin(Phase) * 24.f * Movement * (1.f - CrouchBlend * .5f);
        const float Lift = bAir ? 12.f : FMath::Max(0.f, FMath::Cos(Phase)) * 9.f * Movement;
        const float FloorZ = -Character->GetSimpleCollisionHalfHeight();
        const FVector Hip(0, Side * 12, HipZ);
        const FVector Ankle = FVector(0, Side * 12, FloorZ + 10 + Lift) + Direction * Stride;
        const FVector Knee = (Hip + Ankle) * .5f + FVector(8 + CrouchBlend * 15 + (bAir ? 12 : 0), 0, 0);
        Segment(Prefix + TEXT("Thigh"), Hip, Knee);
        Segment(Prefix + TEXT("Shin"), Knee, Ankle);
        Part(Prefix + TEXT("Boot"))->SetRelativeLocation(Ankle - FVector(0, 0, 5));

        FVector Grip = Side > 0 ? FVector(-2, 0, -9) : (bPistol ? FVector(-3, -4, -10) : FVector(25, -2, -5));
        if (Side < 0) Grip += FVector(-10, -7, -24) * ReloadReach;
        const FVector Hand = WorldGun->GetRelativeTransform().TransformPosition(Grip);
        const FVector Shoulder(CrouchBlend * 6, Side * 26, HipZ + 44 + Bob);
        const FVector RestHand(5, Side * 26, HipZ - 10);
        const FVector Target = Def ? Hand : RestHand;
        const FVector Elbow = (Shoulder + Target) * .5f + FVector(-8, Side * 9, -16);
        Segment(Prefix + TEXT("UpperArm"), Shoulder, Elbow);
        Segment(Prefix + TEXT("Forearm"), Elbow, Target);
        Part(Prefix + TEXT("Glove"))->SetRelativeLocation(Target);
        Part(Prefix + TEXT("Glove"))->SetRelativeRotation(WorldGun->GetRelativeRotation());

        const FVector FPHand = GunToCamera.TransformPosition(Grip);
        const FVector FPElbow(8, Side * 22, -31);
        Segment(TEXT("FP") + Prefix + TEXT("Forearm"), FPElbow, FPHand);
        auto* Glove = Part(TEXT("FP") + Prefix + TEXT("Glove"));
        Glove->SetRelativeLocation(FPHand);
        Glove->SetRelativeRotation(GunToCamera.Rotator());
        Glove->SetVisibility(Def != nullptr);
        Part(TEXT("FP") + Prefix + TEXT("Forearm"))->SetVisibility(Def != nullptr);
    }
}
