// Copyright Epic Games, Inc. All Rights Reserved.

#include "BreakingWaveCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "HeadbobCameraShake.h"
#include "HitCameraShake.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "MGBunkerSystem.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "BreakingWave.h"

ABreakingWaveCharacter::ABreakingWaveCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	FirstPersonRifleMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Rifle"));
	FirstPersonRifleMesh->SetupAttachment(FirstPersonMesh, FName("HandGrip_R"));
	FirstPersonRifleMesh->SetOnlyOwnerSee(true);
	FirstPersonRifleMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonRifleMesh->SetCollisionProfileName(FName("NoCollision"));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> RifleMeshFinder(TEXT("/Game/Weapons/Rifle/Meshes/SKM_Rifle.SKM_Rifle"));
	if (RifleMeshFinder.Succeeded())
	{
		FirstPersonRifleMesh->SetSkeletalMesh(RifleMeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> FireAnimFinder(TEXT("/Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_Fire.MM_Rifle_Fire"));
	if (FireAnimFinder.Succeeded())
	{
		RifleFireAnim = FireAnimFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> ReloadAnimFinder(TEXT("/Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_Reload.MM_Rifle_Reload"));
	if (ReloadAnimFinder.Succeeded())
	{
		RifleReloadAnim = ReloadAnimFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> DryFireAnimFinder(TEXT("/Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_DryFire.MM_Rifle_DryFire"));
	if (DryFireAnimFinder.Succeeded())
	{
		RifleDryFireAnim = DryFireAnimFinder.Object;
	}

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	DebugThirdPersonSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Debug Third Person Spring Arm"));
	DebugThirdPersonSpringArm->SetupAttachment(GetCapsuleComponent());
	DebugThirdPersonSpringArm->bUsePawnControlRotation = true;

	DebugThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Debug Third Person Camera"));
	DebugThirdPersonCamera->SetupAttachment(DebugThirdPersonSpringArm, USpringArmComponent::SocketName);
	DebugThirdPersonCamera->SetAutoActivate(false);

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
}

void ABreakingWaveCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->SetCrouchedHalfHeight(ProneCapsuleHalfHeight);
	GetCharacterMovement()->MaxWalkSpeedCrouched = 0.f;
	StandingFirstPersonMeshRelativeLocation = FirstPersonMesh->GetRelativeLocation();
	StandingBodyAnimClass = GetMesh()->GetAnimClass();

	MagRounds = RifleProfile.MagazineSize;
	DefaultFieldOfView = FirstPersonCameraComponent->FieldOfView;

	if (RifleShotSound == nullptr)
	{
		RifleShotSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/RifleShotPlayer.RifleShotPlayer"));
	}
	if (RifleDryClickSound == nullptr)
	{
		RifleDryClickSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/RifleDryClick.RifleDryClick"));
	}
	if (RifleReloadSound == nullptr)
	{
		RifleReloadSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/RifleReload.RifleReload"));
	}
	if (PainSound == nullptr)
	{
		PainSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/PlayerPain.PlayerPain"));
	}

	SetUpBodyHitVolume();
}

void ABreakingWaveCharacter::SetUpBodyHitVolume()
{
	USkeletalMeshComponent* Body = GetMesh();

	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	if (Body->GetPhysicsAsset() == nullptr)
	{
		Body->SetPhysicsAsset(LoadObject<UPhysicsAsset>(nullptr, TEXT("/Game/Characters/Mannequins/Rigs/PA_Mannequin.PA_Mannequin")));
	}

	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionResponseToAllChannels(ECR_Ignore);
	Body->RecreatePhysicsState();

	bBodyTraceUnavailable = Body->GetPhysicsAsset() == nullptr || Body->Bodies.Num() == 0;
	if (bBodyTraceUnavailable)
	{
		UE_LOG(LogBreakingWave, Error,
			TEXT("Body mesh has no usable physics bodies — bullets fall back to the capsule and headshots are impossible."));
	}
}

void ABreakingWaveCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraShake(UHeadbobCameraShake::StaticClass());
		}
	}
}

void ABreakingWaveCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABreakingWaveCharacter::MoveInput);

		// Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ABreakingWaveCharacter::DoSprintStart);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ABreakingWaveCharacter::DoSprintEnd);

		// Prone
		EnhancedInputComponent->BindAction(ProneAction, ETriggerEvent::Started, this, &ABreakingWaveCharacter::DoProneToggle);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABreakingWaveCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ABreakingWaveCharacter::LookInput);

		// Rifle
		if (FireAction != nullptr)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ABreakingWaveCharacter::DoFire);
		}
		if (AimAction != nullptr)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ABreakingWaveCharacter::DoAimStart);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ABreakingWaveCharacter::DoAimEnd);
		}
		if (ReloadAction != nullptr)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ABreakingWaveCharacter::DoReload);
		}
	}
	else
	{
		UE_LOG(LogBreakingWave, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void ABreakingWaveCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void ABreakingWaveCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void ABreakingWaveCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController() && !bTransitionInputLocked && !bDead)
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ABreakingWaveCharacter::DoMove(float Right, float Forward)
{
	if (GetController() && !IsProne() && !IsProneTransitionActive() && !bAiming
		&& !bTransitionInputLocked && !bDead)
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

bool ABreakingWaveCharacter::IsProneTransitionActive() const
{
	return GetWorld()->GetTimeSeconds() < ProneTransitionEndTime;
}

void ABreakingWaveCharacter::DoProneToggle()
{
	if (bTransitionInputLocked || bDead)
	{
		return;
	}

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABreakingWaveCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	BeginSlideFromMomentum();
	FirstPersonMesh->SetVisibility(false);

	const float DropDuration = (bDiveEyeDropPending ? PredictDiveFlightTime() : 0.f) + ProneDropDuration;

	if (!bDiveEyeDropPending)
	{
		StartEyeDropToProne();
	}
	ProneTransitionEndTime = GetWorld()->GetTimeSeconds() + DropDuration;

	GetWorldTimerManager().ClearTimer(ProneBodyAnimTimer);
	if (StandToProneAnim && ProneBodyIdleAnim)
	{
		PlayBodyAnimCompressed(StandToProneAnim, DropDuration);
		GetWorldTimerManager().SetTimer(ProneBodyAnimTimer, this, &ABreakingWaveCharacter::BeginProneBodyIdle, DropDuration);
	}
	else if (ProneBodyIdleAnim)
	{
		GetMesh()->PlayAnimation(ProneBodyIdleAnim, true);
	}
}

void ABreakingWaveCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	bDiveEyeDropPending = false;
	RestoreDiveFallGravity();
	SettleSlide();
	StartEyeHeightBlend(StandingFirstPersonMeshRelativeLocation, ProneStandUpDuration);
	FirstPersonMesh->SetVisibility(true);
	ProneTransitionEndTime = GetWorld()->GetTimeSeconds() + ProneStandUpDuration;

	GetWorldTimerManager().ClearTimer(ProneBodyAnimTimer);
	if (ProneToStandAnim)
	{
		PlayBodyAnimCompressed(ProneToStandAnim, ProneStandUpDuration);
		GetWorldTimerManager().SetTimer(ProneBodyAnimTimer, this, &ABreakingWaveCharacter::RestoreStandingBodyAnim, ProneStandUpDuration);
	}
	else if (ProneBodyIdleAnim)
	{
		RestoreStandingBodyAnim();
	}
}

void ABreakingWaveCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bAutoAdvancing && !IsProne() && !IsProneTransitionActive())
	{
		AddMovementInput(GetActorForwardVector(), 1.f);
	}

	const float TargetFov = bAiming ? AimFieldOfView : DefaultFieldOfView;
	if (!FMath::IsNearlyEqual(FirstPersonCameraComponent->FieldOfView, TargetFov, 0.01f))
	{
		FirstPersonCameraComponent->SetFieldOfView(
			FMath::FInterpTo(FirstPersonCameraComponent->FieldOfView, TargetFov, DeltaSeconds, AimFovBlendSpeed));
	}

	if (bSlideActive && GetCharacterMovement()->Velocity.Size2D() <= SlideSettleSpeed)
	{
		SettleSlide();
	}

	if (bDiveEyeDropPending && !bDiveFallGravityActive
		&& GetCharacterMovement()->IsFalling() && GetCharacterMovement()->Velocity.Z <= 0.f)
	{
		PreDiveGravityScale = GetCharacterMovement()->GravityScale;
		GetCharacterMovement()->GravityScale = PreDiveGravityScale * ProneDiveFallGravityScale;
		bDiveFallGravityActive = true;
	}

	if (bEyeHeightBlendActive)
	{
		EyeHeightBlendElapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(EyeHeightBlendElapsed / EyeHeightBlendDuration, 0.f, 1.f);
		FirstPersonMesh->SetRelativeLocation(FMath::Lerp(EyeHeightBlendStart, EyeHeightBlendTarget, FMath::SmoothStep(0.f, 1.f, Alpha)));
		bEyeHeightBlendActive = Alpha < 1.f;
	}
}

void ABreakingWaveCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	RestoreDiveFallGravity();
	if (bDiveEyeDropPending)
	{
		bDiveEyeDropPending = false;
		StartEyeDropToProne();
		ProneTransitionEndTime = GetWorld()->GetTimeSeconds() + ProneDropDuration;
	}
}

float ABreakingWaveCharacter::PredictDiveFlightTime() const
{
	const float Gravity = FMath::Abs(GetCharacterMovement()->GetGravityZ());
	const float RiseTime = ProneDiveUpwardSpeed / Gravity;
	const float FallTime = RiseTime / FMath::Sqrt(FMath::Max(ProneDiveFallGravityScale, 0.1f));
	return RiseTime + FallTime;
}

void ABreakingWaveCharacter::RestoreDiveFallGravity()
{
	if (bDiveFallGravityActive)
	{
		GetCharacterMovement()->GravityScale = PreDiveGravityScale;
		bDiveFallGravityActive = false;
	}
}

void ABreakingWaveCharacter::StartEyeDropToProne()
{
	const float GroundZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float EyeDropToProne = FirstPersonCameraComponent->GetComponentLocation().Z - (GroundZ + ProneEyeHeight);
	StartEyeHeightBlend(FirstPersonMesh->GetRelativeLocation() - FVector(0.f, 0.f, EyeDropToProne), ProneDropDuration);
}

void ABreakingWaveCharacter::StartEyeHeightBlend(const FVector& TargetRelativeLocation, float Duration)
{
	if (Duration <= 0.f)
	{
		bEyeHeightBlendActive = false;
		FirstPersonMesh->SetRelativeLocation(TargetRelativeLocation);
		return;
	}

	EyeHeightBlendStart = FirstPersonMesh->GetRelativeLocation();
	EyeHeightBlendTarget = TargetRelativeLocation;
	EyeHeightBlendElapsed = 0.f;
	EyeHeightBlendDuration = Duration;
	bEyeHeightBlendActive = true;
}

void ABreakingWaveCharacter::PlayBodyAnimCompressed(UAnimSequence* Anim, float Duration)
{
	GetMesh()->PlayAnimation(Anim, false);
	if (UAnimSingleNodeInstance* SingleNode = GetMesh()->GetSingleNodeInstance())
	{
		SingleNode->SetPlayRate(Anim->GetPlayLength() / FMath::Max(Duration, 0.01f));
	}
}

void ABreakingWaveCharacter::BeginProneBodyIdle()
{
	if (IsProne() && ProneBodyIdleAnim)
	{
		GetMesh()->PlayAnimation(ProneBodyIdleAnim, true);
	}
}

void ABreakingWaveCharacter::RestoreStandingBodyAnim()
{
	if (!IsProne())
	{
		GetMesh()->SetAnimInstanceClass(StandingBodyAnimClass);
	}
}

void ABreakingWaveCharacter::BeginSlideFromMomentum()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement->Velocity.Size2D() <= SlideSettleSpeed)
	{
		return;
	}

	bPreSlideUseSeparateBrakingFriction = Movement->bUseSeparateBrakingFriction;
	PreSlideBrakingFriction = Movement->BrakingFriction;
	PreSlideBrakingDeceleration = Movement->BrakingDecelerationWalking;
	PreSlideBrakingDecelerationFalling = Movement->BrakingDecelerationFalling;

	Movement->bUseSeparateBrakingFriction = true;
	Movement->BrakingFriction = 0.f;
	Movement->BrakingDecelerationWalking = SlideDeceleration;
	Movement->BrakingDecelerationFalling = 0.f;
	bSlideActive = true;

	if (ProneDiveUpwardSpeed > 0.f && Movement->IsMovingOnGround())
	{
		LaunchCharacter(FVector(0.f, 0.f, ProneDiveUpwardSpeed), false, true);
		bDiveEyeDropPending = true;
	}
}

