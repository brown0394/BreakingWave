#include "MGBunkerSystem.h"

#include "BeachInfantrySystem.h"

#include "BeachAllySim.h"
#include "BreakingWaveCharacter.h"
#include "BreakingWavePlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "TimerManager.h"

AMGBunkerGun::AMGBunkerGun()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	YawPivot = CreateDefaultSubobject<USceneComponent>(TEXT("YawPivot"));
	YawPivot->SetupAttachment(Root);

	Barrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Barrel"));
	Barrel->SetupAttachment(YawPivot);
	Barrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(Barrel);

	FirePort = CreateDefaultSubobject<USceneComponent>(TEXT("FirePort"));
	FirePort->SetupAttachment(Root);
	FirePort->SetRelativeLocation(FVector(120.f, 0.f, 0.f));

	GunnerMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GunnerMesh"));
	GunnerMesh->SetupAttachment(Root);
	GunnerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LoaderMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LoaderMesh"));
	LoaderMesh->SetupAttachment(Root);
	LoaderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FireLoopAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("FireLoopAudio"));
	FireLoopAudio->SetupAttachment(Muzzle);
	FireLoopAudio->bAutoActivate = false;
	FireLoopAudio->bOverrideAttenuation = true;
	FireLoopAudio->AttenuationOverrides.bAttenuate = true;
	FireLoopAudio->AttenuationOverrides.bSpatialize = true;
	FireLoopAudio->AttenuationOverrides.AttenuationShapeExtents = FVector(3000.f, 0.f, 0.f);
	FireLoopAudio->AttenuationOverrides.FalloffDistance = 47000.f;
}

void AMGBunkerGun::SetAimAngles(float YawDegrees, float PitchDegrees)
{
	YawPivot->SetWorldRotation(FRotator(0.f, YawDegrees, 0.f));
	Barrel->SetRelativeRotation(FRotator(PitchDegrees, 0.f, 0.f));
}

FVector AMGBunkerGun::GetMuzzleLocation() const
{
	return Muzzle->GetComponentLocation();
}

FVector AMGBunkerGun::GetFirePosition() const
{
	return FirePort->GetComponentLocation();
}

void AMGBunkerGun::SetRenderedCrewCount(int32 Count)
{
	GunnerMesh->SetVisibility(Count >= 1);
	LoaderMesh->SetVisibility(Count >= 2);
}

void AMGBunkerGun::SetFiringAudioState(bool bFiring, float DelaySeconds)
{
	if (FireLoopSound != nullptr && FireLoopAudio->GetSound() != FireLoopSound)
	{
		FireLoopAudio->SetSound(FireLoopSound);
	}

	GetWorldTimerManager().ClearTimer(AudioDelayTimer);
	FTimerDelegate Apply = FTimerDelegate::CreateWeakLambda(this, [this, bFiring]()
	{
		if (bFiring)
		{
			FireLoopAudio->Play();
		}
		else
		{
			FireLoopAudio->Stop();
		}
	});

	if (DelaySeconds <= 0.f)
	{
		Apply.Execute();
	}
	else
	{
		GetWorldTimerManager().SetTimer(AudioDelayTimer, Apply, DelaySeconds, false);
	}
}

AMGBunkerManager::AMGBunkerManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMGBunkerManager::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<AAllySimManager> It(GetWorld()); It; ++It)
	{
		AllySim = *It;
		break;
	}

	for (TActorIterator<AInfantryManager> It(GetWorld()); It; ++It)
	{
		Infantry = *It;
		break;
	}

	const int32 AwarenessSlots = (AllySim.IsValid() ? AllySim->GetSettings().MaxAlive : 0) + 1;

	for (TActorIterator<AMGBunkerGun> It(GetWorld()); It; ++It)
	{
		FMGBunkerState State;
		State.Gun = *It;
		State.RestYaw = It->GetActorRotation().Yaw;
		State.AimYaw = State.RestYaw;
		State.TargetYaw = State.RestYaw;
		State.CrewAlive = Settings.GarrisonSize;
		State.BeltRounds = FMath::RoundToInt(Settings.BeltSize * FMath::FRandRange(Settings.StartingBeltFractionMin, 1.f));
		State.Heat = Settings.OverheatThreshold * FMath::FRandRange(0.f, Settings.StartingHeatFractionMax);
		State.CurrentTargetId = NoTargetId;
		State.Awareness.SetNum(AwarenessSlots);
		It->SetRenderedCrewCount(2);
		Bunkers.Add(State);
	}

	if (ImpactSound == nullptr)
	{
		ImpactSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/MGImpact.MGImpact"));
	}

	if (WhizzSound == nullptr)
	{
		WhizzSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/MGWhizz.MGWhizz"));
	}

	FallbackCrackSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/MGCrack.MGCrack"));

	ImpactAttenuation = NewObject<USoundAttenuation>(this);
	ImpactAttenuation->Attenuation.AttenuationShapeExtents = FVector(Settings.ImpactSoundInnerRadius, 0.f, 0.f);
	ImpactAttenuation->Attenuation.FalloffDistance = FMath::Max(Settings.ImpactSoundRadius - Settings.ImpactSoundInnerRadius, 1.f);

	CrackAttenuation = NewObject<USoundAttenuation>(this);
	CrackAttenuation->Attenuation.AttenuationShapeExtents = FVector(150.f, 0.f, 0.f);
	CrackAttenuation->Attenuation.FalloffDistance = Settings.CrackRadius * 3.f;

	WhizzAttenuation = NewObject<USoundAttenuation>(this);
	WhizzAttenuation->Attenuation.AttenuationShapeExtents = FVector(150.f, 0.f, 0.f);
	WhizzAttenuation->Attenuation.FalloffDistance = Settings.WhizzRadius * 3.f;

	Recorder.BeginSession(GetWorld(), Settings,
		AllySim.IsValid() ? &AllySim->GetSettings() : nullptr,
		Infantry.IsValid() ? &Infantry->GetSettings() : nullptr);
}

void AMGBunkerManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Recorder.EndSession();
	Super::EndPlay(EndPlayReason);
}

ABreakingWaveCharacter* AMGBunkerManager::GetPlayerCharacter() const
{
	ABreakingWaveCharacter* Player = Cast<ABreakingWaveCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	return (Player != nullptr && !Player->IsDead()) ? Player : nullptr;
}

bool AMGBunkerManager::CanAcquirePlayer() const
{
	return GetWorld()->GetTimeSeconds() >= PlayerAcquireBlockedUntil;
}

void AMGBunkerManager::NotifyPlayerTakeover(float BlockedUntilTime, float DeathAnchorY,
	const FVector& TakeoverPosition, int32 LadderSteps)
{
	PlayerAcquireBlockedUntil = BlockedUntilTime;

	for (FMGBunkerState& State : Bunkers)
	{
		FMGAwareness& PlayerAwareness = AwarenessFor(State, PlayerTargetId);
		PlayerAwareness = FMGAwareness();
		if (State.CurrentTargetId == PlayerTargetId)
		{
			State.CurrentTargetId = NoTargetId;
		}
	}

	Recorder.LogTakeover(DeathAnchorY, TakeoverPosition, LadderSteps);
}

void AMGBunkerManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (int32 i = 0; i < Bunkers.Num(); ++i)
	{
		UpdateBunker(Bunkers[i], i, DeltaSeconds);
	}

	UpdateBullets(DeltaSeconds);

	bool bPlayerTargeted = false;
	int32 StoppedGunCount = 0;
	for (const FMGBunkerState& State : Bunkers)
	{
		if (State.CrewAlive <= 0)
		{
			continue;
		}
		if (State.Stop != EMGStop::None)
		{
			++StoppedGunCount;
		}
		else if (State.CurrentTargetId == PlayerTargetId)
		{
			bPlayerTargeted = true;
		}
	}
	const ABreakingWaveCharacter* SampledPlayer = GetPlayerCharacter();
	const int32 AlliesInSlab = (SampledPlayer != nullptr && AllySim.IsValid())
		? AllySim->CountAlliesInSlab(SampledPlayer->GetActorLocation().Y) : 0;
	Recorder.SamplePlayer(SampledPlayer, bPlayerTargeted, StoppedGunCount, AlliesInSlab, DeltaSeconds);

	if (bDebug)
	{
		DrawDebugState();
	}
}

void AMGBunkerManager::UpdateBunker(FMGBunkerState& State, int32 BunkerIndex, float DeltaSeconds)
{
	if (!State.Gun.IsValid())
	{
		return;
	}

	State.TimeSinceLastShot += DeltaSeconds;
	State.Heat = FMath::Max(0.f, State.Heat - Settings.HeatCoolPerSecond * DeltaSeconds);

	if (State.CrewAlive <= 0)
	{
		SyncFiringAudio(State, false);
		return;
	}

	if (State.Stop != EMGStop::None)
	{
		SyncFiringAudio(State, false);
		State.StopTimer -= DeltaSeconds;
		if (State.StopTimer <= 0.f)
		{
			if (State.Stop == EMGStop::Reload)
			{
				State.BeltRounds = Settings.BeltSize;
			}
			else if (State.Stop == EMGStop::BarrelChange)
			{
				State.Heat = 0.f;
			}
			Recorder.LogStopEnd(BunkerIndex, State.Stop);
			State.Stop = EMGStop::None;
			State.BurstSeconds = 0.f;
			State.bJamRolledThisBurst = false;
		}
		return;
	}

	State.EvalTimer -= DeltaSeconds;
	if (State.EvalTimer <= 0.f)
	{
		State.EvalTimer += Settings.EvaluationInterval;
		EvaluatePerception(State);
		const int32 PreviousTargetId = State.CurrentTargetId;
		SelectTarget(State, BunkerIndex, GetWorld()->GetTimeSeconds());
		if (State.CurrentTargetId != PreviousTargetId)
		{
			Recorder.LogTargetSwitch(BunkerIndex, PreviousTargetId, State.CurrentTargetId);
		}
	}

	UpdateRotation(State, DeltaSeconds);
	UpdateFiring(State, BunkerIndex, DeltaSeconds);
}

