#include "FPSDemoCharacter.h"
#include "FPSCharacterSmoke.h"
#include "FPSMovementSmoke.h"
#include "FPSLifeComponent.h"
#include "FPSBaseMembership.h"
#include "FPSActionPolicy.h"
#include "FPSCharacterInput.h"
#include "FPSCharacterPresentation.h"
#include "Misc/CommandLine.h"
#include "FPSWeaponShot.h"
#include "FPSTeachingSmoke.h"
#include "FPSAvatarVisuals.h"
#include "FPSAudioSubsystem.h"
#include "Sound/SoundBase.h"

#include "FPSDemoAttributeSet.h"
#include "FPSWeaponComponent.h"
#include "FPSMovementComponent.h"
#include "FPSDemoEffects.h"
#include "FPSDemoGameMode.h"
#include "FPSDemoGameState.h"
#include "FPSMatchFlow.h"
#include "FPSDemoHealStation.h"
#include "FPSDemoPlayerController.h"
#include "FPSDemoPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AFPSDemoCharacter::AFPSDemoCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFPSMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	Life = CreateDefaultSubobject<UFPSLifeComponent>(TEXT("Life"));
	BaseMembership = CreateDefaultSubobject<UFPSBaseMembership>(TEXT("BaseMembership"));
	ActionPolicy = CreateDefaultSubobject<UFPSActionPolicy>(TEXT("ActionPolicy"));
	CharacterInput = CreateDefaultSubobject<UFPSCharacterInput>(TEXT("CharacterInput"));
	Presentation = CreateDefaultSubobject<UFPSCharacterPresentation>(TEXT("Presentation"));
	Weapons = CreateDefaultSubobject<UFPSWeaponComponent>(TEXT("Weapons"));
	AvatarVisuals = CreateDefaultSubobject<UFPSAvatarVisuals>(TEXT("AvatarVisuals"));
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	GetCapsuleComponent()->InitCapsuleSize(38.0f, 88.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCharacterMovement()->MaxWalkSpeed = 570.0f;
	GetCharacterMovement()->JumpZVelocity = 650.0f;
	GetCharacterMovement()->AirControl = 0.3f;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->SetCrouchedHalfHeight(48.f);
	CrouchedEyeHeight = 32.f;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;
	BaseEyeHeight = 64.0f;
	TeamMarker = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TeamMarker"));
	TeamMarker->SetupAttachment(GetCapsuleComponent());
	TeamMarker->SetRelativeLocation(FVector(0, 0, 118));
	TeamMarker->SetHorizontalAlignment(EHTA_Center);
	TeamMarker->SetWorldSize(26);
	TeamMarker->SetOwnerNoSee(true);
	TeamMarker->SetText(FText::GetEmpty());

	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	Attributes = CreateDefaultSubobject<UFPSDemoAttributeSet>(TEXT("Attributes"));

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0, 0, 64));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->FieldOfView = BaseFieldOfView;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RifleMesh(TEXT("/Game/FPSDemo/Meshes/SM_FPS_Rifle.SM_FPS_Rifle"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ArmorMaterial(TEXT("/Game/FPSDemo/Materials/MI_FPS_Armor.MI_FPS_Armor"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WeaponMaterial(TEXT("/Game/FPSDemo/Materials/MI_FPS_Weapon.MI_FPS_Weapon"));
	auto Piece = [this](const TCHAR* Name, USceneComponent* Parent,
		const FVector& Location, const FVector& Scale, bool bFirstPerson, bool bSphere = false)
	{
		UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Mesh->SetupAttachment(Parent);
		Mesh->SetStaticMesh(bSphere ? Sphere.Object : Cube.Object);
		const FString PartName(Name);
		Mesh->SetMaterial(0, bFirstPerson || PartName == TEXT("WorldRifle") || PartName == TEXT("Faceplate")
			? WeaponMaterial.Object : ArmorMaterial.Object);
		Mesh->SetRelativeLocation(Location);
		Mesh->SetRelativeScale3D(Scale);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetOnlyOwnerSee(bFirstPerson);
		Mesh->SetOwnerNoSee(!bFirstPerson);
		Mesh->SetCastShadow(!bFirstPerson);
		return Mesh;
	};

	// Third-person silhouette: remote players can identify the target and aim at its capsule.
	Piece(TEXT("ArmorTorso"), GetCapsuleComponent(), FVector(0, 0, -7), FVector(.56f, .38f, .85f), false);
	Piece(TEXT("Helmet"), GetCapsuleComponent(), FVector(0, 0, 58), FVector(.39f), false, true);
	Piece(TEXT("Faceplate"), GetCapsuleComponent(), FVector(28, 0, 57), FVector(.10f, .52f, .17f), false);
	Piece(TEXT("LeftShoulder"), GetCapsuleComponent(), FVector(0, -47, 23), FVector(.30f, .22f, .24f), false);
	Piece(TEXT("RightShoulder"), GetCapsuleComponent(), FVector(0, 47, 23), FVector(.30f, .22f, .24f), false);
	LeftArm = Piece(TEXT("LeftArm"), GetCapsuleComponent(), FVector(0, -48, -12), FVector(.17f, .18f, .55f), false);
	RightArm = Piece(TEXT("RightArm"), GetCapsuleComponent(), FVector(0, 48, -12), FVector(.17f, .18f, .55f), false);
	LeftLeg = Piece(TEXT("LeftLeg"), GetCapsuleComponent(), FVector(0, -21, -59), FVector(.19f, .2f, .55f), false);
	RightLeg = Piece(TEXT("RightLeg"), GetCapsuleComponent(), FVector(0, 21, -59), FVector(.19f, .2f, .55f), false);
	Piece(TEXT("Backpack"), GetCapsuleComponent(), FVector(-35, 0, 3), FVector(.20f, .45f, .54f), false);
	WorldRifle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorldRifle"));
	WorldRifle->SetupAttachment(GetCapsuleComponent());
	WorldRifle->SetStaticMesh(RifleMesh.Object);
	WorldRifle->SetRelativeLocation(FVector(48, 23, 17));
	WorldRifle->SetRelativeScale3D(FVector(.57f));
	WorldRifle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WorldRifle->SetOwnerNoSee(true);

	// The saved rifle mesh is shared between the first-person view and remote model.
	GunRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ProceduralGunRoot"));
	GunRoot->SetupAttachment(FirstPersonCamera);
	GunRoot->SetRelativeLocation(FVector(42, 25, -23));
	FirstPersonRifle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirstPersonRifle"));
	FirstPersonRifle->SetupAttachment(GunRoot);
	FirstPersonRifle->SetStaticMesh(RifleMesh.Object);
	FirstPersonRifle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonRifle->SetOnlyOwnerSee(true);
	FirstPersonRifle->SetCastShadow(false);
	Piece(TEXT("FPLeftHand"), GunRoot, FVector(34, -13, -21), FVector(.16f), true, true);
	Piece(TEXT("FPRightHand"), GunRoot, FVector(-3, 18, -22), FVector(.17f), true, true);
	MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FPMuzzleFlash"));
	MuzzleLight->SetupAttachment(GunRoot);
	MuzzleLight->SetRelativeLocation(FVector(98, 0, 0));
	MuzzleLight->SetLightColor(FLinearColor(1.0f, 0.55f, 0.08f));
	MuzzleLight->SetAttenuationRadius(310.0f);
	MuzzleLight->SetIntensity(0.0f);
	RemoteMuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RemoteMuzzleFlash"));
	RemoteMuzzleLight->SetupAttachment(GetCapsuleComponent());
	RemoteMuzzleLight->SetRelativeLocation(FVector(105, 22, 22));
	RemoteMuzzleLight->SetLightColor(FLinearColor(1.0f, 0.55f, 0.08f));
	RemoteMuzzleLight->SetAttenuationRadius(310.0f);
	RemoteMuzzleLight->SetIntensity(0.0f);


}

void AFPSDemoCharacter::BeginPlay()
{
	Super::BeginPlay();
    Presentation->Initialize(this);
	AbilitySystem->InitAbilityActorInfo(this, this);
	Life->OnLifeEnded.AddUObject(this, &AFPSDemoCharacter::OnLifeEnded);
	if (IsLocallyControlled()) AddInputMap();
}

void AFPSDemoCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* Move = Cast<UFPSMovementComponent>(GetCharacterMovement())) Move->ClearMovementIntents();
	Weapons->ShutdownActions();
	CharacterInput->Unregister();
	Super::EndPlay(EndPlayReason);
}

void AFPSDemoCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AbilitySystem->InitAbilityActorInfo(this, this);
    if (HasActorBegunPlay()) Weapons->InitializeSpawnLoadout();
    BaseMembership->RefreshAuthority();
}

void AFPSDemoCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	AbilitySystem->InitAbilityActorInfo(this, this);
	AddInputMap();
}

void AFPSDemoCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    FFPSTeachingSmoke::Tick(this);
    FFPSCharacterSmoke::Tick(this);
    FFPSMovementSmoke::TickTeaching(this, DeltaSeconds);
    Presentation->Update(this, DeltaSeconds);
}

void AFPSDemoCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFPSDemoCharacter, bEliminated);
	DOREPLIFETIME_CONDITION(AFPSDemoCharacter, RespawnsAt, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AFPSDemoCharacter, EliminatedBy, COND_OwnerOnly);
}

UAbilitySystemComponent* AFPSDemoCharacter::GetAbilitySystemComponent() const { return AbilitySystem; }
FVector AFPSDemoCharacter::GetPawnViewLocation() const { return GetActorLocation() + FVector(0, 0, BaseEyeHeight); }
float AFPSDemoCharacter::GetHealth() const { return Attributes ? Attributes->GetHealth() : 0.0f; }
float AFPSDemoCharacter::GetMaxHealth() const { return Attributes ? Attributes->GetMaxHealth() : 100.0f; }

void AFPSDemoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    CharacterInput->Bind(this, PlayerInputComponent);
}
void AFPSDemoCharacter::AddInputMap() { CharacterInput->Register(this); }

