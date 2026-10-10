#include "FPSWeaponPickup.h"
#include "FPSDynamicDrops.h"
#include "FPSModeRules.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/WorldSettings.h"
#include "Net/UnrealNetwork.h"

AFPSWeaponPickup::AFPSWeaponPickup()
{
    bReplicates = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(20.f);
    SetMinNetUpdateFrequency(2.f);
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .1f;
    Body = CreateDefaultSubobject<UBoxComponent>(TEXT("PickupBody"));
    SetRootComponent(Body);
    Body->InitBoxExtent(FVector(25, 8, 8));
    Body->SetCollisionObjectType(ECC_PhysicsBody);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetCollisionResponseToAllChannels(ECR_Ignore);
    Body->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Body->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    Body->SetLinearDamping(.3f); Body->SetAngularDamping(1.5f);
    Body->BodyInstance.bUseCCD = true;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    Mesh->SetupAttachment(Body);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PickupLabel"));
    Label->SetupAttachment(Body);
    Label->SetRelativeLocation(FVector(0, 0, 65));
    Label->SetWorldSize(23);
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetTextRenderColor(FColor(255, 210, 90));
}

void AFPSWeaponPickup::BeginPlay() { Super::BeginPlay(); OnRep_Instance(); SetActorTickEnabled(HasAuthority()); }
void AFPSWeaponPickup::EndPlay(const EEndPlayReason::Type Reason)
{
    if (HasAuthority()) if (auto* Registry = GetWorld()->GetSubsystem<UFPSDynamicDrops>()) Registry->Unregister(this);
    Super::EndPlay(Reason);
}
void AFPSWeaponPickup::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || !bDynamic || bClaimed) return;
    const FVector Position = GetActorLocation();
    const FBox Bounds = Body->Bounds.GetBox();
    if (Position.ContainsNaN() || Position.Z < GetWorld()->GetWorldSettings()->KillZ ||
        Bounds.Min.GetMin() < -HALF_WORLD_MAX || Bounds.Max.GetMax() > HALF_WORLD_MAX)
    {
        // Drop cleanup also applies when a map disables the engine's generic bounds check.
        FlushNetDormancy(); Destroy(); return;
    }
    if (Body->IsAnyRigidBodyAwake())
    {
        SleepingSince = -1;
        if (NetDormancy != DORM_Awake) { FlushNetDormancy(); SetNetDormancy(DORM_Awake); ForceNetUpdate(); }
    }
    else if (NetDormancy == DORM_Awake)
    {
        if (SleepingSince < 0) SleepingSince = GetWorld()->GetTimeSeconds();
        if (GetWorld()->GetTimeSeconds() - SleepingSince > .5) { ForceNetUpdate(); SetNetDormancy(DORM_DormantAll); }
    }
}
void AFPSWeaponPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AFPSWeaponPickup, Display);
    DOREPLIFETIME(AFPSWeaponPickup, bDynamic);
}
void AFPSWeaponPickup::Initialize(const FFPSWeaponInstance& Value)
{
    if (!HasAuthority()) return;
    Instance = Value;
    Display.InstanceId = Value.InstanceId; Display.WeaponId = Value.WeaponId;
    Display.Magazine = Value.Magazine; Display.Reserve = Value.Reserve;
    OnRep_Instance();
    ForceNetUpdate();
}
bool AFPSWeaponPickup::Claim()
{
    if (!HasAuthority() || !IsAvailable()) return false;
    bClaimed = true;
    FlushNetDormancy();
    return true;
}
void AFPSWeaponPickup::ReleaseClaim() { if (HasAuthority() && !IsActorBeingDestroyed()) bClaimed = false; }
bool AFPSWeaponPickup::LaunchDynamic(const FVector& Velocity, const FVector& AngularVelocity)
{
    const auto* Rules = UFPSModeRules::ForWorld(GetWorld());
    if (!HasAuthority() || !Instance.IsValid() || !Rules || Rules->DynamicDropLimit < 1 || Rules->DynamicDropLifetime <= 0) return false;
    bDynamic = true; OnRep_Instance();
    Body->SetSimulatePhysics(true);
    Body->SetMassOverrideInKg(NAME_None, 2.f);
    Body->SetPhysicsLinearVelocity(Velocity);
    Body->SetPhysicsAngularVelocityInDegrees(AngularVelocity);
    SetLifeSpan(Rules->DynamicDropLifetime);
    if (!GetWorld()->GetSubsystem<UFPSDynamicDrops>()->Register(this)) return false;
    ForceNetUpdate();
    return true;
}
void AFPSWeaponPickup::OnRep_Instance()
{
    if (!HasAuthority())
    {
        Instance = FFPSWeaponInstance();
        Instance.InstanceId = Display.InstanceId; Instance.WeaponId = Display.WeaponId;
        Instance.Magazine = Display.Magazine; Instance.Reserve = Display.Reserve;
    }
    if (!GetGameInstance()) return;
    const auto* Catalog = GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    const auto* Def = Catalog ? Catalog->Find(Instance.WeaponId) : nullptr;
    if (!Def) return;
    // Dedicated servers can strip presentation components from map actors.
    // The authoritative collider still derives its shape from the weapon asset.
    UStaticMesh* PickupMesh = Def->Pickup.Mesh.Get();
    FTransform Visual = Def->Pickup.Transform;
    if (bDynamic && PickupMesh)
    {
        const FBox Bounds = PickupMesh->GetBoundingBox().TransformBy(Visual);
        Body->SetBoxExtent(Bounds.GetExtent().ComponentMax(FVector(3)), false);
        Visual.AddToTranslation(-Bounds.GetCenter());
    }
    if (Mesh)
    {
        Mesh->SetStaticMesh(PickupMesh);
        Mesh->SetRelativeTransform(Visual);
    }
    Body->SetCollisionEnabled(bDynamic ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    // Native RepMovement synchronizes client simulation and authoritative rigid-body snapshots.
    if (!HasAuthority() && bDynamic) Body->SetSimulatePhysics(true);
    if (Label)
    {
        Label->SetAbsolute(false, true, false);
        Label->SetWorldRotation(FRotator::ZeroRotator);
        Label->SetText(FText::FromString(FString::Printf(TEXT("%s  [%d / %d]"), *Def->DisplayName.ToString(), Instance.Magazine, Instance.Reserve)));
    }
}