void ABreakingWaveCharacter::SettleSlide()
{
	if (!bSlideActive)
	{
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bUseSeparateBrakingFriction = bPreSlideUseSeparateBrakingFriction;
	Movement->BrakingFriction = PreSlideBrakingFriction;
	Movement->BrakingDecelerationWalking = PreSlideBrakingDeceleration;
	Movement->BrakingDecelerationFalling = PreSlideBrakingDecelerationFalling;
	bSlideActive = false;
}

void ABreakingWaveCharacter::DebugThirdPerson()
{
	bDebugThirdPersonViewActive = !bDebugThirdPersonViewActive;

	DebugThirdPersonSpringArm->TargetArmLength = DebugThirdPersonDistance;
	DebugThirdPersonCamera->SetActive(bDebugThirdPersonViewActive);
	FirstPersonCameraComponent->SetActive(!bDebugThirdPersonViewActive);

	GetMesh()->SetFirstPersonPrimitiveType(bDebugThirdPersonViewActive
		? EFirstPersonPrimitiveType::None
		: EFirstPersonPrimitiveType::WorldSpaceRepresentation);
	GetMesh()->SetOwnerNoSee(!bDebugThirdPersonViewActive);
	FirstPersonMesh->SetOwnerNoSee(bDebugThirdPersonViewActive);
}

void ABreakingWaveCharacter::MGNoDamage()
{
	for (TActorIterator<AMGBunkerManager> It(GetWorld()); It; ++It)
	{
		It->ToggleNoDamage();
		return;
	}
}

void ABreakingWaveCharacter::MGKillCrew()
{
	for (TActorIterator<AMGBunkerManager> It(GetWorld()); It; ++It)
	{
		It->KillGunCrewMember();
		return;
	}
}

void ABreakingWaveCharacter::MGDebug()
{
	for (TActorIterator<AMGBunkerManager> It(GetWorld()); It; ++It)
	{
		It->ToggleDebug();
		return;
	}
}

void ABreakingWaveCharacter::DoSprintStart()
{
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}

void ABreakingWaveCharacter::DoSprintEnd()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ABreakingWaveCharacter::DoFire()
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (bReloading || IsProneTransitionActive() || bTransitionInputLocked || bDead
		|| Now - LastShotTime < RifleProfile.FireIntervalSeconds)
	{
		return;
	}

	if (MagRounds <= 0)
	{
		if (RifleDryClickSound != nullptr)
		{
			UGameplayStatics::PlaySound2D(this, RifleDryClickSound);
		}
		PlayFirstPersonAnim(RifleDryFireAnim);
		LastShotTime = Now;
		return;
	}

	LastShotTime = Now;
	--MagRounds;

	const FVector CameraLocation = FirstPersonCameraComponent->GetComponentLocation();
	const FVector CameraForward = FirstPersonCameraComponent->GetForwardVector();
	const float SpreadDeg = bAiming ? RifleProfile.AimSpreadDeg : RifleProfile.HipSpreadDeg;
	const FVector ShotDir = FMath::VRandCone(CameraForward, FMath::DegreesToRadians(SpreadDeg));

	if (AMGBunkerManager* Manager = GetBunkerManager())
	{
		Manager->SpawnBullet(EMGBulletSource::PlayerRifle, 0,
			CameraLocation + CameraForward * MuzzleSpawnForwardOffset, ShotDir * RifleProfile.MuzzleVelocity);
	}

	if (RifleShotSound != nullptr)
	{
		UGameplayStatics::PlaySound2D(this, RifleShotSound, 1.f, FMath::FRandRange(0.96f, 1.04f));
	}
	PlayFirstPersonAnim(RifleFireAnim);
}

void ABreakingWaveCharacter::DoAimStart()
{
	bAiming = true;
}

void ABreakingWaveCharacter::DoAimEnd()
{
	bAiming = false;
}

void ABreakingWaveCharacter::DoReload()
{
	if (bReloading || bTransitionInputLocked || bDead || MagRounds >= RifleProfile.MagazineSize)
	{
		return;
	}
	bReloading = true;
	if (RifleReloadSound != nullptr)
	{
		UGameplayStatics::PlaySound2D(this, RifleReloadSound);
	}
	PlayFirstPersonAnim(RifleReloadAnim);
	GetWorldTimerManager().SetTimer(ReloadTimer, this, &ABreakingWaveCharacter::FinishReload, RifleProfile.ReloadSeconds, false);
}

void ABreakingWaveCharacter::FinishReload()
{
	MagRounds = RifleProfile.MagazineSize;
	bReloading = false;
}

void ABreakingWaveCharacter::PlayFirstPersonAnim(UAnimSequence* Anim)
{
	if (Anim == nullptr)
	{
		return;
	}
	if (UAnimInstance* AnimInstance = FirstPersonMesh->GetAnimInstance())
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(Anim, FName("DefaultSlot"), 0.05f, 0.08f);
	}
}