void AFPSDemoCharacter::MoveForward(const FInputActionValue& Value)
{
	if (!bEliminated && ActionPolicy->CanMove()) AddMovementInput(GetActorForwardVector(), Value.Get<float>());
}
void AFPSDemoCharacter::MoveRight(const FInputActionValue& Value)
{
	if (!bEliminated && ActionPolicy->CanMove()) AddMovementInput(GetActorRightVector(), Value.Get<float>());
}
void AFPSDemoCharacter::LookYaw(const FInputActionValue& Value) { AddControllerYawInput(Value.Get<float>()); }
void AFPSDemoCharacter::LookPitch(const FInputActionValue& Value) { AddControllerPitchInput(-Value.Get<float>()); }
void AFPSDemoCharacter::PressJump()
{
    const auto* PC = Cast<APlayerController>(Controller);
    if (!bEliminated && ActionPolicy->CanMove() && !(PC && PC->IsMoveInputIgnored()))
    {
        auto* Move = Cast<UFPSMovementComponent>(GetCharacterMovement());
        if (Move && Move->UsesMovementRules()) Move->RequestJump(); else Jump();
    }
}
void AFPSDemoCharacter::ReleaseJump() { StopJumping(); }

void AFPSDemoCharacter::TriggerLocalShotAnimation()
{
	Presentation->LocalShot(GetWorld()->GetTimeSeconds());
	const auto* Def = Weapons->IsEnabled() ? Weapons->Definition() : Weapons->Catalog()->Find(Weapons->Catalog()->StartingPrimary);
	if (auto* Audio = UFPSAudioSubsystem::Get(this); Audio && Def) Audio->PlaySound(Weapons->IsEnabled() ? Weapons->EffectiveStats().FireSound.Get() : Def->FireSound.Get(), FVector::ZeroVector, true, true);
}

void AFPSDemoCharacter::PressFire()
{
	if (Weapons->IsEnabled()) { Weapons->StartFire(); return; }
	const float Now = GetWorld()->GetTimeSeconds();
	if (bEliminated || Now - LastLocalFire < 0.27f) return;
	LastLocalFire = Now;
	TriggerLocalShotAnimation();
	ServerFire();
}


void AFPSDemoCharacter::ResolveWeaponShot(const FFPSWeaponShotContext& Shot)
{
	if (Shot.Source.Get() != this || !Shot.InstanceId.IsValid() || Shot.WeaponId.IsNone() || !FMath::IsFinite(Shot.Range) || Shot.Range <= 0) return;
	ResolveShot(Shot.Range, Shot.FireSound.Get(), &Shot);
}

