#include "HitCameraShake.h"
#include "BreakingWaveCharacter.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"

void UHitShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
	OutInfo.Duration = FCameraShakeDuration::Custom();
}

const ABreakingWaveCharacter* UHitShakePattern::ResolveViewTargetCharacter() const
{
	const UCameraShakeBase* Shake = GetShakeInstance();
	const APlayerCameraManager* CameraManager = Shake ? Shake->GetCameraManager() : nullptr;
	return CameraManager ? Cast<ABreakingWaveCharacter>(CameraManager->GetViewTargetPawn()) : nullptr;
}

void UHitShakePattern::StartShakePatternImpl(const FCameraShakePatternStartParams& Params)
{
	Elapsed = 0.f;
	PhaseOffset = FMath::FRandRange(0.f, UE_TWO_PI);
	PushAxis = FVector2D::ZeroVector;

	const ABreakingWaveCharacter* Character = ResolveViewTargetCharacter();
	if (Character == nullptr)
	{
		return;
	}

	const FHitShakeSettings& Settings = Character->GetHitShakeSettings();
	Duration = Settings.Duration;
	RattleAmplitudeDeg = Settings.RattleAmplitudeDeg;
	RattleFrequency = Settings.RattleFrequency;
	DirectionalPushDeg = Settings.DirectionalPushDeg;

	const UCameraComponent* Camera = Character->GetFirstPersonCameraComponent();
	const FVector ShotDirection = Character->GetLastHitDirection();
	PushAxis = FVector2D(
		FVector::DotProduct(ShotDirection, Camera->GetUpVector()),
		FVector::DotProduct(ShotDirection, Camera->GetRightVector()));
}

void UHitShakePattern::UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult)
{
	Elapsed += Params.DeltaTime;

	const float Remaining = FMath::Clamp(1.f - Elapsed / FMath::Max(Duration, 0.01f), 0.f, 1.f);
	const float Rattle = FMath::Sin(PhaseOffset + UE_TWO_PI * RattleFrequency * Elapsed)
		* RattleAmplitudeDeg * Remaining * Remaining;

	OutResult.Rotation = FRotator(
		PushAxis.X * DirectionalPushDeg * Remaining + Rattle,
		PushAxis.Y * DirectionalPushDeg * Remaining,
		Rattle * 0.5f);
}

bool UHitShakePattern::IsFinishedImpl() const
{
	return Elapsed >= Duration;
}

UHitCameraShake::UHitCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UHitShakePattern>(TEXT("RootShakePattern")))
{
	bSingleInstance = true;
}
