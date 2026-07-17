// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeadbobCameraShake.h"
#include "BreakingWaveCharacter.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	float MapGroundSpeed(float Speed, float WalkSpeed, float RunSpeed, float WalkValue, float RunValue)
	{
		if (Speed <= WalkSpeed)
		{
			return WalkValue * (Speed / FMath::Max(WalkSpeed, 1.f));
		}
		const float Alpha = FMath::Clamp((Speed - WalkSpeed) / FMath::Max(RunSpeed - WalkSpeed, 1.f), 0.f, 1.f);
		return FMath::Lerp(WalkValue, RunValue, Alpha);
	}

	float EaseAmplitude(float Current, float Target, float DeltaTime, float SmoothingTime)
	{
		if (SmoothingTime <= 0.f)
		{
			return Target;
		}
		return FMath::FInterpTo(Current, Target, DeltaTime, 1.f / SmoothingTime);
	}
}

void UHeadbobShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
	OutInfo.Duration = FCameraShakeDuration::Infinite();
}

const ABreakingWaveCharacter* UHeadbobShakePattern::ResolveViewTargetCharacter() const
{
	const UCameraShakeBase* Shake = GetShakeInstance();
	const APlayerCameraManager* CameraManager = Shake ? Shake->GetCameraManager() : nullptr;
	return CameraManager ? Cast<ABreakingWaveCharacter>(CameraManager->GetViewTargetPawn()) : nullptr;
}

void UHeadbobShakePattern::UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult)
{
	float TargetBobAmplitude = 0.f;
	float TargetBreathingAmplitude = 0.f;
	float BobFrequency = 0.f;
	float LateralRatio = 0.f;
	float BreathingFrequency = 0.f;
	float SmoothingTime = 0.f;

	if (const ABreakingWaveCharacter* Character = ResolveViewTargetCharacter())
	{
		const FHeadbobSettings& Settings = Character->GetHeadbobSettings();
		LateralRatio = Settings.LateralRatio;
		BreathingFrequency = Settings.BreathingFrequency;
		SmoothingTime = Settings.SmoothingTime;

		const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		const bool bCameraOwnedElsewhere = !Movement->IsMovingOnGround()
			|| Character->IsSliding()
			|| Character->IsProneTransitionActive()
			|| Character->IsDebugThirdPersonActive();

		if (!bCameraOwnedElsewhere)
		{
			if (Character->IsProne())
			{
				TargetBreathingAmplitude = Settings.BreathingAmplitude * Settings.ProneBreathingScale;
			}
			else
			{
				const float GroundSpeed = Movement->Velocity.Size2D();
				TargetBobAmplitude = MapGroundSpeed(GroundSpeed, Character->GetWalkSpeed(), Character->GetRunSpeed(), Settings.WalkAmplitude, Settings.SprintAmplitude);
				BobFrequency = MapGroundSpeed(GroundSpeed, Character->GetWalkSpeed(), Character->GetRunSpeed(), Settings.WalkFrequency, Settings.SprintFrequency);
				TargetBreathingAmplitude = Settings.BreathingAmplitude;
			}
		}
	}

	CurrentBobAmplitude = EaseAmplitude(CurrentBobAmplitude, TargetBobAmplitude, Params.DeltaTime, SmoothingTime);
	CurrentBreathingAmplitude = EaseAmplitude(CurrentBreathingAmplitude, TargetBreathingAmplitude, Params.DeltaTime, SmoothingTime);

	BobPhaseRadians = FMath::Fmod(BobPhaseRadians + UE_TWO_PI * BobFrequency * Params.DeltaTime, UE_TWO_PI * 2.f);
	BreathingPhaseRadians = FMath::Fmod(BreathingPhaseRadians + UE_TWO_PI * BreathingFrequency * Params.DeltaTime, UE_TWO_PI);

	const float VerticalOffset = CurrentBobAmplitude * FMath::Sin(BobPhaseRadians)
		+ CurrentBreathingAmplitude * FMath::Sin(BreathingPhaseRadians);
	const float LateralOffset = CurrentBobAmplitude * LateralRatio * FMath::Sin(BobPhaseRadians * 0.5f);

	OutResult.Location = FVector(0.f, LateralOffset, VerticalOffset);
}

UHeadbobCameraShake::UHeadbobCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UHeadbobShakePattern>(TEXT("RootShakePattern")))
{
	bSingleInstance = true;
}