void AMGBunkerManager::EvaluatePerception(FMGBunkerState& State)
{
	const float Now = GetWorld()->GetTimeSeconds();
	const FVector MuzzlePos = State.Gun->GetFirePosition();
	const int32 AllyCount = AllySim.IsValid() ? AllySim->GetAllies().Num() : 0;

	for (int32 TargetId = -1; TargetId < AllyCount; ++TargetId)
	{
		if (TargetId >= 0 && AllySim.IsValid())
		{
			const FSimAlly& Ally = AllySim->GetAllies()[TargetId];
			FMGAwareness& Aw = AwarenessFor(State, TargetId);
			if (Aw.AllyGeneration != Ally.Generation)
			{
				Aw = FMGAwareness();
				Aw.AllyGeneration = Ally.Generation;
			}
		}

		if (!IsTargetAlive(TargetId))
		{
			continue;
		}

		if (TargetId == PlayerTargetId && !CanAcquirePlayer())
		{
			AwarenessFor(State, TargetId).LastExposure = 0.f;
			continue;
		}

		FMGAwareness& Aw = AwarenessFor(State, TargetId);
		const FVector TargetPos = GetTargetPosition(TargetId);
		const FVector ToTarget = TargetPos - MuzzlePos;
		const float Distance = ToTarget.Size();
		if (Distance > Settings.VisibilityMaxRange || Distance < 1.f)
		{
			Aw.LastExposure = 0.f;
			continue;
		}

		const float YawToTarget = FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Y, ToTarget.X));
		if (FMath::Abs(FMath::FindDeltaAngleDegrees(State.RestYaw, YawToTarget)) > Settings.SlitArcHalfAngleDeg)
		{
			Aw.LastExposure = 0.f;
			continue;
		}

		FVector AimPoints[3];
		GetTargetAimPoints(TargetId, AimPoints);
		float Exposure;
		const float Visibility = ComputeVisibility(State, MuzzlePos, AimPoints, Exposure)
			* [&]()
			{
				const float OffAxis = FMath::Abs(FMath::FindDeltaAngleDegrees(State.AimYaw, YawToTarget));
				if (OffAxis <= Settings.AttentionFullAngleDeg)
				{
					return 1.f;
				}
				const float Span = FMath::Max(1.f, Settings.SlitArcHalfAngleDeg - Settings.AttentionFullAngleDeg);
				const float Fade = FMath::Clamp((OffAxis - Settings.AttentionFullAngleDeg) / Span, 0.f, 1.f);
				return FMath::Lerp(1.f, Settings.AttentionEdgeValue, Fade);
			}()
			* (0.3f + 0.7f * FMath::Clamp(1.f - Distance / Settings.VisibilityMaxRange, 0.f, 1.f));

		if (Visibility >= Settings.AwarenessThreshold)
		{
			const bool bWasHidden = Aw.LastExposure <= 0.f;
			if (bWasHidden && Exposure > 0.f)
			{
				Aw.BrokeCoverTime = Now;
			}
			Aw.LastSeenTime = Now;
			Aw.LastKnownPos = TargetPos;
			Aw.LastExposure = Exposure;
		}
		else
		{
			Aw.LastExposure = 0.f;
		}
	}
}

float AMGBunkerManager::ComputeVisibility(const FMGBunkerState& State, const FVector& MuzzlePos,
	const FVector AimPoints[3], float& OutExposure) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MGPerception), false);
	Params.AddIgnoredActor(State.Gun.Get());

	int32 Visible = 0;
	for (int32 i = 0; i < 3; ++i)
	{
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByChannel(Hit, MuzzlePos, AimPoints[i], ECC_Visibility, Params))
		{
			++Visible;
		}
	}
	OutExposure = Visible / 3.f;
	return OutExposure;
}

float AMGBunkerManager::ScoreTarget(const FMGBunkerState& State, int32 TargetId, float Now) const
{
	const FMGAwareness& Aw = AwarenessFor(State, TargetId);
	const float Distance = FVector::Dist(State.Gun->GetFirePosition(), Aw.LastKnownPos);
	const float DistNorm = FMath::Clamp(1.f - Distance / Settings.VisibilityMaxRange, 0.f, 1.f);

	float Score;
	if (Aw.LastExposure > 0.f)
	{
		Score = 100.f * Aw.LastExposure * (0.25f + 0.75f * DistNorm);
		if (IsTargetAlive(TargetId))
		{
			const float GroundSpeed = GetTargetVelocity(TargetId).Size2D();
			const float SpeedNorm = FMath::Clamp(GroundSpeed / Settings.MovingTargetScoreReferenceSpeed, 0.f, 1.f);
			Score *= 1.f + Settings.MovingTargetScoreBonus * SpeedNorm;
		}
	}
	else
	{
		Score = 5.f;
	}

	const float SinceBroke = Now - Aw.BrokeCoverTime;
	if (SinceBroke < Settings.BrokeCoverWindowSeconds)
	{
		Score += 500.f * (1.f - SinceBroke / Settings.BrokeCoverWindowSeconds);
	}

	if (TargetId == PlayerTargetId)
	{
		const float SinceFiredUpon = Now - State.LastFiredUponTime;
		if (SinceFiredUpon < Settings.FiredUponWindowSeconds)
		{
			Score += Settings.FiredUponScoreBonus * (1.f - SinceFiredUpon / Settings.FiredUponWindowSeconds);
		}
		Score *= Settings.PlayerTargetScoreMultiplier;
	}

	return Score;
}

void AMGBunkerManager::SelectTarget(FMGBunkerState& State, int32 BunkerIndex, float Now)
{
	const int32 AllyCount = AllySim.IsValid() ? AllySim->GetAllies().Num() : 0;

	const auto EffectiveScore = [&](int32 TargetId)
	{
		const float Penalty = IsTargetedByAnotherGun(BunkerIndex, TargetId) ? Settings.SharedTargetScorePenalty : 1.f;
		return ScoreTarget(State, TargetId, Now) * Penalty;
	};

	int32 BestId = NoTargetId;
	float BestScore = 0.f;
	for (int32 TargetId = -1; TargetId < AllyCount; ++TargetId)
	{
		if (!IsTargetAlive(TargetId) || !IsAware(State, TargetId, Now))
		{
			continue;
		}
		if (TargetId == PlayerTargetId && !CanAcquirePlayer())
		{
			continue;
		}
		const float Score = EffectiveScore(TargetId);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestId = TargetId;
		}
	}

	const bool bCurrentValid = State.CurrentTargetId != NoTargetId
		&& IsTargetAlive(State.CurrentTargetId)
		&& IsAware(State, State.CurrentTargetId, Now);

	if (!bCurrentValid)
	{
		State.CurrentTargetId = BestId;
		State.RotationSpeedJitter = FMath::FRandRange(1.f - Settings.RotationSpeedVariance, 1.f + Settings.RotationSpeedVariance);
		State.LeadFraction = FMath::FRandRange(Settings.LeadFractionMin, Settings.LeadFractionMax);
		return;
	}

	if (BestId != State.CurrentTargetId && BestId != NoTargetId)
	{
		const float CurrentScore = EffectiveScore(State.CurrentTargetId);
		if (BestScore > CurrentScore * Settings.TargetSwitchMargin)
		{
			State.CurrentTargetId = BestId;
			State.RotationSpeedJitter = FMath::FRandRange(1.f - Settings.RotationSpeedVariance, 1.f + Settings.RotationSpeedVariance);
			State.LeadFraction = FMath::FRandRange(Settings.LeadFractionMin, Settings.LeadFractionMax);
		}
	}
}

