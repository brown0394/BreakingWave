// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "HeadbobCameraShake.generated.h"

/**
 *  Tuning knobs for the handheld-camera motion (07_CAMERA.md headbob section).
 *  Lives on BreakingWaveCharacter next to the movement knobs; all values tentative until feel-checked.
 */
USTRUCT(BlueprintType)
struct FHeadbobSettings
{
	GENERATED_BODY()

	/** Vertical bob amplitude (cm) at walk speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WalkAmplitude = 1.5f;

	/** Vertical bob amplitude (cm) at sprint speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SprintAmplitude = 2.5f;

	/** Footfall rate (Hz) at walk speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WalkFrequency = 2.8f;

	/** Footfall rate (Hz) at sprint speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SprintFrequency = 3.4f;

	/** Side-to-side sway amplitude as a fraction of the vertical amplitude, swinging at half the footfall rate; 0 = vertical-only (the motion-sickness fallback) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LateralRatio = 0.4f;

	/** Vertical amplitude (cm) of the faint breathing motion while stationary */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BreathingAmplitude = 0.3f;

	/** Breathing rate (Hz) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BreathingFrequency = 0.35f;

	/** Breathing amplitude multiplier while prone ("near-zero headbob while prone") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ProneBreathingScale = 0.4f;

	/** Seconds for amplitudes to ease when the movement state changes, so stopping or dropping prone never pops the camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SmoothingTime = 0.2f;
};

/**
 *  Speed-synced headbob: vertical sine at footfall rate plus half-rate lateral sway,
 *  fading to faint breathing when stationary and to stillness while airborne, sliding,
 *  or mid prone-transition (those states own the camera motion themselves).
 *  Reads the view-target BreakingWaveCharacter every update, so one instance keeps
 *  working across possession changes.
 */
UCLASS()
class UHeadbobShakePattern : public UCameraShakePattern
{
	GENERATED_BODY()

private:

	virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const override;

	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;

	virtual bool IsFinishedImpl() const override { return false; }

	const class ABreakingWaveCharacter* ResolveViewTargetCharacter() const;

	float BobPhaseRadians = 0.f;

	float BreathingPhaseRadians = 0.f;

	float CurrentBobAmplitude = 0.f;

	float CurrentBreathingAmplitude = 0.f;
};

/**
 *  The camera shake wrapper the character starts on possession; single-instance so
 *  repossession never stacks a second bob.
 */
UCLASS()
class UHeadbobCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:

	UHeadbobCameraShake(const FObjectInitializer& ObjectInitializer);
};
