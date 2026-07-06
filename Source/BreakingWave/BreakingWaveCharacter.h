// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "BreakingWaveCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UAnimSequence;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

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

	/** Empty shim: the template touch UI in BP_FirstPersonCharacter still calls this, but jumping is disabled by design (06_COMBAT.md). Delete the BP's jump nodes, then delete this. */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Empty shim: see DoJumpStart */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Handles sprint start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintStart();

	/** Handles sprint end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintEnd();

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

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

private:

	void StartEyeHeightBlend(const FVector& TargetRelativeLocation, float Duration);

	void PlayBodyAnimCompressed(UAnimSequence* Anim, float Duration);

	void BeginProneBodyIdle();

	void RestoreStandingBodyAnim();

	FVector StandingFirstPersonMeshRelativeLocation = FVector::ZeroVector;

	TSubclassOf<UAnimInstance> StandingBodyAnimClass;

	bool bDebugThirdPersonViewActive = false;

	bool bEyeHeightBlendActive = false;

	FVector EyeHeightBlendStart = FVector::ZeroVector;

	FVector EyeHeightBlendTarget = FVector::ZeroVector;

	float EyeHeightBlendElapsed = 0.f;

	float EyeHeightBlendDuration = 0.f;

	float ProneTransitionEndTime = 0.f;

	FTimerHandle ProneBodyAnimTimer;

public:

	/** Console command: toggles the debug third person view (shows the world-space body mesh, hides the FP arms) */
	UFUNCTION(Exec)
	void DebugThirdPerson();

	/** Prone is implemented on the engine crouch machinery; crouched means prone */
	bool IsProne() const { return bIsCrouched; }

	/** True while dropping into or rising out of prone; movement input is ignored during it */
	bool IsProneTransitionActive() const;

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