bool AMGBunkerManager::IsTargetedByAnotherGun(int32 BunkerIndex, int32 TargetId) const
{
	for (int32 i = 0; i < Bunkers.Num(); ++i)
	{
		if (i == BunkerIndex || Bunkers[i].CrewAlive <= 0)
		{
			continue;
		}
		if (Bunkers[i].CurrentTargetId == TargetId)
		{
			return true;
		}
	}
	return false;
}

void AMGBunkerManager::UpdateRotation(FMGBunkerState& State, float DeltaSeconds)
{
	if (State.CurrentTargetId != NoTargetId)
	{
		const FMGAwareness& Aw = AwarenessFor(State, State.CurrentTargetId);
		const bool bLiveTrack = Aw.LastExposure > 0.f && IsTargetAlive(State.CurrentTargetId);
		FVector AimPos = bLiveTrack ? GetTargetPosition(State.CurrentTargetId) : Aw.LastKnownPos;
		if (bLiveTrack)
		{
			const float FlightTime = FVector::Dist(State.Gun->GetFirePosition(), AimPos) / Settings.MuzzleVelocity;
			AimPos += GetTargetVelocity(State.CurrentTargetId) * FlightTime * State.LeadFraction;
		}
		const FVector ToAim = AimPos - State.Gun->GetFirePosition();
		State.TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(ToAim.Y, ToAim.X));
		State.TargetPitch = FMath::RadiansToDegrees(FMath::Atan2(ToAim.Z, ToAim.Size2D()));
	}
	else
	{
		const float ScanHalfArc = Settings.SlitArcHalfAngleDeg * 0.7f;
		State.ScanPhase += DeltaSeconds * Settings.ScanSpeedDegPerSec / FMath::Max(1.f, ScanHalfArc);
		State.TargetYaw = State.RestYaw + FMath::Sin(State.ScanPhase) * ScanHalfArc;
		State.TargetPitch = 0.f;
	}

	const float MaxStep = Settings.MaxRotationSpeedDegPerSec * State.RotationSpeedJitter * DeltaSeconds;
	State.AimYaw = FMath::FixedTurn(State.AimYaw, State.TargetYaw, MaxStep);
	State.AimPitch = FMath::FixedTurn(State.AimPitch, State.TargetPitch, MaxStep);
	State.Gun->SetAimAngles(State.AimYaw, State.AimPitch);
}

void AMGBunkerManager::UpdateFiring(FMGBunkerState& State, int32 BunkerIndex, float DeltaSeconds)
{
	if (State.CurrentTargetId == NoTargetId)
	{
		State.BurstSeconds = 0.f;
		State.FireAccumulator = 0.f;
		SyncFiringAudio(State, false);
		return;
	}

	if (State.TimeSinceLastShot > Settings.BurstGapSeconds)
	{
		State.BurstSeconds = 0.f;
		State.bJamRolledThisBurst = false;
	}

	if (!State.bJamRolledThisBurst)
	{
		State.bJamRolledThisBurst = true;
		if (FMath::FRand() < Settings.JamChancePerBurst)
		{
			StartStop(State, BunkerIndex, EMGStop::Jam);
			return;
		}
	}

	State.FireAccumulator += Settings.RoundsPerSecond * DeltaSeconds;
	while (State.FireAccumulator >= 1.f && State.Stop == EMGStop::None)
	{
		State.FireAccumulator -= 1.f;
		FireRound(State, BunkerIndex);
		--State.BeltRounds;
		State.Heat += Settings.HeatPerShot;
		State.TimeSinceLastShot = 0.f;

		if (State.BeltRounds <= 0)
		{
			StartStop(State, BunkerIndex, EMGStop::Reload);
		}
		else if (State.Heat >= Settings.OverheatThreshold)
		{
			StartStop(State, BunkerIndex, EMGStop::BarrelChange);
		}
	}

	State.BurstSeconds += DeltaSeconds;
	SyncFiringAudio(State, State.Stop == EMGStop::None);
}

void AMGBunkerManager::FireRound(FMGBunkerState& State, int32 BunkerIndex)
{
	float Dispersion = Settings.BaseDispersionDeg;

	if (FMath::Abs(FMath::FindDeltaAngleDegrees(State.AimYaw, State.TargetYaw)) > Settings.RotatingAccuracyThresholdDeg)
	{
		Dispersion *= Settings.RotatingDispersionMultiplier;
	}

	const float BurstAlpha = FMath::Clamp(State.BurstSeconds / Settings.BurstDispersionRampSeconds, 0.f, 1.f);
	Dispersion *= FMath::Lerp(1.f, Settings.MaxBurstDispersionMultiplier, BurstAlpha);

	if (IsTargetAlive(State.CurrentTargetId))
	{
		if (GetTargetVelocity(State.CurrentTargetId).Size() > Settings.MovingTargetSpeedThreshold)
		{
			Dispersion *= Settings.MovingTargetDispersionMultiplier;
		}
		if (IsTargetProne(State.CurrentTargetId))
		{
			Dispersion *= Settings.ProneTargetDispersionMultiplier;
		}
	}

	const FVector AimDir = FRotator(State.AimPitch, State.AimYaw, 0.f).Vector();
	const FVector ShotDir = FMath::VRandCone(AimDir, FMath::DegreesToRadians(Dispersion));

	FMGBullet Bullet;
	Bullet.Position = State.Gun->GetFirePosition();
	Bullet.Velocity = ShotDir * Settings.MuzzleVelocity;
	Bullet.RemainingLife = Settings.BulletLifetime;
	Bullet.Source = EMGBulletSource::Gun;
	Bullet.SourceIndex = BunkerIndex;
	Bullets.Add(Bullet);

	Recorder.LogShot(BunkerIndex, Bullet.Position, State.CurrentTargetId);
}

