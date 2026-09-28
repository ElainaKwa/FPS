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
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PickupLabel"));
    Label->SetupAttachment(Mesh);
    Label->SetRelativeLocation(FVector(0, 0, 65));
    Label->SetWorldSize(23);
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetTextRenderColor(FColor(255, 210, 90));
}

void AFPSWeaponPickup::BeginPlay() { Super::BeginPlay(); OnRep_Instance(); }
void AFPSWeaponPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AFPSWeaponPickup, Instance);
}
void AFPSWeaponPickup::Initialize(const FFPSWeaponInstance& Value)
{
    if (!HasAuthority()) return;
    Instance = Value;
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
    if (!GetGameInstance()) return;
    const auto* Catalog = GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    const auto* Def = Catalog ? Catalog->Find(Instance.Model) : nullptr;
    if (!Def) return;
    Mesh->SetStaticMesh(Def->Mesh.LoadSynchronous());
    Mesh->SetWorldScale3D(Def->PresentationScale * .65f);
    Label->SetText(FText::FromString(FString::Printf(TEXT("%s  [%d / %d]"), *Def->DisplayName.ToString(), Instance.Magazine, Instance.Reserve)));
}
