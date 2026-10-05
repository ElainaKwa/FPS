#include "FPSWeaponPickup.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"

AFPSWeaponPickup::AFPSWeaponPickup()
{
    bReplicates = true;
    SetReplicateMovement(true);
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    auto* Root = CreateDefaultSubobject<USceneComponent>(TEXT("PickupRoot"));
    SetRootComponent(Root);
    Mesh->SetupAttachment(Root);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PickupLabel"));
    Label->SetupAttachment(Root);
    Label->SetRelativeLocation(FVector(0, 0, 65));
    Label->SetWorldSize(23);
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetTextRenderColor(FColor(255, 210, 90));
}

void AFPSWeaponPickup::BeginPlay() { Super::BeginPlay(); OnRep_Instance(); }
void AFPSWeaponPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AFPSWeaponPickup, Display);
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
    SetActorEnableCollision(false);
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
    Mesh->SetStaticMesh(Def->Pickup.Mesh.Get());
    Mesh->SetRelativeTransform(Def->Pickup.Transform);
    Label->SetText(FText::FromString(FString::Printf(TEXT("%s  [%d / %d]"), *Def->DisplayName.ToString(), Instance.Magazine, Instance.Reserve)));
}