void AMGBunkerManager::StartStop(FMGBunkerState& State, int32 BunkerIndex, EMGStop Stop)
{
	State.Stop = Stop;
	State.StopTimer = StopDuration(State, Stop);
	SyncFiringAudio(State, false);
	Recorder.LogStopStart(BunkerIndex, Stop, State.StopTimer);
}

float AMGBunkerManager::StopDuration(const FMGBunkerState& State, EMGStop Stop) const
{
	const bool bSolo = State.CrewAlive <= 1;
	const bool bReducedCrew = State.CrewAlive <= Settings.ReducedCrewThreshold;

	switch (Stop)
	{
	case EMGStop::Reload:
	{
		const float Base = FMath::FRandRange(Settings.ReloadDurationMin, Settings.ReloadDurationMax);
		return Base * (bSolo ? Settings.SoloStopMultiplier : bReducedCrew ? Settings.ReducedCrewReloadMultiplier : 1.f);
	}
	case EMGStop::BarrelChange:
	{
		const float Base = FMath::FRandRange(Settings.BarrelChangeDurationMin, Settings.BarrelChangeDurationMax);
		return Base * (bSolo ? Settings.SoloStopMultiplier : 1.f);
	}
	case EMGStop::Jam:
		return FMath::FRandRange(Settings.JamDurationMin, Settings.JamDurationMax);
	case EMGStop::Takeover:
		return FMath::FRandRange(Settings.TakeoverDurationMin, Settings.TakeoverDurationMax);
	default:
		return 0.f;
	}
}

static int32 BulletShooterId(const FMGBullet& Bullet)
{
	switch (Bullet.Source)
	{
	case EMGBulletSource::Gun: return Bullet.SourceIndex;
	case EMGBulletSource::PlayerRifle: return Playtest::PlayerTargetId;
	default: return 1000 + Bullet.SourceIndex;
	}
}

void AMGBunkerManager::SpawnBullet(EMGBulletSource Source, int32 SourceIndex, const FVector& Position, const FVector& Velocity,
	int32 TargetId)
{
	FMGBullet Bullet;
	Bullet.Position = Position;
	Bullet.Velocity = Velocity;
	Bullet.RemainingLife = Settings.BulletLifetime;
	Bullet.Source = Source;
	Bullet.SourceIndex = SourceIndex;
	Bullets.Add(Bullet);

	if (Source == EMGBulletSource::PlayerRifle)
	{
		Recorder.LogPlayerShot(Position);
	}
	else if (Source == EMGBulletSource::InfantryRifle)
	{
		Recorder.LogInfantryShot(SourceIndex, Position, TargetId);
	}
}

