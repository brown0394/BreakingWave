// Copyright Epic Games, Inc. All Rights Reserved.


#include "BreakingWavePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputMappingContext.h"
#include "BeachAllySim.h"
#include "BeachInfantrySystem.h"
#include "BreakingWaveCameraManager.h"
#include "BreakingWaveCharacter.h"
#include "Blueprint/UserWidget.h"
#include "BreakingWave.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "MGBunkerSystem.h"
#include "Widgets/Input/SVirtualJoystick.h"

ABreakingWavePlayerController::ABreakingWavePlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = ABreakingWaveCameraManager::StaticClass();
}

void ABreakingWavePlayerController::BeginPlay()
{
	Super::BeginPlay();

	
	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogBreakingWave, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void ABreakingWavePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (Phase == ETransitionPhase::None)
	{
		return;
	}

	PhaseElapsed += DeltaTime;
	const float Duration = PhaseDuration(Phase);
	const float Alpha = Duration > 0.f ? FMath::Clamp(PhaseElapsed / Duration, 0.f, 1.f) : 1.f;

	if (Phase != ETransitionPhase::FadeIn)
	{
		UpdateDeathCamera(DeltaTime, Alpha);
	}

	if (PhaseElapsed < Duration)
	{
		return;
	}

	switch (Phase)
	{
	case ETransitionPhase::DeathShake:
		EnterPhase(ETransitionPhase::DeathDescend);
		break;

	case ETransitionPhase::DeathDescend:
		EnterPhase(ETransitionPhase::DeathHold);
		break;

	case ETransitionPhase::DeathHold:
		EnterPhase(ETransitionPhase::FadeOut);
		break;

	case ETransitionPhase::FadeOut:
		if (TryTakeover())
		{
			EnterPhase(ETransitionPhase::FadeIn);
		}
		break;

	case ETransitionPhase::FadeIn:
		if (ABreakingWaveCharacter* NewLife = Cast<ABreakingWaveCharacter>(GetPawn()))
		{
			NewLife->SetTransitionInputLocked(false);
		}
		EnterPhase(ETransitionPhase::None);
		break;

	default:
		break;
	}
}

void ABreakingWavePlayerController::BeginDeathTransition()
{
	ABreakingWaveCharacter* Dying = Cast<ABreakingWaveCharacter>(GetPawn());
	if (Phase != ETransitionPhase::None || Dying == nullptr)
	{
		return;
	}

	TakeoverPawnClass = Dying->GetClass();
	DeathAnchorY = Dying->GetActorLocation().Y;

	const UCameraComponent* Camera = Dying->GetFirstPersonCameraComponent();
	DeathCameraStartLocation = Camera->GetComponentLocation();
	DeathCameraStartRotation = GetControlRotation();

	float GroundZ = DeathCameraStartLocation.Z - 200.f;
	FHitResult Ground;
	const FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(DeathCameraFloor), false, Dying);
	if (GetWorld()->LineTraceSingleByChannel(Ground, DeathCameraStartLocation,
		DeathCameraStartLocation - FVector(0.f, 0.f, 1000.f), ECC_Visibility, GroundParams))
	{
		GroundZ = Ground.ImpactPoint.Z;
	}
	DeathCameraEndLocation = FVector(DeathCameraStartLocation.X, DeathCameraStartLocation.Y,
		FMath::Min(DeathCameraStartLocation.Z, GroundZ + TransitionSettings.GroundClearance));

	const float FallSide = FVector::DotProduct(Dying->GetLastHitDirection(), Camera->GetRightVector()) >= 0.f ? 1.f : -1.f;
	DeathCameraEndRotation = FRotator(TransitionSettings.SettledPitchDegrees,
		DeathCameraStartRotation.Yaw, FallSide * TransitionSettings.TiltDegrees);

	Dying->BecomeCorpse();
	UnPossess();
	Corpses.Add(Dying);
	RetireOldestCorpse();

	DeathCamera = GetWorld()->SpawnActor<ACameraActor>(DeathCameraStartLocation, DeathCameraStartRotation);
	if (DeathCamera != nullptr)
	{
		SetViewTargetWithBlend(DeathCamera, 0.f);
	}

	DeathShakePhase = FMath::FRandRange(0.f, UE_TWO_PI);
	EnterPhase(ETransitionPhase::DeathShake);
}

void ABreakingWavePlayerController::EnterPhase(ETransitionPhase NextPhase)
{
	Phase = NextPhase;
	PhaseElapsed = 0.f;

	if (PlayerCameraManager == nullptr)
	{
		return;
	}

	if (NextPhase == ETransitionPhase::FadeOut)
	{
		PlayerCameraManager->StartCameraFade(0.f, 1.f, TransitionSettings.FadeOutSeconds,
			FLinearColor::Black, true, true);
	}
	else if (NextPhase == ETransitionPhase::FadeIn)
	{
		PlayerCameraManager->StartCameraFade(1.f, 0.f, TransitionSettings.FadeInSeconds,
			FLinearColor::Black, true, false);
	}
}

