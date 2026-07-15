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
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "TimerManager.h"
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
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ABreakingWaveCharacter::DoMove(float Right, float Forward)
{
	if (GetController() && !IsProne() && !IsProneTransitionActive())
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

void ABreakingWaveCharacter::DoJumpStart()
{
}

void ABreakingWaveCharacter::DoJumpEnd()
{
}

void ABreakingWaveCharacter::DoProneToggle()
{
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

void ABreakingWaveCharacter::DoSprintStart()
{
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}

void ABreakingWaveCharacter::DoSprintEnd()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}
