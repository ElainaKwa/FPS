#include "FPSDemoCharacter.h"
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

	InputMap = CreateDefaultSubobject<UInputMappingContext>(TEXT("FPSInputMap"));
	ForwardAction = CreateDefaultSubobject<UInputAction>(TEXT("ForwardAction"));
	RightAction = CreateDefaultSubobject<UInputAction>(TEXT("RightAction"));
	YawAction = CreateDefaultSubobject<UInputAction>(TEXT("YawAction"));
	PitchAction = CreateDefaultSubobject<UInputAction>(TEXT("PitchAction"));
	FireAction = CreateDefaultSubobject<UInputAction>(TEXT("FireAction"));
	JumpAction = CreateDefaultSubobject<UInputAction>(TEXT("FPSJumpAction"));
	HealAction = CreateDefaultSubobject<UInputAction>(TEXT("HealAction"));
	for (UInputAction* Action : {ForwardAction.Get(), RightAction.Get(), YawAction.Get(), PitchAction.Get()})
		Action->ValueType = EInputActionValueType::Axis1D;
	for (UInputAction* Action : {FireAction.Get(), JumpAction.Get(), HealAction.Get()})
		Action->ValueType = EInputActionValueType::Boolean;
	UInputModifierNegate* Negate = CreateDefaultSubobject<UInputModifierNegate>(TEXT("ReverseAxis"));
	InputMap->MapKey(ForwardAction, EKeys::W);
	InputMap->MapKey(ForwardAction, EKeys::S).Modifiers.Add(Negate);
	InputMap->MapKey(RightAction, EKeys::D);
	InputMap->MapKey(RightAction, EKeys::A).Modifiers.Add(Negate);
	InputMap->MapKey(YawAction, EKeys::MouseX);
	InputMap->MapKey(PitchAction, EKeys::MouseY);
	InputMap->MapKey(FireAction, EKeys::LeftMouseButton);
	InputMap->MapKey(JumpAction, EKeys::SpaceBar);
	InputMap->MapKey(HealAction, EKeys::E);
	auto Action = [this](const TCHAR* Name, const FKey& Key)
	{
		auto* Value = CreateDefaultSubobject<UInputAction>(Name);
		Value->ValueType = EInputActionValueType::Boolean;
		InputMap->MapKey(Value, Key);
		return Value;
	};
	ReloadAction = Action(TEXT("Reload"), EKeys::R);
	AimAction = Action(TEXT("Aim"), EKeys::RightMouseButton);
	InspectAction = Action(TEXT("Inspect"), EKeys::Q);
	PrimaryAction = Action(TEXT("PrimaryWeapon"), EKeys::One);
	SecondaryAction = Action(TEXT("SecondaryWeapon"), EKeys::Two);
	DropAction = Action(TEXT("DropWeapon"), EKeys::G);
	PickupAction = Action(TEXT("PickupWeapon"), EKeys::F);
	SprintAction = Action(TEXT("Sprint"), EKeys::LeftShift);
	CrouchAction = Action(TEXT("Crouch"), EKeys::LeftControl);
}

void AFPSDemoCharacter::BeginPlay()
{
	Super::BeginPlay();
    if (Weapons->IsEnabled())
    {
        CameraFOVBlend = FPSWeaponVisualTiming::FTransition(BaseFieldOfView);
        FirstPersonCamera->SetFieldOfView(BaseFieldOfView);
    }
	AbilitySystem->InitAbilityActorInfo(this, this);
	if (IsLocallyControlled()) AddInputMap();
}

void AFPSDemoCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = RegisteredInputSubsystem.Get())
	{
		Subsystem->RemoveMappingContext(InputMap);
		RegisteredInputSubsystem.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void AFPSDemoCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AbilitySystem->InitAbilityActorInfo(this, this);
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
	if (const auto* PS = GetPlayerState<AFPSDemoPlayerState>(); PS && PS->TeamId >= 0)
	{
		TeamMarker->SetText(FText::FromString(PS->TeamId == 0 ? TEXT("BLUE") : TEXT("RED")));
		TeamMarker->SetTextRenderColor(PS->TeamId == 0 ? FColor(75, 180, 255) : FColor(255, 80, 60));
		TeamMarker->SetVisibility(!bEliminated);
		if (const auto* Local = GetWorld()->GetFirstPlayerController(); Local && Local->GetPawn())
			TeamMarker->SetWorldRotation((Local->GetPawn()->GetPawnViewLocation() - TeamMarker->GetComponentLocation()).Rotation());
	}
	const float Now = GetWorld()->GetTimeSeconds();
	Recoil = Weapons->IsEnabled()
        ? RecoilKick * (1.f - FPSWeaponVisualTiming::Smooth(FPSWeaponVisualTiming::Progress(Now - RecoilStartedAt, VisualRecoilReturnTime)))
        : FMath::FInterpTo(Recoil, 0.0f, DeltaSeconds, 12.0f);
	const float MoveAlpha = FMath::Clamp(GetVelocity().Size2D() / 570.0f, 0.0f, 1.0f);
	const float Bob = FMath::Sin(Now * 13.0f) * 1.7f * MoveAlpha;
	const float WalkSwing = FMath::Sin(Now * 12.0f) * 23.0f * MoveAlpha;
	LeftLeg->SetRelativeRotation(FRotator(WalkSwing, 0, 0));
	RightLeg->SetRelativeRotation(FRotator(-WalkSwing, 0, 0));
	LeftArm->SetRelativeRotation(FRotator(-WalkSwing * 0.55f, 0, 0));
	RightArm->SetRelativeRotation(FRotator(WalkSwing * 0.55f, 0, 0));
	if (!Weapons->IsEnabled())
	{
		GunRoot->SetRelativeLocation(FVector(42.0f - Recoil * 15.0f, 25.0f, -23.0f + Bob));
		GunRoot->SetRelativeRotation(FRotator(Recoil * 6.0f, 0, Recoil * 2.0f));
	}
	MuzzleLight->SetIntensity(Now < LocalFlashUntil ? 7500.0f : 0.0f);
	RemoteMuzzleLight->SetIntensity(Now < RemoteFlashUntil ? 7500.0f : 0.0f);
	if (Weapons->IsEnabled())
	{
		const auto& State = Weapons->GetLoadout();
		const auto* Def = Weapons->Definition();
		FirstPersonRifle->SetVisibility(Def != nullptr);
        const float AimDuration = Def ? Def->AimTransitionTime : 0;
        const float AimAlpha = AimVisualBlend.Update(Now, State.bAiming && Def ? 1.f : 0.f, AimDuration);
        FirstPersonCamera->SetFieldOfView(CameraFOVBlend.Update(Now, State.bAiming && Def ? Def->AimFOV : BaseFieldOfView, AimDuration));
        FirstPersonCamera->SetRelativeLocation(FVector(0, 0, BaseEyeHeight));
        FVector Offset = Def ? FMath::Lerp(Def->HipLocation, Def->AimLocation, AimAlpha) : FVector::ZeroVector;
        Offset.Z += Bob * (1.f - AimAlpha);
        FRotator Rotation(Recoil * 6, 0, 0);
        const double ServerNow = Weapons->ServerTime();
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
        const float SprintAlpha = SprintVisualBlend.Update(Now, GetVelocity().Size2D() > 650.f && !State.bAiming ? 1.f : 0.f, .15f);
        Offset += FVector(-5, -4, -5) * SprintAlpha;
        Rotation += FRotator(-12, -15, 12) * SprintAlpha;
        GunRoot->SetRelativeLocation(Offset - FVector(Recoil * 12, 0, 0));
        GunRoot->SetRelativeRotation(Rotation);
		WorldRifle->SetRelativeRotation((Def ? Def->ThirdPerson.Transform.Rotator() : FRotator::ZeroRotator) + FRotator(State.Action == EFPSWeaponAction::Reloading ? -35 : 0, 0, State.Action == EFPSWeaponAction::Inspecting ? 40 : 0));
	}
	AvatarVisuals->UpdatePose(this, FirstPersonCamera, GunRoot, WorldRifle, DeltaSeconds);
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
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(ForwardAction, ETriggerEvent::Triggered, this, &AFPSDemoCharacter::MoveForward);
		Input->BindAction(RightAction, ETriggerEvent::Triggered, this, &AFPSDemoCharacter::MoveRight);
		Input->BindAction(YawAction, ETriggerEvent::Triggered, this, &AFPSDemoCharacter::LookYaw);
		Input->BindAction(PitchAction, ETriggerEvent::Triggered, this, &AFPSDemoCharacter::LookPitch);
		Input->BindAction(FireAction, ETriggerEvent::Started, this, &AFPSDemoCharacter::PressFire);
		Input->BindAction(FireAction, ETriggerEvent::Completed, this, &AFPSDemoCharacter::ReleaseFire);
		Input->BindAction(FireAction, ETriggerEvent::Canceled, this, &AFPSDemoCharacter::ReleaseFire);
		Input->BindAction(ReloadAction, ETriggerEvent::Started, Weapons.Get(), &UFPSWeaponComponent::Reload);
		Input->BindAction(AimAction, ETriggerEvent::Started, Weapons.Get(), &UFPSWeaponComponent::AimOn);
		Input->BindAction(AimAction, ETriggerEvent::Completed, Weapons.Get(), &UFPSWeaponComponent::AimOff);
		Input->BindAction(AimAction, ETriggerEvent::Canceled, Weapons.Get(), &UFPSWeaponComponent::AimOff);
		Input->BindAction(InspectAction, ETriggerEvent::Started, Weapons.Get(), &UFPSWeaponComponent::Inspect);
		Input->BindAction(PrimaryAction, ETriggerEvent::Started, Weapons.Get(), &UFPSWeaponComponent::Primary);
		Input->BindAction(SecondaryAction, ETriggerEvent::Started, Weapons.Get(), &UFPSWeaponComponent::Secondary);
		Input->BindAction(DropAction, ETriggerEvent::Started, Weapons.Get(), &UFPSWeaponComponent::Drop);
		Input->BindAction(PickupAction, ETriggerEvent::Started, Weapons.Get(), &UFPSWeaponComponent::Pickup);
		Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AFPSDemoCharacter::SprintOn);
		Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AFPSDemoCharacter::SprintOff);
		Input->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AFPSDemoCharacter::SprintOff);
		Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &AFPSDemoCharacter::CrouchOn);
		Input->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AFPSDemoCharacter::CrouchOff);
		Input->BindAction(CrouchAction, ETriggerEvent::Canceled, this, &AFPSDemoCharacter::CrouchOff);
		Input->BindAction(HealAction, ETriggerEvent::Started, this, &AFPSDemoCharacter::PressHeal);
		Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AFPSDemoCharacter::PressJump);
		Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AFPSDemoCharacter::ReleaseJump);
	}
}