void ABreakingWavePlayerController::UpdateDeathCamera(float DeltaTime, float PhaseAlpha)
{
	if (DeathCamera == nullptr)
	{
		return;
	}

	FVector Location = DeathCameraEndLocation;
	FRotator Rotation = DeathCameraEndRotation;

	if (Phase == ETransitionPhase::DeathShake)
	{
		Location = DeathCameraStartLocation;
		Rotation = DeathCameraStartRotation;

		DeathShakePhase += UE_TWO_PI * TransitionSettings.DeathShakeFrequency * DeltaTime;
		const float Rattle = FMath::Sin(DeathShakePhase)
			* TransitionSettings.DeathShakeAmplitudeDeg * (1.f - PhaseAlpha);
		Rotation += FRotator(Rattle, Rattle * 0.6f, Rattle * 1.4f);
	}
	else if (Phase == ETransitionPhase::DeathDescend)
	{
		const float Eased = FMath::InterpEaseOut(0.f, 1.f, PhaseAlpha, 2.f);
		Location = FMath::Lerp(DeathCameraStartLocation, DeathCameraEndLocation, Eased);
		Rotation = FMath::Lerp(DeathCameraStartRotation, DeathCameraEndRotation, Eased);
	}

	DeathCamera->SetActorLocationAndRotation(Location, Rotation);
}

bool ABreakingWavePlayerController::TryTakeover()
{
	AAllySimManager* AllySim = FindAllySim();
	if (AllySim == nullptr)
	{
		if (!bTakeoverBlockedWarned)
		{
			bTakeoverBlockedWarned = true;
			UE_LOG(LogBreakingWave, Error,
				TEXT("No AllySimManager in the level — the takeover has nobody to hand you to and the screen will stay black."));
		}
		return false;
	}

	int32 LadderSteps = 0;
	const int32 Slot = AllySim->SelectTakeoverSlot(DeathAnchorY, LadderSteps);
	if (Slot == INDEX_NONE)
	{
		return false;
	}

	const FSimAlly& Ally = AllySim->GetAllies()[Slot];
	const FVector GroundPosition = Ally.Position;
	const float HeadingYaw = Ally.HeadingYaw;
	const bool bStartProne = Ally.Stance == ESimAllyStance::Prone;
	AllySim->KillAlly(Slot);

	const ACharacter* PawnDefaults = TakeoverPawnClass->GetDefaultObject<ACharacter>();
	const float HalfHeight = PawnDefaults != nullptr
		? PawnDefaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		: 96.f;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	APawn* NewPawn = GetWorld()->SpawnActor<APawn>(TakeoverPawnClass,
		GroundPosition + FVector(0.f, 0.f, HalfHeight + 5.f), FRotator(0.f, HeadingYaw, 0.f), SpawnParams);
	if (NewPawn == nullptr)
	{
		return false;
	}

	Possess(NewPawn);
	SetViewTargetWithBlend(NewPawn, 0.f);

	if (ABreakingWaveCharacter* NewLife = Cast<ABreakingWaveCharacter>(NewPawn))
	{
		NewLife->SetTransitionInputLocked(true);
		NewLife->ApplyTakeoverState(HeadingYaw, bStartProne, !bStartProne);
	}

	if (DeathCamera != nullptr)
	{
		DeathCamera->Destroy();
		DeathCamera = nullptr;
	}

	const float BlockedUntil = GetWorld()->GetTimeSeconds()
		+ TransitionSettings.FadeInSeconds + TransitionSettings.TargetingDelaySeconds;
	for (TActorIterator<AMGBunkerManager> It(GetWorld()); It; ++It)
	{
		It->NotifyPlayerTakeover(BlockedUntil, DeathAnchorY, GroundPosition, LadderSteps);
	}
	for (TActorIterator<AInfantryManager> It(GetWorld()); It; ++It)
	{
		It->NotifyPlayerTakeover(BlockedUntil);
	}

	return true;
}

void ABreakingWavePlayerController::RetireOldestCorpse()
{
	while (Corpses.Num() > FMath::Max(TransitionSettings.MaxCorpses, 0))
	{
		APawn* Oldest = Corpses[0];
		Corpses.RemoveAt(0);
		if (Oldest != nullptr)
		{
			Oldest->Destroy();
		}
	}
}

AAllySimManager* ABreakingWavePlayerController::FindAllySim() const
{
	for (TActorIterator<AAllySimManager> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

float ABreakingWavePlayerController::PhaseDuration(ETransitionPhase Query) const
{
	switch (Query)
	{
	case ETransitionPhase::DeathShake: return TransitionSettings.DeathShakeSeconds;
	case ETransitionPhase::DeathDescend: return TransitionSettings.DeathDescendSeconds;
	case ETransitionPhase::DeathHold: return TransitionSettings.DeathHoldSeconds;
	case ETransitionPhase::FadeOut: return TransitionSettings.FadeOutSeconds;
	case ETransitionPhase::FadeIn: return TransitionSettings.FadeInSeconds;
	default: return 0.f;
	}
}

void ABreakingWavePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
	
}