void AMGBunkerManager::UpdateBullets(float DeltaSeconds)
{
	const ABreakingWaveCharacter* Player = GetPlayerCharacter();
	const FVector PlayerHead = Player != nullptr
		? Player->GetFirstPersonCameraComponent()->GetComponentLocation()
		: FVector::ZeroVector;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MGBullet), false);
	for (const FMGBunkerState& State : Bunkers)
	{
		Params.AddIgnoredActor(State.Gun.Get());
	}

	for (int32 i = Bullets.Num() - 1; i >= 0; --i)
	{
		FMGBullet& Bullet = Bullets[i];
		Bullet.RemainingLife -= DeltaSeconds;
		if (Bullet.RemainingLife <= 0.f)
		{
			Bullets.RemoveAtSwap(i);
			continue;
		}

		const FVector Start = Bullet.Position;
		const FVector End = Start + Bullet.Velocity * DeltaSeconds;

		float HitT = 2.f;
		FVector HitPoint = End;
		enum class EHit { None, World, Ally, Player, Crew, Soldier } HitKind = EHit::None;
		int32 HitAllyIndex = -1;
		int32 HitCrewBunkerIndex = -1;
		FName HitBoneName = NAME_None;
		const bool bPlayerBullet = Bullet.Source == EMGBulletSource::PlayerRifle;

		FHitResult WorldHit;
		if (GetWorld()->LineTraceSingleByChannel(WorldHit, Start, End, ECC_Visibility, Params))
		{
			HitT = WorldHit.Time;
			HitPoint = WorldHit.ImpactPoint;
			HitKind = EHit::World;
		}

		if (!bPlayerBullet && AllySim.IsValid())
		{
			const FAllySimSettings& AllySettings = AllySim->GetSettings();
			TArray<FSimAlly>& Allies = AllySim->GetAllies();
			for (int32 AllyIndex = 0; AllyIndex < Allies.Num(); ++AllyIndex)
			{
				const FSimAlly& Ally = Allies[AllyIndex];
				if (!Ally.bAlive)
				{
					continue;
				}
				const FVector BodyBottom = Ally.Position + FVector(0.f, 0.f, AllySettings.BodyRadius);
				const FVector BodyTop = Ally.Position + FVector(0.f, 0.f, FMath::Max(AllySim->GetBodyHeight(Ally) - AllySettings.BodyRadius, AllySettings.BodyRadius + 1.f));
				FVector OnBullet, OnBody;
				FMath::SegmentDistToSegmentSafe(Start, End, BodyBottom, BodyTop, OnBullet, OnBody);
				if (FVector::Dist(OnBullet, OnBody) < AllySettings.BodyRadius)
				{
					const float T = FVector::Dist(Start, OnBullet) / FMath::Max(FVector::Dist(Start, End), 1.f);
					if (T < HitT)
					{
						HitT = T;
						HitPoint = OnBullet;
						HitKind = EHit::Ally;
						HitAllyIndex = AllyIndex;
					}
				}
			}
		}

		int32 HitSoldierIndex = -1;
		if (bPlayerBullet && Infantry.IsValid())
		{
			for (int32 SoldierIndex = 0; SoldierIndex < Infantry->GetSoldierCount(); ++SoldierIndex)
			{
				FVector BodyBottom, BodyTop;
				float BodyRadius = 0.f;
				if (!Infantry->GetSoldierBodySegment(SoldierIndex, BodyBottom, BodyTop, BodyRadius))
				{
					continue;
				}
				FVector OnBullet, OnBody;
				FMath::SegmentDistToSegmentSafe(Start, End, BodyBottom, BodyTop, OnBullet, OnBody);
				if (FVector::Dist(OnBullet, OnBody) < BodyRadius)
				{
					const float T = FVector::Dist(Start, OnBullet) / FMath::Max(FVector::Dist(Start, End), 1.f);
					if (T < HitT)
					{
						HitT = T;
						HitPoint = OnBullet;
						HitKind = EHit::Soldier;
						HitSoldierIndex = SoldierIndex;
					}
				}
			}
		}

		if (bPlayerBullet)
		{
			for (int32 BunkerIndex = 0; BunkerIndex < Bunkers.Num(); ++BunkerIndex)
			{
				const FMGBunkerState& State = Bunkers[BunkerIndex];
				if (State.CrewAlive <= 0 || !State.Gun.IsValid())
				{
					continue;
				}
				const USkeletalMeshComponent* CrewMeshes[2] = { State.Gun->GunnerMesh, State.Gun->LoaderMesh };
				const int32 RenderedCrew = FMath::Min(State.CrewAlive, 2);
				for (int32 CrewIndex = 0; CrewIndex < RenderedCrew; ++CrewIndex)
				{
					const USkeletalMeshComponent* CrewMesh = CrewMeshes[CrewIndex];
					if (CrewMesh == nullptr)
					{
						continue;
					}
					const FBoxSphereBounds CrewBounds = CrewMesh->Bounds;
					const float AxisHalf = FMath::Max(CrewBounds.BoxExtent.Z - Settings.CrewHitRadius, 1.f);
					const FVector AxisBottom = CrewBounds.Origin - FVector(0.f, 0.f, AxisHalf);
					const FVector AxisTop = CrewBounds.Origin + FVector(0.f, 0.f, AxisHalf);
					FVector OnBullet, OnBody;
					FMath::SegmentDistToSegmentSafe(Start, End, AxisBottom, AxisTop, OnBullet, OnBody);
					if (FVector::Dist(OnBullet, OnBody) < Settings.CrewHitRadius)
					{
						const float T = FVector::Dist(Start, OnBullet) / FMath::Max(FVector::Dist(Start, End), 1.f);
						if (T < HitT)
						{
							HitT = T;
							HitPoint = OnBullet;
							HitKind = EHit::Crew;
							HitCrewBunkerIndex = BunkerIndex;
						}
					}
				}
			}
		}
		else if (Player != nullptr)
		{
			FHitResult BodyHit;
			if (Player->TraceBody(Start, End, BodyHit))
			{
				const float T = FVector::Dist(Start, BodyHit.ImpactPoint) / FMath::Max(FVector::Dist(Start, End), 1.f);
				if (T < HitT)
				{
					HitT = T;
					HitPoint = BodyHit.ImpactPoint;
					HitBoneName = BodyHit.BoneName;
					HitKind = EHit::Player;
				}
			}
		}

		const FVector TravelEnd = HitKind == EHit::None ? End : HitPoint;

		if (!bPlayerBullet && !Bullet.bCrackPlayed && Player != nullptr)
		{
			const FVector NearPoint = FMath::ClosestPointOnSegment(PlayerHead, Start, TravelEnd);
			const float MissDistance = FVector::Dist(NearPoint, PlayerHead);
			const bool bClosestApproachKnown = HitKind != EHit::None
				|| FVector::DistSquared(NearPoint, TravelEnd) > 1.f;
			if (MissDistance < Settings.WhizzRadius && bClosestApproachKnown)
			{
				Bullet.bCrackPlayed = true;
				if (MissDistance < Settings.CrackRadius)
				{
					Recorder.LogCrack(BulletShooterId(Bullet), NearPoint);
					USoundBase* Crack = FallbackCrackSound;
					if (Bullet.Source == EMGBulletSource::Gun
						&& Bunkers.IsValidIndex(Bullet.SourceIndex) && Bunkers[Bullet.SourceIndex].Gun.IsValid()
						&& Bunkers[Bullet.SourceIndex].Gun->GetCrackSound() != nullptr)
					{
						Crack = Bunkers[Bullet.SourceIndex].Gun->GetCrackSound();
					}
					if (Crack != nullptr)
					{
						const float Volume = FMath::Lerp(1.f, Settings.CrackVolumeAtEdge,
							MissDistance / Settings.CrackRadius);
						const float Pitch = FMath::FRandRange(
							1.f - Settings.CrackPitchVariance, 1.f + Settings.CrackPitchVariance);
						UGameplayStatics::PlaySoundAtLocation(GetWorld(), Crack, NearPoint,
							FRotator::ZeroRotator, Volume, Pitch, 0.f, CrackAttenuation);
					}
				}
				else if (WhizzSound != nullptr)
				{
					Recorder.LogWhizz(BulletShooterId(Bullet), NearPoint);
					const float BandFraction = (MissDistance - Settings.CrackRadius)
						/ FMath::Max(Settings.WhizzRadius - Settings.CrackRadius, 1.f);
					const float Volume = FMath::Lerp(1.f, Settings.WhizzVolumeAtEdge, BandFraction);
					const float Pitch = FMath::FRandRange(
						1.f - Settings.WhizzPitchVariance, 1.f + Settings.WhizzPitchVariance);
					UGameplayStatics::PlaySoundAtLocation(GetWorld(), WhizzSound, NearPoint,
						FRotator::ZeroRotator, Volume, Pitch, 0.f, WhizzAttenuation);
				}
			}
		}

#if ENABLE_DRAW_DEBUG
		DrawDebugLine(GetWorld(), Start, TravelEnd,
			bPlayerBullet ? FColor(120, 200, 255) : FColor(255, 190, 90), false, 0.06f, 0, 1.5f);
#endif

		switch (HitKind)
		{
		case EHit::World:
#if ENABLE_DRAW_DEBUG
			DrawDebugPoint(GetWorld(), HitPoint, 6.f, FColor(240, 220, 140), false, 0.4f);
#endif
			Recorder.LogImpact(BulletShooterId(Bullet), HitPoint);
			if (ImpactSound != nullptr && Player != nullptr
				&& FVector::Dist(HitPoint, PlayerHead) < Settings.ImpactSoundRadius
				&& GetWorld()->GetTimeSeconds() - LastImpactSoundTime >= Settings.ImpactSoundMinInterval)
			{
				LastImpactSoundTime = GetWorld()->GetTimeSeconds();
				const float Pitch = FMath::FRandRange(
					1.f - Settings.ImpactPitchVariance, 1.f + Settings.ImpactPitchVariance);
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, HitPoint,
					FRotator::ZeroRotator, 1.f, Pitch, 0.f, ImpactAttenuation);
			}
			if (bPlayerBullet)
			{
				MarkFiredUponNear(HitPoint, GetWorld()->GetTimeSeconds());
				if (Infantry.IsValid())
				{
					Infantry->NotifyImpactNear(HitPoint);
				}
			}
			Bullets.RemoveAtSwap(i);
			break;
		case EHit::Ally:
			AllySim->KillAlly(HitAllyIndex);
			Recorder.LogAllyKilled(BulletShooterId(Bullet), HitPoint);
			Bullets.RemoveAtSwap(i);
			break;
		case EHit::Player:
			HandlePlayerHit(BulletShooterId(Bullet), HitPoint, Bullet.Velocity.GetSafeNormal(), HitBoneName);
			Bullets.RemoveAtSwap(i);
			break;
		case EHit::Crew:
			KillCrewMemberOnBunker(HitCrewBunkerIndex);
			if (Bunkers.IsValidIndex(HitCrewBunkerIndex))
			{
				Bunkers[HitCrewBunkerIndex].LastFiredUponTime = GetWorld()->GetTimeSeconds();
			}
			Bullets.RemoveAtSwap(i);
			break;
		case EHit::Soldier:
			Infantry->NotifySoldierHit(HitSoldierIndex, HitPoint);
			Recorder.LogInfantryDown(HitSoldierIndex, HitPoint);
			Bullets.RemoveAtSwap(i);
			break;
		default:
			Bullet.Position = End;
			break;
		}
	}
}

