#include "FPSAvatarVisuals.h"
#include "FPSHitRegions.h"
#include "FPSDemoCharacter.h"
#include "FPSMovementComponent.h"
#include "FPSWeaponComponent.h"
#include "FPSWeaponVisualTiming.h"
#include "FPSWeaponReloadRig.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

USceneComponent* UFPSAvatarVisuals::Part(const FString& Name) const
{
    const auto* Found = Parts.Find(Name);
    return Found ? Found->Get() : nullptr;
}

bool UFPSAvatarVisuals::Build(AFPSDemoCharacter* Character, USceneComponent* Camera)
{
    const bool Dedicated = Character->GetNetMode() == NM_DedicatedServer;
    // Dedicated servers evaluate the same pose without loading cosmetic avatar meshes.
    TMap<FString, UStaticMesh*> Meshes;
    if (!Dedicated) for (const TCHAR* Name : {TEXT("Torso"), TEXT("Helmet"), TEXT("Pelvis"),
        TEXT("LBoot"), TEXT("RBoot"), TEXT("LUpperArm"), TEXT("RUpperArm"), TEXT("LForearm"), TEXT("RForearm"), TEXT("LThigh"), TEXT("RThigh"), TEXT("LShin"), TEXT("RShin"), TEXT("LGlove"), TEXT("RGlove")})
    {
        const FString Path = FString::Printf(TEXT("/Game/FPSDemo/Art/Industrial/Character/Runtime/SM_IND_RT_%s.SM_IND_RT_%s"), Name, Name);
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
        if (Dedicated)
        {
            auto* Node = NewObject<USceneComponent>(Character, FName(*(TEXT("Avatar_") + Name)));
            Character->AddInstanceComponent(Node);
            Node->SetupAttachment(bFP ? Camera : Character->GetRootComponent());
            Node->RegisterComponent(); Parts.Add(Name, Node);
            return;
        }
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
            Add(FString(Side) + Name, FString(Side) + Name, false);
        for (const TCHAR* Name : {TEXT("Forearm"), TEXT("Glove")}) Add(TEXT("FP") + FString(Side) + Name, FString(Side) + Name, true);
    }
    const TSet<FName> Legacy = {TEXT("ArmorTorso"), TEXT("Helmet"), TEXT("Faceplate"), TEXT("LeftShoulder"),
        TEXT("RightShoulder"), TEXT("LeftArm"), TEXT("RightArm"), TEXT("LeftLeg"), TEXT("RightLeg"),
        TEXT("Backpack"), TEXT("FPLeftHand"), TEXT("FPRightHand")};
    TInlineComponentArray<UStaticMeshComponent*> Existing(Character);
    for (auto* Mesh : Existing) if (Legacy.Contains(Mesh->GetFName()))
    {
        Mesh->SetVisibility(false);
        // Weapon refresh propagates visibility through GunRoot. Keep replaced
        // placeholder hands hidden even when equipping or inspecting again.
        Mesh->SetHiddenInGame(true);
    }
    Character->GetHitRegions()->BindPose(Parts);
    bBuilt = true;
    return true;
}

void UFPSAvatarVisuals::Segment(const FString& Name, FVector Start, FVector End, FVector Up)
{
    if (auto* Mesh = Part(Name))
    {
        const FVector Span = End - Start;
        Mesh->SetRelativeLocation(Start);
        Mesh->SetRelativeRotation(Up.IsNearlyZero() ? Span.Rotation() : FRotationMatrix::MakeFromXZ(Span, Up).Rotator());
        Mesh->SetRelativeScale3D(FVector(Span.Size(), 1, 1));
    }
}