void AFPSDemoCharacter::ResolveShot(float Range, USoundBase* FireSound, const FFPSWeaponShotContext* WeaponShot)
{
	if (!HasAuthority() || !Controller || bEliminated) return;
	const FVector Start = GetPawnViewLocation();
	const FVector Direction = Controller->GetControlRotation().Vector();
	FVector End = Start + Direction * Range;
    if (WeaponShot && !ActionPolicy->CanAttack()) return;
    const bool bBaseBlocked = WeaponShot && UFPSBaseMembership::ClipAttack(GetWorld(), Start, End);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(FPSDemoShot), true, this);
	Query.AddIgnoredActor(this);
	FHitResult Hit;
	GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query);
#if !UE_BUILD_SHIPPING
    if (WeaponShot && FParse::Param(FCommandLine::Get(), TEXT("FPSRecoilSmoke")))
        UE_LOG(LogTemp, Display, TEXT("FPS_RECOIL_SAMPLE AUTH serial=%u time=%.6f pitch=%.6f yaw=%.6f hitPawn=%d target=%s"),
            WeaponShot->ShotSerial, Weapons->ServerTime(), Controller->GetControlRotation().Pitch,
            Controller->GetControlRotation().Yaw, Cast<AFPSDemoCharacter>(Hit.GetActor()) != nullptr, *GetNameSafe(Hit.GetActor()));
#endif
	MulticastShotFX(Start, FireSound);
    if (bBaseBlocked && !Hit.bBlockingHit) return;
	if (AFPSDemoCharacter* Target = Cast<AFPSDemoCharacter>(Hit.GetActor()))
	{
		if (!Target->IsEliminated())
		{
			if (const auto* Rules = GetWorld()->GetAuthGameMode<AFPSMatchMode>())
				if (!Rules->CanDamage(this, Target)) return;
			const float BeforeHealth = Target->GetHealth();
			if (WeaponShot) ApplyDamageEffectTo(Target, *WeaponShot);
			else ApplyTeachingDamageEffectTo(Target);
			if (Target->GetHealth() >= BeforeHealth) return;
			const bool bKilled = Target->GetHealth() <= 0.0f;
			if (AFPSDemoPlayerController* PC = Cast<AFPSDemoPlayerController>(Controller)) PC->ConfirmHit(bKilled);
		}
	}
}

void AFPSDemoCharacter::ApplyDamageEffectTo(AFPSDemoCharacter* Target, const FFPSWeaponShotContext& Shot)
{
	if (!HasAuthority() || Shot.Source.Get() != this || !Shot.InstanceId.IsValid() || Shot.WeaponId.IsNone() ||
		!FMath::IsFinite(Shot.BaseDamage) || Shot.BaseDamage < 0 || !Target || Target->IsEliminated() || !Target->AbilitySystem) return;
	const auto* Rules = GetWorld()->GetAuthGameMode<AFPSMatchMode>();
	if (Rules && !Rules->CanDamage(this, Target)) return;
    if (Rules)
    {
        FVector End = Target->GetPawnViewLocation();
        if (!ActionPolicy->CanAttack() || UFPSBaseMembership::ClipAttack(GetWorld(), GetPawnViewLocation(), End)) return;
    }
	FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(UFPSWeaponDamageEffect::StaticClass(), 1.0f, AbilitySystem->MakeEffectContext());
	if (!Spec.IsValid()) return;
	// Target-dependent match policy is evaluated at impact; weapon values were captured at commit.
	const float Multiplier = Rules ? Rules->DamageMultiplier(this, Target) : 1.f;
	Spec.Data->SetSetByCallerMagnitude(FName(TEXT("WeaponDamage")), -Shot.BaseDamage * Multiplier);
	Target->AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}


void AFPSDemoCharacter::FireBotShot()
{
	if (!HasAuthority()) return;
	if (Weapons->IsEnabled())
	{
		if (Weapons->Equipped().Magazine <= 0) Weapons->Reload();
		else { Weapons->StartFire(); Weapons->StopFire(); }
	}
	else ServerFire_Implementation();
}