void AMGBunkerManager::HandlePlayerHit(int32 SourceBunkerIndex, const FVector& HitPoint,
	const FVector& ShotDirection, FName BoneName)
{
	ABreakingWaveCharacter* Player = GetPlayerCharacter();
	if (Player == nullptr)
	{
		return;
	}

	const bool bHeadshot = Player->IsHeadBone(BoneName);
	Recorder.LogPlayerHit(SourceBunkerIndex, HitPoint, bNoDamage, BoneName, bHeadshot);

	if (bNoDamage)
	{
		if (GEngine != nullptr)
		{
			GEngine->AddOnScreenDebugMessage(101, 1.f, FColor::Yellow, TEXT("MG hit (no damage)"));
		}
		return;
	}

	if (Player->TakeBulletHit(HitPoint, ShotDirection, bHeadshot) != EPlayerHitOutcome::Killed)
	{
		return;
	}

	Recorder.LogPlayerDeath(Player->GetActorLocation());

	if (ABreakingWavePlayerController* PC = Cast<ABreakingWavePlayerController>(Player->GetController()))
	{
		PC->BeginDeathTransition();
	}
}

void AMGBunkerManager::SyncFiringAudio(FMGBunkerState& State, bool bActuallyFiring)
{
	if (State.bAudioFiring == bActuallyFiring || !State.Gun.IsValid())
	{
		return;
	}
	State.bAudioFiring = bActuallyFiring;

	float Delay = 0.f;
	if (const ABreakingWaveCharacter* Player = GetPlayerCharacter())
	{
		Delay = FVector::Dist(Player->GetActorLocation(), State.Gun->GetFirePosition()) / Settings.SpeedOfSound;
	}
	State.Gun->SetFiringAudioState(bActuallyFiring, Delay);
}

bool AMGBunkerManager::IsTargetAlive(int32 TargetId) const
{
	if (TargetId == PlayerTargetId)
	{
		return GetPlayerCharacter() != nullptr;
	}
	if (TargetId >= 0 && AllySim.IsValid() && AllySim->GetAllies().IsValidIndex(TargetId))
	{
		return AllySim->GetAllies()[TargetId].bAlive;
	}
	return false;
}

FVector AMGBunkerManager::GetTargetPosition(int32 TargetId) const
{
	if (TargetId == PlayerTargetId)
	{
		const ABreakingWaveCharacter* Player = GetPlayerCharacter();
		return Player != nullptr ? Player->GetActorLocation() : FVector::ZeroVector;
	}
	const FSimAlly& Ally = AllySim->GetAllies()[TargetId];
	return Ally.Position + FVector(0.f, 0.f, AllySim->GetBodyHeight(Ally) * 0.6f);
}

FVector AMGBunkerManager::GetTargetVelocity(int32 TargetId) const
{
	if (TargetId == PlayerTargetId)
	{
		const ABreakingWaveCharacter* Player = GetPlayerCharacter();
		return Player != nullptr ? Player->GetVelocity() : FVector::ZeroVector;
	}
	const FSimAlly& Ally = AllySim->GetAllies()[TargetId];
	if (Ally.Stance == ESimAllyStance::Prone)
	{
		return FVector::ZeroVector;
	}
	const float HeadingRad = FMath::DegreesToRadians(Ally.HeadingYaw);
	return FVector(FMath::Cos(HeadingRad), FMath::Sin(HeadingRad), 0.f) * Ally.Speed;
}