bool ABreakingWaveCharacter::TraceBody(const FVector& Start, const FVector& End, FHitResult& OutHit) const
{
	if (bDead)
	{
		return false;
	}

	const USkeletalMeshComponent* Body = GetMesh();
	const FBox BroadPhase = Body->Bounds.GetBox().ExpandBy(BodyTraceBoundsPadding);
	if (!FMath::LineBoxIntersection(BroadPhase, Start, End, End - Start))
	{
		return false;
	}

	if (bBodyTraceUnavailable)
	{
		const UCapsuleComponent* Capsule = GetCapsuleComponent();
		const FVector Center = Capsule->GetComponentLocation();
		const float HalfHeight = FMath::Max(
			Capsule->GetScaledCapsuleHalfHeight() - Capsule->GetScaledCapsuleRadius(), 1.f);
		FVector OnSegment, OnAxis;
		FMath::SegmentDistToSegmentSafe(Start, End,
			Center - FVector(0.f, 0.f, HalfHeight), Center + FVector(0.f, 0.f, HalfHeight), OnSegment, OnAxis);
		if (FVector::Dist(OnSegment, OnAxis) >= Capsule->GetScaledCapsuleRadius())
		{
			return false;
		}
		OutHit = FHitResult();
		OutHit.ImpactPoint = OnSegment;
		OutHit.Location = OnSegment;
		return true;
	}

	return const_cast<USkeletalMeshComponent*>(Body)->LineTraceComponent(
		OutHit, Start, End, FCollisionQueryParams(SCENE_QUERY_STAT(BulletVsBody), true));
}

bool ABreakingWaveCharacter::IsHeadBone(FName BoneName) const
{
	return HeadBones.Contains(BoneName);
}

EPlayerHitOutcome ABreakingWaveCharacter::TakeBulletHit(const FVector& HitPoint, const FVector& ShotDirection, bool bHeadshot)
{
	LastHitDirection = ShotDirection.GetSafeNormal();
	LastHitTime = GetWorld()->GetTimeSeconds();

	if (PainSound != nullptr)
	{
		UGameplayStatics::PlaySound2D(this, PainSound, 1.f, FMath::FRandRange(0.94f, 1.06f));
	}

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (PlayerController->PlayerCameraManager != nullptr)
		{
			PlayerController->PlayerCameraManager->StartCameraShake(UHitCameraShake::StaticClass());
		}
	}

	++WoundsTaken;
	if (bHeadshot || WoundsTaken >= WoundsToKill)
	{
		bDead = true;
		return EPlayerHitOutcome::Killed;
	}
	return EPlayerHitOutcome::Wounded;
}

void ABreakingWaveCharacter::BecomeCorpse()
{
	bDead = true;
	bAutoAdvancing = false;

	SetActorTickEnabled(false);
	GetWorldTimerManager().ClearAllTimersForObject(this);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FirstPersonMesh->SetVisibility(false);
	FirstPersonRifleMesh->SetVisibility(false);

	USkeletalMeshComponent* Body = GetMesh();
	Body->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
	Body->SetOwnerNoSee(false);
	Body->SetCollisionProfileName(FName("Ragdoll"));
	Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Body->SetAllBodiesSimulatePhysics(true);
	Body->WakeAllRigidBodies();
}

void ABreakingWaveCharacter::ApplyTakeoverState(float HeadingYaw, bool bStartProne, bool bAdvancing)
{
	MagRounds = FMath::RandRange(FMath::Min(TakeoverMagRoundsMin, RifleProfile.MagazineSize), RifleProfile.MagazineSize);
	bAutoAdvancing = bAdvancing;

	SetActorRotation(FRotator(0.f, HeadingYaw, 0.f));
	if (AController* OwningController = GetController())
	{
		OwningController->SetControlRotation(FRotator(0.f, HeadingYaw, 0.f));
	}

	if (bStartProne)
	{
		Crouch();
	}
}

void ABreakingWaveCharacter::SetTransitionInputLocked(bool bLocked)
{
	bTransitionInputLocked = bLocked;
	if (!bLocked)
	{
		bAutoAdvancing = false;
	}
}

AMGBunkerManager* ABreakingWaveCharacter::GetBunkerManager()
{
	if (!CachedBunkerManager.IsValid())
	{
		for (TActorIterator<AMGBunkerManager> It(GetWorld()); It; ++It)
		{
			CachedBunkerManager = *It;
			break;
		}
	}
	return CachedBunkerManager.Get();
}