void AFPSDemoCharacter::AddInputMap()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (RegisteredInputSubsystem.Get() == Subsystem) return;
			if (UEnhancedInputLocalPlayerSubsystem* PreviousSubsystem = RegisteredInputSubsystem.Get())
				PreviousSubsystem->RemoveMappingContext(InputMap);
			Subsystem->AddMappingContext(InputMap, 1);
			RegisteredInputSubsystem = Subsystem;
		}
	}
}

void AFPSDemoCharacter::MoveForward(const FInputActionValue& Value)
{
	if (!bEliminated) AddMovementInput(GetActorForwardVector(), Value.Get<float>());
}
void AFPSDemoCharacter::MoveRight(const FInputActionValue& Value)
{
	if (!bEliminated) AddMovementInput(GetActorRightVector(), Value.Get<float>());
}
void AFPSDemoCharacter::LookYaw(const FInputActionValue& Value) { AddControllerYawInput(Value.Get<float>()); }
void AFPSDemoCharacter::LookPitch(const FInputActionValue& Value) { AddControllerPitchInput(-Value.Get<float>()); }
void AFPSDemoCharacter::PressJump() { if (!bEliminated) Jump(); }
void AFPSDemoCharacter::ReleaseJump() { StopJumping(); }

void AFPSDemoCharacter::TriggerLocalShotAnimation()
{
	Recoil = 1.0f;
	LocalFlashUntil = GetWorld()->GetTimeSeconds() + 0.07f;
	const auto* Def = Weapons->IsEnabled() ? Weapons->Definition() : Weapons->Catalog()->Find(Weapons->Catalog()->StartingPrimary);
	if (auto* Audio = UFPSAudioSubsystem::Get(this); Audio && Def) Audio->PlaySound(Def->FireSound.Get(), FVector::ZeroVector, true, true);
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

void AFPSDemoCharacter::ServerFire_Implementation()
{
	if (Weapons->IsEnabled()) return; // Structured matches use GAS only; this RPC belongs to the teaching map.
	if (AFPSMatchMode* Rules = GetWorld()->GetAuthGameMode<AFPSMatchMode>())
		if (!Rules->IsCombatActive()) return;
	const float Now = GetWorld()->GetTimeSeconds();
	if (!HasAuthority() || bEliminated || Now - LastServerFire < 0.27f || !Controller) return;
	LastServerFire = Now;
	const auto* Def = Weapons->Catalog()->Find(Weapons->Catalog()->StartingPrimary);
	ResolveShot(6000.f, Def ? Def->FireSound.Get() : nullptr, nullptr);
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
	const FVector End = Start + Direction * Range;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(FPSDemoShot), true, this);
	Query.AddIgnoredActor(this);
	FHitResult Hit;
	GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query);
	MulticastShotFX(Start, FireSound);
	if (AFPSDemoCharacter* Target = Cast<AFPSDemoCharacter>(Hit.GetActor()))
	{
		if (!Target->IsEliminated())
		{
			if (const auto* Rules = GetWorld()->GetAuthGameMode<AFPSMatchMode>())
				if (!Rules->CanDamage(this, Target)) return;
			if (WeaponShot) ApplyDamageEffectTo(Target, *WeaponShot);
			else ApplyTeachingDamageEffectTo(Target);
			const bool bKilled = Target->GetHealth() <= 0.0f;
			if (AFPSDemoPlayerController* PC = Cast<AFPSDemoPlayerController>(Controller)) PC->ConfirmHit(bKilled);
			if (bKilled)
			{
				if (AFPSDemoGameMode* Mode = GetWorld()->GetAuthGameMode<AFPSDemoGameMode>())
					Mode->HandleElimination(Target, Controller);
			}
		}
	}
}