void AFPSDemoCharacter::PressHeal()
{
	if (!bEliminated) ServerRequestHeal();
}


void AFPSDemoCharacter::MulticastShotFX_Implementation(FVector_NetQuantize Start, USoundBase* ShotSound)
{
	// Hitscan remains authoritative; no debug ray is rendered.
	const bool bLocalPlayer = IsLocallyControlled() && Controller && Controller->IsPlayerController();
	if (!bLocalPlayer)
	{
		Presentation->RemoteShot(GetWorld()->GetTimeSeconds());
		if (auto* Audio = UFPSAudioSubsystem::Get(this)) Audio->PlaySound(ShotSound, Start, false, true);
	}
}

float AFPSDemoCharacter::GetRespawnSeconds() const
{
	const auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
	return bEliminated && GS ? FMath::Clamp(RespawnsAt - GS->GetServerWorldTimeSeconds(), 0.0, static_cast<double>(RespawnDelay)) : 0.f;
}

void AFPSDemoCharacter::OnLifeEnded(UFPSLifeComponent* Component, const FFPSDeathContext& Context)
{
    MarkEliminated(Context.SourceName);
}

void AFPSDemoCharacter::MarkEliminated(const FString& KillerName)
{
	if (!HasAuthority() || bEliminated) return;
	const auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
	RespawnsAt = (GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds()) + RespawnDelay;
	EliminatedBy = KillerName.Left(32);
	bEliminated = true;
	Weapons->StopAllActions();
	OnRep_Eliminated();
	ForceNetUpdate();
}

void AFPSDemoCharacter::ReleaseFire() { Weapons->StopFire(); }
void AFPSDemoCharacter::SprintOn()
{
    const auto* PC = Cast<APlayerController>(Controller);
	if (!bEliminated && GetWorld()->GetGameState<AFPSDemoGameState>()->bStructuredMatch && ActionPolicy->CanMove() && !(PC && PC->IsMoveInputIgnored()))
		if (auto* Movement = Cast<UFPSMovementComponent>(GetCharacterMovement())) Movement->RequestSprint(true);
}
void AFPSDemoCharacter::SprintOff()
{
	if (auto* Movement = Cast<UFPSMovementComponent>(GetCharacterMovement())) Movement->RequestSprint(false);
}
void AFPSDemoCharacter::CrouchOn()
{
    const auto* PC = Cast<APlayerController>(Controller);
    if (!bEliminated && GetWorld()->GetGameState<AFPSDemoGameState>()->bStructuredMatch && ActionPolicy->CanMove() && !(PC && PC->IsMoveInputIgnored()))
        if (auto* Move = Cast<UFPSMovementComponent>(GetCharacterMovement())) Move->RequestCrouch(true);
}
void AFPSDemoCharacter::CrouchOff()
{
    if (auto* Move = Cast<UFPSMovementComponent>(GetCharacterMovement()); Move && Move->UsesMovementRules()) Move->RequestCrouch(false);
    else UnCrouch();
}
void AFPSDemoCharacter::StopCombatInput()
{
    Weapons->CancelForMenu();
    if (auto* Move = Cast<UFPSMovementComponent>(GetCharacterMovement())) Move->ClearMovementIntents();
    StopJumping();
}
void AFPSDemoCharacter::PlayWeaponShot(const FVector& Kick)
{
	TriggerLocalShotAnimation();
	Presentation->WeaponKick(Kick.Z, GetWorld()->GetTimeSeconds());
	if (Controller && IsLocallyControlled())
	{
		FRotator View = Controller->GetControlRotation();
		View.Pitch = FMath::Clamp(FRotator::NormalizeAxis(View.Pitch) + Kick.X, -85.f, 85.f);
		View.Yaw += Kick.Y;
		Controller->SetControlRotation(View);
	}
}
void AFPSDemoCharacter::RefreshWeaponPresentation() { Presentation->Refresh(this); }

void AFPSDemoCharacter::OnRep_Eliminated()
{
	if (bEliminated)
	{
		if (auto* Move = Cast<UFPSMovementComponent>(GetCharacterMovement())) Move->ClearMovementIntents();
		GetCharacterMovement()->DisableMovement();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
