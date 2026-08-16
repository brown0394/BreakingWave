#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "HitCameraShake.generated.h"

/**
 *  The jolt of being hit (07_CAMERA.md hit camera): a decaying rattle plus a push along the
 *  round's direction of travel, so a hit from the left throws the view right. Knobs live in
 *  FHitShakeSettings on BreakingWaveCharacter; the direction is read once when the shake starts.
 */
UCLASS()
class UHitShakePattern : public UCameraShakePattern
{
	GENERATED_BODY()

private:

	virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const override;

	virtual void StartShakePatternImpl(const FCameraShakePatternStartParams& Params) override;

	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;

	virtual bool IsFinishedImpl() const override;

	const class ABreakingWaveCharacter* ResolveViewTargetCharacter() const;

	float Elapsed = 0.f;

	float Duration = 0.35f;

	float RattleAmplitudeDeg = 2.f;

	float RattleFrequency = 24.f;

	float DirectionalPushDeg = 4.f;

	/** Push in camera space, resolved at start: X = pitch down, Y = yaw right */
	FVector2D PushAxis = FVector2D::ZeroVector;

	float PhaseOffset = 0.f;
};

UCLASS()
class UHitCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:

	UHitCameraShake(const FObjectInitializer& ObjectInitializer);
};
