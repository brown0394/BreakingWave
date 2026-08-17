// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Character.h"
#include "HeadbobCameraShake.h"
#include "Logging/LogMacros.h"
#include "RifleProfile.h"
#include "BreakingWaveCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UAnimSequence;
class USoundBase;
class AMGBunkerManager;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/** 06_COMBAT.md damage model: head is instant, everything else wounds, the second wound kills */
UENUM()
enum class EPlayerHitOutcome : uint8
{
	Wounded,
	Killed,
};

/** The "I've been hit" jolt — the only wounded feedback in this pass; the persistent presentation is deferred */
USTRUCT(BlueprintType)
struct FHitShakeSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Duration = 0.35f;

	/** Rotational rattle, decaying over the duration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RattleAmplitudeDeg = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RattleFrequency = 24.f;

	/** Push away from the shot's direction of travel — hit from the left throws the view right (07_CAMERA.md) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DirectionalPushDeg = 4.f;
};

/** How the body lands. A dead man drops; he does not sail on at sprint speed */
USTRUCT(BlueprintType)
struct FCorpseSettings
{
	GENERATED_BODY()

	/** Fraction of the running speed the ragdoll keeps. The physics bodies otherwise inherit the full
	 *  kinematic velocity of the sprint (900 uu/s) and throw the corpse metres down the beach */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MomentumRetained = 0.2f;

	/** Bleeds off the remaining slide so the body settles where it fell instead of skating */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LinearDamping = 0.75f;

	/** Kills the weightless windmilling that reads as a rag rather than a body */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AngularDamping = 4.f;