void AFPSDemoCharacter::ApplyDamageEffectTo(AFPSDemoCharacter* Target, const FFPSWeaponShotContext& Shot)
{
	if (!HasAuthority() || Shot.Source.Get() != this || !Shot.InstanceId.IsValid() || Shot.WeaponId.IsNone() ||
		!FMath::IsFinite(Shot.BaseDamage) || Shot.BaseDamage < 0 || !Target || Target->IsEliminated() || !Target->AbilitySystem) return;
	const auto* Rules = GetWorld()->GetAuthGameMode<AFPSMatchMode>();
	if (Rules && !Rules->CanDamage(this, Target)) return;
	FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(UFPSWeaponDamageEffect::StaticClass(), 1.0f, AbilitySystem->MakeEffectContext());
	if (!Spec.IsValid()) return;
	// Target-dependent match policy is evaluated at impact; weapon values were captured at commit.
	const float Multiplier = Rules ? Rules->DamageMultiplier(this, Target) : 1.f;
	Spec.Data->SetSetByCallerMagnitude(FName(TEXT("WeaponDamage")), -Shot.BaseDamage * Multiplier);
	Target->AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void AFPSDemoCharacter::ApplyTeachingDamageEffectTo(AFPSDemoCharacter* Target)
{
	if (!HasAuthority() || !Target || Target->IsEliminated() || !Target->AbilitySystem) return;
	if (const auto* Rules = GetWorld()->GetAuthGameMode<AFPSMatchMode>())
		if (!Rules->CanDamage(this, Target)) return;
	const auto Spec = AbilitySystem->MakeOutgoingSpec(UFPSDemoDamageEffect::StaticClass(), 1.0f, AbilitySystem->MakeEffectContext());
	if (Spec.IsValid()) Target->AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
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

void AFPSDemoCharacter::ServerRequestHeal_Implementation()
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (!HasAuthority() || bEliminated || Now - LastServerHeal < 3.0f) return;
	bool bNearStation = false;
	for (TActorIterator<AFPSDemoHealStation> It(GetWorld()); It; ++It)
	{
		if (It->IsWithinUseRange(GetActorLocation())) { bNearStation = true; break; }
	}
	if (!bNearStation) return;
	LastServerHeal = Now;
	FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(UFPSDemoHealEffect::StaticClass(), 1.0f, AbilitySystem->MakeEffectContext());
	if (Spec.IsValid()) AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void AFPSDemoCharacter::MulticastShotFX_Implementation(FVector_NetQuantize Start, USoundBase* ShotSound)
{
	// Hitscan remains authoritative; no debug ray is rendered.
	const bool bLocalPlayer = IsLocallyControlled() && Controller && Controller->IsPlayerController();
	if (!bLocalPlayer)
	{
		Recoil = 1.0f;
		RemoteFlashUntil = GetWorld()->GetTimeSeconds() + 0.08f;
		if (auto* Audio = UFPSAudioSubsystem::Get(this)) Audio->PlaySound(ShotSound, Start, false, true);
	}
}

float AFPSDemoCharacter::GetRespawnSeconds() const
{
	const auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>();
	return bEliminated && GS ? FMath::Clamp(RespawnsAt - GS->GetServerWorldTimeSeconds(), 0.0, static_cast<double>(RespawnDelay)) : 0.f;
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
	if (Weapons->CanAct())
		if (auto* Movement = Cast<UFPSMovementComponent>(GetCharacterMovement())) Movement->bWantsSprint = true;
}
void AFPSDemoCharacter::SprintOff()
{
	if (auto* Movement = Cast<UFPSMovementComponent>(GetCharacterMovement())) Movement->bWantsSprint = false;
}
void AFPSDemoCharacter::CrouchOn() { if (Weapons->CanAct()) { SprintOff(); Crouch(); } }
void AFPSDemoCharacter::CrouchOff() { UnCrouch(); }
void AFPSDemoCharacter::StopCombatInput() { Weapons->CancelForMenu(); SprintOff(); StopJumping(); }
void AFPSDemoCharacter::PlayWeaponShot(const FVector& Kick)
{
	TriggerLocalShotAnimation();
	Recoil = Kick.Z; RecoilKick = Kick.Z; RecoilStartedAt = GetWorld()->GetTimeSeconds();
	if (Controller && IsLocallyControlled())
	{
		FRotator View = Controller->GetControlRotation();
		View.Pitch = FMath::Clamp(FRotator::NormalizeAxis(View.Pitch) + Kick.X, -85.f, 85.f);
		View.Yaw += Kick.Y;
		Controller->SetControlRotation(View);
	}
}
void AFPSDemoCharacter::RefreshWeaponPresentation()
{
	if (!Weapons->IsEnabled()) return;
	const auto* Def = Weapons->Definition();
	GunRoot->SetVisibility(Def != nullptr, true);
	WorldRifle->SetVisibility(Def != nullptr);
	if (!Def) return;
	FirstPersonRifle->SetStaticMesh(Def->FirstPerson.Mesh.Get());
	WorldRifle->SetStaticMesh(Def->ThirdPerson.Mesh.Get());
	FirstPersonRifle->SetRelativeTransform(Def->FirstPerson.Transform);
	WorldRifle->SetRelativeTransform(Def->ThirdPerson.Transform);
	MuzzleLight->AttachToComponent(FirstPersonRifle, FAttachmentTransformRules::KeepRelativeTransform);
	MuzzleLight->SetRelativeLocation(Def->MuzzleLocation);
	RemoteMuzzleLight->AttachToComponent(WorldRifle, FAttachmentTransformRules::KeepRelativeTransform);
	RemoteMuzzleLight->SetRelativeLocation(Def->MuzzleLocation);
}

void AFPSDemoCharacter::OnRep_Eliminated()
{
	if (bEliminated)
	{
		GetCharacterMovement()->DisableMovement();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