bool AMGBunkerManager::IsTargetProne(int32 TargetId) const
{
	if (TargetId == PlayerTargetId)
	{
		const ABreakingWaveCharacter* Player = GetPlayerCharacter();
		return Player != nullptr && Player->IsProne();
	}
	return AllySim->GetAllies()[TargetId].Stance == ESimAllyStance::Prone;
}

void AMGBunkerManager::GetTargetAimPoints(int32 TargetId, FVector OutPoints[3]) const
{
	if (TargetId == PlayerTargetId)
	{
		const ABreakingWaveCharacter* Player = GetPlayerCharacter();
		if (Player == nullptr)
		{
			OutPoints[0] = OutPoints[1] = OutPoints[2] = FVector::ZeroVector;
			return;
		}
		OutPoints[0] = Player->GetFirstPersonCameraComponent()->GetComponentLocation();
		OutPoints[1] = Player->GetActorLocation();
		OutPoints[2] = Player->GetActorLocation() - FVector(0.f, 0.f, 40.f);
		return;
	}
	AllySim->GetAimPoints(AllySim->GetAllies()[TargetId], OutPoints);
}

FMGAwareness& AMGBunkerManager::AwarenessFor(FMGBunkerState& State, int32 TargetId)
{
	return State.Awareness[TargetId == PlayerTargetId ? State.Awareness.Num() - 1 : TargetId];
}

const FMGAwareness& AMGBunkerManager::AwarenessFor(const FMGBunkerState& State, int32 TargetId) const
{
	return State.Awareness[TargetId == PlayerTargetId ? State.Awareness.Num() - 1 : TargetId];
}

bool AMGBunkerManager::IsAware(const FMGBunkerState& State, int32 TargetId, float Now) const
{
	return Now - AwarenessFor(State, TargetId).LastSeenTime < Settings.AwarenessMemorySeconds;
}

void AMGBunkerManager::ToggleNoDamage()
{
	bNoDamage = !bNoDamage;
	Recorder.LogNoDamageToggle(bNoDamage);
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(102, 3.f, FColor::Yellow,
			bNoDamage ? TEXT("MG damage OFF") : TEXT("MG damage ON"));
	}
}

void AMGBunkerManager::KillCrewMemberOnBunker(int32 BunkerIndex)
{
	if (!Bunkers.IsValidIndex(BunkerIndex) || Bunkers[BunkerIndex].CrewAlive <= 0)
	{
		return;
	}
	FMGBunkerState& State = Bunkers[BunkerIndex];
	--State.CrewAlive;
	Recorder.LogCrewKilled(BunkerIndex, State.CrewAlive);
	if (State.Gun.IsValid())
	{
		State.Gun->SetRenderedCrewCount(FMath::Min(State.CrewAlive, 2));
	}
	if (State.CrewAlive > 0)
	{
		StartStop(State, BunkerIndex, EMGStop::Takeover);
	}
	else
	{
		SyncFiringAudio(State, false);
	}
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(103, 3.f, FColor::Orange,
			FString::Printf(TEXT("MG crew member killed — %d remain"), State.CrewAlive));
	}
}

void AMGBunkerManager::MarkFiredUponNear(const FVector& ImpactPoint, float Now)
{
	for (FMGBunkerState& State : Bunkers)
	{
		if (State.Gun.IsValid()
			&& FVector::Dist(ImpactPoint, State.Gun->GetFirePosition()) < Settings.FiredUponAlertRadius)
		{
			State.LastFiredUponTime = Now;
		}
	}
}

void AMGBunkerManager::KillGunCrewMember()
{
	for (int32 BunkerIndex = 0; BunkerIndex < Bunkers.Num(); ++BunkerIndex)
	{
		if (Bunkers[BunkerIndex].CrewAlive > 0)
		{
			KillCrewMemberOnBunker(BunkerIndex);
			return;
		}
	}
}

void AMGBunkerManager::ToggleDebug()
{
	bDebug = !bDebug;
	if (AllySim.IsValid())
	{
		AllySim->SetDebugDraw(bDebug);
	}
	if (Infantry.IsValid())
	{
		Infantry->SetDebugDraw(bDebug);
	}
}

void AMGBunkerManager::DrawDebugState() const
{
#if ENABLE_DRAW_DEBUG
	for (const FMGBunkerState& State : Bunkers)
	{
		if (!State.Gun.IsValid())
		{
			continue;
		}
		const FVector Muzzle = State.Gun->GetFirePosition();
		const FVector AimDir = FRotator(State.AimPitch, State.AimYaw, 0.f).Vector();
		DrawDebugLine(GetWorld(), Muzzle, Muzzle + AimDir * 3000.f, FColor::Red, false, -1.f, 0, 2.f);

		const TCHAR* StopName =
			State.Stop == EMGStop::Reload ? TEXT("RELOAD") :
			State.Stop == EMGStop::BarrelChange ? TEXT("BARREL") :
			State.Stop == EMGStop::Jam ? TEXT("JAM") :
			State.Stop == EMGStop::Takeover ? TEXT("TAKEOVER") : TEXT("FIRING");
		const FString Text = FString::Printf(TEXT("crew %d  belt %d  heat %.0f  %s %.1f  tgt %d"),
			State.CrewAlive, State.BeltRounds, State.Heat,
			State.CrewAlive > 0 ? StopName : TEXT("DEAD"),
			State.StopTimer, State.CurrentTargetId);
		DrawDebugString(GetWorld(), Muzzle + FVector(0.f, 0.f, 200.f), Text, nullptr, FColor::White, 0.f, true);
	}
#endif
}