	/** Downward kick at the moment of death — the collapse starts immediately instead of drifting into it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DropSpeed = 150.f;
};

/**
 *  A basic first person character
 */
UCLASS(abstract)
class ABreakingWaveCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** Rifle in the FP arms' grip socket; visual only, bullets spawn from the camera */
	UPROPERTY(VisibleAnywhere, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonRifleMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Debug-only orbit arm for eyeballing body animations in PIE; the game itself is first-person only */
	UPROPERTY(VisibleAnywhere, Category="Debug", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* DebugThirdPersonSpringArm;

	/** Debug-only camera on the orbit arm, activated by the DebugThirdPerson console command */
	UPROPERTY(VisibleAnywhere, Category="Debug", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* DebugThirdPersonCamera;

protected:

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* SprintAction;

	/** Prone Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* ProneAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* FireAction;

	/** Aim (ADS) Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* AimAction;

	/** Reload Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* ReloadAction;

public:
	ABreakingWaveCharacter();

protected:

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles prone toggle inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoProneToggle();

	/** Handles sprint start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintStart();

	/** Handles sprint end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintEnd();

	/** Semi-auto: one round per trigger pull; dry click when the mag runs empty */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoFire();

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAimStart();

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAimEnd();

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoReload();

	/** Movement speed while walking (default) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float WalkSpeed = 600.f;

	/** Movement speed while sprinting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float RunSpeed = 900.f;

	/** Collision capsule half height while prone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float ProneCapsuleHalfHeight = 40.f;

	/** Camera height above the ground while prone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float ProneEyeHeight = 30.f;

	/** Time the camera and body take to drop into prone (fast — an emergency dive, 06_COMBAT.md) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float ProneDropDuration = 0.35f;

	/** Time the camera and body take to rise back to standing (slower — standing up is costly) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float ProneStandUpDuration = 0.9f;

	/** Ground deceleration after a dive lands (slide-into-prone, 06_COMBAT.md); high = short impact skid, not a long glide */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float SlideDeceleration = 2500.f;

	/** Speed below which a slide settles into the stationary prone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float SlideSettleSpeed = 60.f;

	/** Upward pop applied on prone entry at speed, turning the drop into a ballistic dive (~0.6 s airtime, ~46 cm rise at 300; 0 = flat slide) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float ProneDiveUpwardSpeed = 300.f;

	/** Gravity multiplier past the dive's apex — the rise keeps its pop, the fall snaps down (1 = symmetric arc) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float ProneDiveFallGravityScale = 2.f;

	/** The attacker's semi-auto rifle: shot per trigger pull, ~8-round mag (06_COMBAT.md firing) */
	UPROPERTY(EditAnywhere, Category="Rifle")
	FRifleProfile RifleProfile;

	/** Aimed fire narrows the view; hip fire keeps the full field (no crosshair either way) */
	UPROPERTY(EditAnywhere, Category="Rifle")
	float AimFieldOfView = 55.f;

	UPROPERTY(EditAnywhere, Category="Rifle")
	float AimFovBlendSpeed = 12.f;

	/** Bullets spawn this far ahead of the camera so the tracer never clips the near plane */
	UPROPERTY(EditAnywhere, Category="Rifle")
	float MuzzleSpawnForwardOffset = 40.f;

	UPROPERTY(EditAnywhere, Category="Rifle|Anim")
	UAnimSequence* RifleFireAnim;

	UPROPERTY(EditAnywhere, Category="Rifle|Anim")
	UAnimSequence* RifleReloadAnim;

	UPROPERTY(EditAnywhere, Category="Rifle|Anim")
	UAnimSequence* RifleDryFireAnim;

	UPROPERTY(EditAnywhere, Category="Rifle|Audio")
	USoundBase* RifleShotSound;

	UPROPERTY(EditAnywhere, Category="Rifle|Audio")
	USoundBase* RifleDryClickSound;

	UPROPERTY(EditAnywhere, Category="Rifle|Audio")
	USoundBase* RifleReloadSound;

	/** Handheld-camera bob/breathing knobs (07_CAMERA.md); applied by UHeadbobShakePattern, started on possession */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	FHeadbobSettings HeadbobSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	FHitShakeSettings HitShakeSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	FCorpseSettings CorpseSettings;

	/** Torso hits needed to kill; the head bypasses the count entirely (06_COMBAT.md) */
	UPROPERTY(EditAnywhere, Category="Damage")
	int32 WoundsToKill = 2;

	/** Physics-asset bodies that count as a head hit. Limb tiering stays an open question (06_COMBAT.md) */
	UPROPERTY(EditAnywhere, Category="Damage")
	TArray<FName> HeadBones = { FName("head") };

	/** Slack on the animated mesh bounds used as the bullet broadphase */
	UPROPERTY(EditAnywhere, Category="Damage")
	float BodyTraceBoundsPadding = 20.f;

	UPROPERTY(EditAnywhere, Category="Damage")
	USoundBase* PainSound;

	/** Rounds in the magazine of the man you take over — never a full mag, so the dry click stays unpredictable */
	UPROPERTY(EditAnywhere, Category="Rifle")
	int32 TakeoverMagRoundsMin = 3;

	/** Orbit distance of the debug third person camera */
	UPROPERTY(EditAnywhere, Category="Debug")
	float DebugThirdPersonDistance = 400.f;

	/** Looping full-body pose the world-space mesh plays while prone (body/shadow only; the ABP takes over again on stand-up) */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimSequence* ProneBodyIdleAnim;

	/** One-shot the body plays going down, time-compressed to ProneDropDuration */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimSequence* StandToProneAnim;

	/** One-shot the body plays getting up, time-compressed to ProneStandUpDuration */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimSequence* ProneToStandAnim;

protected:

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual void Landed(const FHitResult& Hit) override;

	virtual void NotifyControllerChanged() override;

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

private:

	void FinishReload();

	void SetUpBodyHitVolume();

	void PlayFirstPersonAnim(UAnimSequence* Anim);

	AMGBunkerManager* GetBunkerManager();

	void StartEyeHeightBlend(const FVector& TargetRelativeLocation, float Duration);

	void StartEyeDropToProne();

	float PredictDiveFlightTime() const;

	void RestoreDiveFallGravity();

	void PlayBodyAnimCompressed(UAnimSequence* Anim, float Duration);

	void BeginProneBodyIdle();

	void RestoreStandingBodyAnim();

	void BeginSlideFromMomentum();

	void SettleSlide();

	FVector StandingFirstPersonMeshRelativeLocation = FVector::ZeroVector;

	TSubclassOf<UAnimInstance> StandingBodyAnimClass;

	int32 MagRounds = 0;

	bool bAiming = false;

	bool bReloading = false;

	float LastShotTime = -100.f;

	float DefaultFieldOfView = 90.f;

	FTimerHandle ReloadTimer;

	TWeakObjectPtr<AMGBunkerManager> CachedBunkerManager;

	bool bDebugThirdPersonViewActive = false;

	bool bEyeHeightBlendActive = false;

	FVector EyeHeightBlendStart = FVector::ZeroVector;

	FVector EyeHeightBlendTarget = FVector::ZeroVector;

	float EyeHeightBlendElapsed = 0.f;

	float EyeHeightBlendDuration = 0.f;

	float ProneTransitionEndTime = 0.f;

	FTimerHandle ProneBodyAnimTimer;

	bool bSlideActive = false;

	bool bDiveEyeDropPending = false;

	bool bDiveFallGravityActive = false;

	float PreDiveGravityScale = 1.f;

	bool bPreSlideUseSeparateBrakingFriction = false;

	float PreSlideBrakingFriction = 0.f;

	float PreSlideBrakingDeceleration = 0.f;

	float PreSlideBrakingDecelerationFalling = 0.f;

	int32 WoundsTaken = 0;

	bool bDead = false;

	bool bTransitionInputLocked = false;

	bool bAutoAdvancing = false;

	/** Set when the mesh has no usable physics bodies, so bullets fall back to the capsule instead of passing through a player who can never be hit */
	bool bBodyTraceUnavailable = false;

	FVector LastHitDirection = FVector::ForwardVector;

	float LastHitTime = -1000.f;

public:

	/** Console command: toggles the debug third person view (shows the world-space body mesh, hides the FP arms) */
	UFUNCTION(Exec)
	void DebugThirdPerson();

	/** Console command: toggles MG bullet damage to the player off/on (observation mode) */
	UFUNCTION(Exec)
	void MGNoDamage();

	/** Console command: kills the man on the gun of the first manned bunker (tests takeover windows and crew degradation) */
	UFUNCTION(Exec)
	void MGKillCrew();

	/** Console command: toggles the MG/ally-sim debug readout (F7) */
	UFUNCTION(Exec)
	void MGDebug();

	/** Prone is implemented on the engine crouch machinery; crouched means prone */
	bool IsProne() const { return bIsCrouched; }

	/** True while dropping into or rising out of prone; movement input is ignored during it */
	bool IsProneTransitionActive() const;

	/** True while prone momentum is still carrying the character along the ground */
	bool IsSliding() const { return bSlideActive; }

	/** True while aiming down sights; movement is locked and the view narrows */
	bool IsAiming() const { return bAiming; }

	/** True while the F6 debug orbit camera is active; headbob stays out of the debug view */
	bool IsDebugThirdPersonActive() const { return bDebugThirdPersonViewActive; }

	const FHeadbobSettings& GetHeadbobSettings() const { return HeadbobSettings; }

	const FHitShakeSettings& GetHitShakeSettings() const { return HitShakeSettings; }

	/** Direction the round was travelling when it landed; the hit shake pushes the view along it */
	const FVector& GetLastHitDirection() const { return LastHitDirection; }

	/** Bullet-vs-body test against the animated physics asset — the capsule is not a combat volume (Decision 040) */
	bool TraceBody(const FVector& Start, const FVector& End, FHitResult& OutHit) const;

	bool IsHeadBone(FName BoneName) const;

	EPlayerHitOutcome TakeBulletHit(const FVector& HitPoint, const FVector& ShotDirection, bool bHeadshot);

	bool IsDead() const { return bDead; }

	/** Hands the pawn over to the death camera: unhides the world-space body, ragdolls it, stops driving anything */
	void BecomeCorpse();

	/** Opens this life in the state the simulated ally was already in (07_CAMERA.md starting state) */
	void ApplyTakeoverState(float HeadingYaw, bool bStartProne, bool bAdvancing);

	/** Locked through the fade-in: you watch what this man was doing before you take the reins */
	void SetTransitionInputLocked(bool bLocked);

	float GetWalkSpeed() const { return WalkSpeed; }

	float GetRunSpeed() const { return RunSpeed; }

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