void UFPSAvatarVisuals::UpdatePose(AFPSDemoCharacter* Character, USceneComponent* Camera,
    USceneComponent* Gun, UStaticMeshComponent* WorldGun, float DeltaSeconds,
    const FPSProceduralAnimation::FActionPose& ActionPose)
{
    if (!Character || !Character->GetWeapons()->IsEnabled()) return;
    const auto* PoseDef = Character->GetWeapons()->Definition();
    // Absolute base every frame, including the legacy mesh fallback. Never accumulate aim pitch.
    if (PoseDef) WorldGun->SetRelativeTransform(PoseDef->ThirdPerson.Transform);
    WorldGun->SetRelativeRotation((PoseDef ? PoseDef->ThirdPerson.Transform.Rotator() : FRotator::ZeroRotator) + ActionPose.Rotation);
    WorldGun->SetVisibility(PoseDef && !Character->IsEliminated());
    if (bBuildFailed) return;
    if (!bBuilt && !Build(Character, Camera)) return;
    const bool bAlive = !Character->IsEliminated();
    Character->GetHitRegions()->SetEnabled(bAlive);
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
    const auto* Move=Cast<UFPSMovementComponent>(Character->GetCharacterMovement());
    const bool ActualSprint=Move&&Move->IsSprinting();
    const float WalkSpeed=Move&&Move->UsesMovementRules()?FMath::Max(1.f,Move->MaxWalkSpeed):570.f;
    const FVector Velocity = Character->GetActorRotation().UnrotateVector(Character->GetVelocity());
    const float Speed = Velocity.Size2D();
    const float Blend = 1.f - FMath::Exp(-16.f * DeltaSeconds);
    const FVector BodyVelocity = BodyMotion.Update(FVector(Velocity.X, Velocity.Y, 0), DeltaSeconds, 20.f);
    const float Movement = FMath::Clamp(BodyVelocity.Size2D() / WalkSpeed, 0.f, 1.f);
    const FVector Direction = BodyVelocity.GetSafeNormal();
    Gait = FMath::Fmod(Gait + (bAir ? 0 : Speed) * DeltaSeconds * .023f, 2.f * PI);
    CrouchBlend = FMath::Lerp(CrouchBlend, Character->bIsCrouched ? 1.f : 0.f, Blend);
    SprintBlend = FMath::Lerp(SprintBlend, ActualSprint ? 1.f : 0.f, Blend);
    AirBlend = FMath::Lerp(AirBlend, bAir ? 1.f : 0.f, Blend);
    const float HipZ = FMath::Lerp(-17.f, -30.f, CrouchBlend);
    const float Bob = (1.f - AirBlend) * (1.f - FMath::Cos(Gait * 2.f)) * Movement * .9f;
    const float CounterTwist = FMath::Sin(Gait) * Movement * (1.f - AirBlend) * 3.f;
    const float Lean = FMath::Clamp(BodyVelocity.Y / WalkSpeed, -1.f, 1.f) * -4.f;
    Part(TEXT("Pelvis"))->SetRelativeLocation(FVector(0, 0, HipZ + Bob));
    Part(TEXT("Pelvis"))->SetRelativeRotation(FRotator(0, CounterTwist, Lean * .4f));
    Part(TEXT("Torso"))->SetRelativeLocation(FVector(CrouchBlend * 6 + SprintBlend * 3, 0, HipZ + 16 + Bob));
    Part(TEXT("Torso"))->SetRelativeRotation(FRotator(-CrouchBlend * 12 - SprintBlend * 6, -CounterTwist * .5f, Lean));
    Part(TEXT("Helmet"))->SetRelativeLocation(FVector(CrouchBlend * 12 + SprintBlend * 5, 0, HipZ + FMath::Lerp(70.f, 60.f, CrouchBlend) + Bob));
    Part(TEXT("Helmet"))->SetRelativeRotation(FRotator(FMath::Clamp(FRotator::NormalizeAxis(Character->GetBaseAimRotation().Pitch), -45.f, 45.f), 0, 0));

    const auto* GS = GetWorld()->GetGameState();
    const double Now = GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    const FTransform MeshToWorld = (Def ? Def->FirstPerson.Transform : FTransform::Identity) * Gun->GetComponentTransform();
    const FTransform GunToCamera = MeshToWorld.GetRelativeTransform(Camera->GetComponentTransform());
    const float EquipDrop = Def ? 28.f * (1.f - FPSWeaponVisualTiming::Smooth(FPSWeaponVisualTiming::Progress(Now - State.EquipStartedAt, Def->EquipBlendTime))) : 0.f;
    WorldGun->SetRelativeLocation((Def ? Def->ThirdPerson.Transform.GetLocation() : FVector(43,8,17)) +
        FVector(0,0,-9.f * CrouchBlend - EquipDrop + Bob * .35f) + ActionPose.Location);
    FRotator WorldPose = WorldGun->GetRelativeRotation();
    WorldPose += FRotator(-12, -15, 12) * SprintBlend;
    WorldPose.Pitch += FMath::Clamp(FRotator::NormalizeAxis(Character->GetBaseAimRotation().Pitch), -60.f, 60.f);
    WorldGun->SetRelativeRotation(WorldPose);
    for (int32 Side : {-1, 1})
    {
        const FString Prefix = Side < 0 ? TEXT("L") : TEXT("R");
        const float Phase = Gait + (Side > 0 ? PI : 0.f);
        // The backward half is the planted step; only the forward swing lifts the boot.
        const float Swing = FMath::Max(0.f, FMath::Cos(Phase));
        const float Stride = (1.f - AirBlend) * FMath::Sin(Phase) * 24.f * Movement * (1.f - CrouchBlend * .5f) * (1.f + SprintBlend * .2f);
        const float Lift = FMath::Lerp(Swing * Swing * 10.f * Movement, 12.f, AirBlend);
        const float FloorZ = -Character->GetSimpleCollisionHalfHeight();
        const FVector Hip(0, Side * 12, HipZ);
        const FVector Ankle = FVector(0, Side * 12, FloorZ + 10 + Lift) + Direction * Stride;
        const FVector Knee = (Hip + Ankle) * .5f + FVector(8 + CrouchBlend * 15 + AirBlend * 12, 0, 0);
        Segment(Prefix + TEXT("Thigh"), Hip, Knee);
        Segment(Prefix + TEXT("Shin"), Knee, Ankle);
        Part(Prefix + TEXT("Boot"))->SetRelativeLocation(Ankle - FVector(0, 0, 5));
        Part(Prefix + TEXT("Boot"))->SetRelativeRotation(FRotator(-Swing * 12.f * Movement * (1.f - AirBlend) - AirBlend * 12.f, Direction.Y * 6.f * Movement, 0));

        const FVector RestGrip = Side > 0 ? FVector(-2, 0, -9) : (bPistol ? FVector(-3, -4, -10) : FVector(25, -2, -5));
        FVector Grip = RestGrip;
        if (Side < 0)
        {
            const auto* Rig = Def ? UFPSWeaponReloadRig::Find(Def->FirstPerson.Mesh.Get()) : nullptr;
            if (Rig)
            {
                Grip = ActionPose.SupportGrip(RestGrip, Rig->MagazineGrip, Rig->CyclingGrip, Rig->CyclingTravel);
            }
            else Grip += ActionPose.SupportHand;
        }
        const FVector Hand = WorldGun->GetRelativeTransform().TransformPosition(Grip);
        const FVector Shoulder(CrouchBlend * 6, Side * 26, HipZ + 44 + Bob);
        const FVector RestHand(5, Side * 26, HipZ - 10);
        const FVector Target = Def ? Hand : RestHand;
        const FQuat HandRotation = (Side < 0 ? ActionPose.SupportRotation : FRotator::ZeroRotator).Quaternion();
        const FQuat WorldHandRotation = WorldGun->GetRelativeTransform().GetRotation() * HandRotation;
        // The glove origin is its palm; the forearm meets the wrist above it.
        const FVector Wrist = Target + WorldHandRotation.RotateVector(FVector(0,0,5.5f));
        const FVector Elbow = (Shoulder + Wrist) * .5f + FVector(-8, Side * 9, -16);
        Segment(Prefix + TEXT("UpperArm"), Shoulder, Elbow);
        Segment(Prefix + TEXT("Forearm"), Elbow, Wrist, WorldHandRotation.RotateVector(FVector::UpVector));
        Part(Prefix + TEXT("Glove"))->SetRelativeLocation(Target);
        Part(Prefix + TEXT("Glove"))->SetRelativeRotation(WorldHandRotation);

        const FVector FPHand = GunToCamera.TransformPosition(Grip);
        const FQuat FPHandRotation = GunToCamera.GetRotation() * HandRotation;
        const FVector FPWrist = FPHand + FPHandRotation.RotateVector(FVector(0,0,5.5f));
        const FVector FPElbow = FVector(8, Side * 22, -31) +
            (GunToCamera.GetLocation() - (Def ? Def->HipLocation : FVector::ZeroVector)) * .3f +
            (FPHand - GunToCamera.TransformPosition(RestGrip)) * .4f;
        Segment(TEXT("FP") + Prefix + TEXT("Forearm"), FPElbow, FPWrist, FPHandRotation.RotateVector(FVector::UpVector));
        auto* Glove = Part(TEXT("FP") + Prefix + TEXT("Glove"));
        Glove->SetRelativeLocation(FPHand);
        Glove->SetRelativeRotation(FPHandRotation);
        Glove->SetVisibility(Def != nullptr);
        Part(TEXT("FP") + Prefix + TEXT("Forearm"))->SetVisibility(Def != nullptr);
    }
}
