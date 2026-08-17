// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BreakingWavePlayerController.generated.h"

class ACameraActor;
class AAllySimManager;
class UInputMappingContext;
class UUserWidget;

/**
 *  The death -> takeover loop (07_CAMERA.md §2 and §4, Decision 040). The narrative screen is a
 *  state of zero duration between FadeOut and the takeover; the written half of Step 5 drops into
 *  that seam without reshaping anything around it.
 */
UENUM()
enum class ETransitionPhase : uint8
{
	None,
	DeathShake,
	DeathDescend,
	DeathHold,
	FadeOut,
	FadeIn,
};

/** ALL NUMBERS TENTATIVE — 07_CAMERA.md's spec durations, kept as knobs because the loop fires every 30-60 s */
USTRUCT(BlueprintType)
struct FTransitionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Death")
	float DeathShakeSeconds = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Death")
	float DeathDescendSeconds = 1.2f;

	/** The final view holds here — sky, sand, someone's boot */
	UPROPERTY(EditAnywhere, Category = "Death")
	float DeathHoldSeconds = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Death")
	float FadeOutSeconds = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Death")
	float DeathShakeAmplitudeDeg = 3.5f;

	UPROPERTY(EditAnywhere, Category = "Death")
	float DeathShakeFrequency = 17.f;

	/** Camera settles this far above the ground — a face resting on the sand */
	UPROPERTY(EditAnywhere, Category = "Death")
	float GroundClearance = 18.f;

	/** Roll at rest; the fall goes sideways out of a forward run */
	UPROPERTY(EditAnywhere, Category = "Death")
	float TiltDegrees = 22.f;

	/** Pitch the settled view carries, so the last frame is ground and fog rather than empty sky */
	UPROPERTY(EditAnywhere, Category = "Death")
	float SettledPitchDegrees = -12.f;

	UPROPERTY(EditAnywhere, Category = "Transition")
	float FadeInSeconds = 1.25f;

	/** Enemies stay unable to acquire the new man until this long AFTER control returns (07_CAMERA.md §4) */
	UPROPERTY(EditAnywhere, Category = "Transition")
	float TargetingDelaySeconds = 1.5f;

	/** Bodies kept on the beach; the oldest is removed past this. The real corpse system is Step 6 */
	UPROPERTY(EditAnywhere, Category = "Transition")
	int32 MaxCorpses = 8;
};

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context and owns the death -> takeover transition.
 */
UCLASS(abstract)
class BREAKINGWAVE_API ABreakingWavePlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	/** Constructor */
	ABreakingWavePlayerController();

	virtual void PlayerTick(float DeltaTime) override;

	/** Entry point from the bullet loop once a round has killed the man you were */
	void BeginDeathTransition();

	bool IsTransitionActive() const { return Phase != ETransitionPhase::None; }

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY(EditAnywhere, Category="Transition", meta = (ShowOnlyInnerProperties))
	FTransitionSettings TransitionSettings;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

private:

	void EnterPhase(ETransitionPhase NextPhase);

	void UpdateDeathCamera(float DeltaTime, float PhaseAlpha);

	/** Runs at the very end of the fade, so the man taken over is alive at the instant of takeover */
	bool TryTakeover();

	void RetireOldestCorpse();

	AAllySimManager* FindAllySim() const;

	float PhaseDuration(ETransitionPhase Query) const;

	ETransitionPhase Phase = ETransitionPhase::None;

	float PhaseElapsed = 0.f;

	UPROPERTY(Transient)
	ACameraActor* DeathCamera = nullptr;

	UPROPERTY(Transient)
	TArray<APawn*> Corpses;

	TSubclassOf<APawn> TakeoverPawnClass;

	/** Stamped at the instant of death and never moved; the takeover disc is measured from it */
	FVector DeathAnchor = FVector::ZeroVector;

	FVector DeathCameraStartLocation = FVector::ZeroVector;

	FVector DeathCameraEndLocation = FVector::ZeroVector;

	FRotator DeathCameraStartRotation = FRotator::ZeroRotator;

	FRotator DeathCameraEndRotation = FRotator::ZeroRotator;

	float DeathShakePhase = 0.f;

	bool bTakeoverBlockedWarned = false;
};
