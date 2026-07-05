// Copyright Epic Games, Inc. All Rights Reserved.

#include "BreakingWaveCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
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
	GetCharacterMovement()->MaxWalkSpeedCrouched = ProneSpeed;
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
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
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

	const float GroundZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float EyeDropToProne = FirstPersonCameraComponent->GetComponentLocation().Z - (GroundZ + ProneEyeHeight);
	FirstPersonMesh->SetRelativeLocation(StandingFirstPersonMeshRelativeLocation - FVector(0.f, 0.f, EyeDropToProne));
	FirstPersonMesh->SetVisibility(false);

	if (ProneBodyIdleAnim)
	{
		GetMesh()->PlayAnimation(ProneBodyIdleAnim, true);
	}
}

void ABreakingWaveCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	FirstPersonMesh->SetRelativeLocation(StandingFirstPersonMeshRelativeLocation);
	FirstPersonMesh->SetVisibility(true);

	if (ProneBodyIdleAnim)
	{
		GetMesh()->SetAnimInstanceClass(StandingBodyAnimClass);
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
