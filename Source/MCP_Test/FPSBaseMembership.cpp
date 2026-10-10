#include "FPSBaseMembership.h"
#include "FPSBaseRegion.h"
#include "FPSModeRules.h"
#include "FPSLifeComponent.h"
#include "FPSDemoCharacter.h"
#include "FPSDemoPlayerState.h"
#include "FPSDemoGameState.h"
#include "FPSWeaponComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "NativeGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SafeDamage, "Character.Protection.HostileDamage");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SafeControl, "Character.Protection.HostileControl");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SafeDisplacement, "Character.Protection.ForcedMovement");
namespace
{
int32 Team(const AActor* Actor)
{
    const auto* PS = Cast<AFPSDemoPlayerState>(Actor);
    if (const auto* Pawn = Cast<APawn>(Actor)) PS = Pawn->GetPlayerState<AFPSDemoPlayerState>();
    if (const auto* Controller = Cast<AController>(Actor)) PS = Controller->GetPlayerState<AFPSDemoPlayerState>();
    if (!PS && Actor && Actor->GetInstigatorController()) PS = Actor->GetInstigatorController()->GetPlayerState<AFPSDemoPlayerState>();
    if (!PS && Actor)
    {
        const AActor* Owner = Actor->GetOwner();
        for (int32 Depth = 0; Owner && Depth < 8 && !PS; ++Depth, Owner = Owner->GetOwner())
        {
            PS = Cast<AFPSDemoPlayerState>(Owner);
            if (const auto* Pawn = Cast<APawn>(Owner)) PS = Pawn->GetPlayerState<AFPSDemoPlayerState>();
            if (const auto* Controller = Cast<AController>(Owner)) PS = Controller->GetPlayerState<AFPSDemoPlayerState>();
        }
    }
    return PS ? PS->TeamId : -1;
}
bool Structured(const UWorld* World)
{
    const auto* GS = World ? World->GetGameState<AFPSDemoGameState>() : nullptr;
    return GS && GS->bStructuredMatch && !GS->bTraining;
}
bool InOwnBase(const AActor* Actor)
{
    if (!Actor || !Structured(Actor->GetWorld())) return false;
    const int32 Id = Team(Actor);
    for (TActorIterator<AFPSBaseRegion> It(Actor->GetWorld()); It; ++It)
        if (Id >= 0 && Id == It->TeamId && It->Contains(Actor->GetActorLocation(), 4)) return true;
    return false;
}
}
UFPSBaseProtectionEffect::UFPSBaseProtectionEffect()
{
    DurationPolicy = EGameplayEffectDurationType::Infinite;
    FInheritedTagContainer Tags;
    for (const FGameplayTag Tag : {TAG_SafeDamage.GetTag(), TAG_SafeControl.GetTag(), TAG_SafeDisplacement.GetTag(),
        FGameplayTag::RequestGameplayTag(TEXT("Character.Restriction.Attack")), FGameplayTag::RequestGameplayTag(TEXT("Character.Restriction.Drop"))})
        Tags.AddTag(Tag);
    auto* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("ProtectionTags"));
    GEComponents.Add(TargetTags);
    TargetTags->SetAndApplyTargetTagChanges(Tags);
}
UFPSBaseMembership::UFPSBaseMembership()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}
void UFPSBaseMembership::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner()->HasAuthority()) LifeId = FGuid::NewGuid();
}
void UFPSBaseMembership::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UFPSBaseMembership, LifeId, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UFPSBaseMembership, bSelectionAllowed, COND_OwnerOnly);
    DOREPLIFETIME(UFPSBaseMembership, bProtected);
}
void UFPSBaseMembership::OnRep_Protection()
{
    if (bProtected) if (auto* Pawn = Cast<AFPSDemoCharacter>(GetOwner())) Pawn->GetWeapons()->CancelAttack();
}
void UFPSBaseMembership::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta, Type, Function);
    EnforceMovement(GetOwner()->GetActorLocation());
    if (GetOwner()->HasAuthority()) RefreshAuthority();
    // Locally clear intent immediately on entry, before replicated GAS tags arrive.
    else if (InOwnBase(GetOwner()))
        if (auto* Pawn = Cast<AFPSDemoCharacter>(GetOwner()); Pawn && Pawn->IsLocallyControlled()) Pawn->GetWeapons()->CancelAttack();
}
bool UFPSBaseMembership::IsProtected() const { return bProtected || InOwnBase(GetOwner()); }
bool UFPSBaseMembership::BlocksAttack(const AActor* Actor) { return InOwnBase(Actor); }
bool UFPSBaseMembership::BlocksDrop(const AActor* Actor) { return InOwnBase(Actor); }
bool UFPSBaseMembership::CanSelect() const
{
    const auto* Pawn = Cast<APawn>(GetOwner());
    const auto* Life = GetOwner()->FindComponentByClass<UFPSLifeComponent>();
    const auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
    if (!Pawn || !Pawn->GetController() || !Pawn->GetController()->IsPlayerController() || !Life || Life->IsDead() || !GS || !GS->IsCombatActive()) return false;
    const auto* Rules = UFPSModeRules::ForWorld(GetWorld());
    if (!Rules) return false;
    if (Rules->Selection == EFPSSelectionPolicy::Always) return true;
    return bSelectionAllowed && InOwnBase(GetOwner());
}
void UFPSBaseMembership::RefreshAuthority()
{
    if (!GetOwner()->HasAuthority()) return;
    auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
    const auto* Pawn = Cast<APawn>(GetOwner());
    const auto* Life = GetOwner()->FindComponentByClass<UFPSLifeComponent>();
    const auto* Rules = UFPSModeRules::ForWorld(GetWorld());
    if (!ASC || !Pawn || !Pawn->GetController() || !Rules || Team(Pawn) < 0) return;
    const bool Alive = Life && !Life->IsDead();
    if (!bInitialized)
    {
        bInitialized = true;
        for (TActorIterator<AFPSBaseRegion> It(GetWorld()); It; ++It)
            if (It->TeamId == Team(Pawn) && It->Contains(Pawn->GetActorLocation(), 4)) { SpawnRegion = *It; break; }
        if (Rules->Selection != EFPSSelectionPolicy::Always && !SpawnRegion.IsValid()) bHasExited = true;
    }
    if (SpawnRegion.IsValid() && !SpawnRegion->Contains(Pawn->GetActorLocation(), 4)) bHasExited = true;
    const bool Selection = Alive && (Rules->Selection == EFPSSelectionPolicy::Always || (!bHasExited && SpawnRegion.IsValid()));
    const bool OldSelection = bSelectionAllowed, OldProtection = bProtected;
    bSelectionAllowed = Selection;
    TSet<TWeakObjectPtr<AFPSBaseRegion>> Current;
    if (Alive && Structured(GetWorld()))
        for (TActorIterator<AFPSBaseRegion> It(GetWorld()); It; ++It)
            if (It->TeamId == Team(Pawn) && It->Contains(Pawn->GetActorLocation(), 4)) Current.Add(*It);
    for (auto It = Effects.CreateIterator(); It; ++It)
        if (!Current.Contains(It.Key()) || !It.Key().IsValid()) { ASC->RemoveActiveGameplayEffect(It.Value()); It.RemoveCurrent(); }
    for (const auto& Region : Current)
        if (!Effects.Contains(Region))
        {
            auto Context = ASC->MakeEffectContext();
            Context.AddSourceObject(Region.Get());
            auto Spec = ASC->MakeOutgoingSpec(UFPSBaseProtectionEffect::StaticClass(), 1, Context);
            if (Spec.IsValid())
            {
                const auto Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
                if (Handle.IsValid()) Effects.Add(Region, Handle);
            }
        }
    bProtected = !Effects.IsEmpty();
    if (!OldProtection && bProtected) OnRep_Protection();
    if (OldSelection != bSelectionAllowed || OldProtection != bProtected) GetOwner()->ForceNetUpdate();
}
void UFPSBaseMembership::EndPlay(const EEndPlayReason::Type Reason)
{
    if (GetOwner()->HasAuthority())
        if (auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
            for (const auto& Pair : Effects) ASC->RemoveActiveGameplayEffect(Pair.Value);
    Effects.Empty();
    Super::EndPlay(Reason);
}
bool UFPSBaseMembership::BlocksHostileDamage(const AActor* Target, const FGameplayEffectContextHandle& Context)
{
    if (!Target) return false;
    const auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(Target));
    if (!InOwnBase(Target) && !(ASC && ASC->HasMatchingGameplayTag(TAG_SafeDamage))) return false;
    const AActor* Source = Context.GetOriginalInstigator();
    if (!Source) Source = Context.GetEffectCauser();
    if (Source == Target || !Source) return false; // Self/environment effects are not hostile by default.
    const int32 SourceTeam = Team(Source), TargetTeam = Team(Target);
    return SourceTeam >= 0 && TargetTeam >= 0 && SourceTeam != TargetTeam;
}
bool UFPSBaseMembership::ClipAttack(UWorld* World, const FVector& Start, FVector& End)
{
    if (!Structured(World)) return false;
    float Nearest = 1;
    bool Blocked = false;
    for (TActorIterator<AFPSBaseRegion> It(World); It; ++It)
    {
        float Entry;
        if (It->SegmentEntry(Start, End, FVector::ZeroVector, Entry) && Entry <= Nearest) { Nearest = Entry; Blocked = true; }
    }
    if (Blocked) End = FMath::Lerp(Start, End, Nearest);
    return Blocked;
}
void UFPSBaseMembership::EnforceMovement(const FVector& Previous)
{
    auto* Pawn = Cast<AFPSDemoCharacter>(GetOwner());
    if (!Pawn || Team(Pawn) < 0 || !Structured(GetWorld()) || Pawn->IsEliminated() || (!Pawn->HasAuthority() && !Pawn->IsLocallyControlled())) return;
    const FVector End = Pawn->GetActorLocation();
    const FVector Padding(Pawn->GetCapsuleComponent()->GetScaledCapsuleRadius(), Pawn->GetCapsuleComponent()->GetScaledCapsuleRadius(), Pawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    float Nearest = 1;
    for (TActorIterator<AFPSBaseRegion> It(GetWorld()); It; ++It)
        if (It->TeamId != Team(Pawn))
        {
            float Entry;
            if (It->SegmentEntry(Previous, End, Padding, Entry)) Nearest = FMath::Min(Nearest, FMath::Max(0.f, Entry - .002f));
        }
    if (Nearest < 1)
    {
        FVector Corrected = FMath::Lerp(Previous, End, Nearest);
        // An exceptional teleport starting inside enemy bounds is expelled to its closest horizontal side.
        for (TActorIterator<AFPSBaseRegion> It(GetWorld()); It; ++It)
            if (It->TeamId != Team(Pawn) && It->Contains(Corrected, Padding.X)) Corrected = It->OutsidePosition(Corrected, Padding.X + 2);
        Pawn->SetActorLocation(Corrected, false, nullptr, ETeleportType::TeleportPhysics);
        Pawn->GetCharacterMovement()->Velocity = FVector::ZeroVector;
    }
    if (Pawn->HasAuthority()) RefreshAuthority();
}
