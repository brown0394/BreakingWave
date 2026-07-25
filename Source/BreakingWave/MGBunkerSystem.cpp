#include "MGBunkerSystem.h"

#include "BeachAllySim.h"
#include "BreakingWaveCharacter.h"
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

	const int32 AwarenessSlots = (AllySim.IsValid() ? AllySim->GetSettings().MaxAlive : 0) + 1;

	for (TActorIterator<AMGBunkerGun> It(GetWorld()); It; ++It)
	{
		FMGBunkerState State;
		State.Gun = *It;
		State.RestYaw = It->GetActorRotation().Yaw;
		State.AimYaw = State.RestYaw;
		State.TargetYaw = State.RestYaw;
		State.CrewAlive = Settings.GarrisonSize;
		State.BeltRounds = Settings.BeltSize;
		State.CurrentTargetId = NoTargetId;
		State.Awareness.SetNum(AwarenessSlots);
		It->SetRenderedCrewCount(2);
		Bunkers.Add(State);
	}
}

ABreakingWaveCharacter* AMGBunkerManager::GetPlayerCharacter() const
{
	if (!CachedPlayer.IsValid())
	{
		CachedPlayer = Cast<ABreakingWaveCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	}
	return CachedPlayer.Get();
}

void AMGBunkerManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bPlayerSpawnCaptured)
	{
		if (const ABreakingWaveCharacter* Player = GetPlayerCharacter())
		{
			PlayerSpawnTransform = Player->GetActorTransform();
			bPlayerSpawnCaptured = true;
		}
	}

	for (int32 i = 0; i < Bunkers.Num(); ++i)
	{
		UpdateBunker(Bunkers[i], i, DeltaSeconds);
	}

	UpdateBullets(DeltaSeconds);

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
		SelectTarget(State, GetWorld()->GetTimeSeconds());
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

	return Score;
}

void AMGBunkerManager::SelectTarget(FMGBunkerState& State, float Now)
{
	const int32 AllyCount = AllySim.IsValid() ? AllySim->GetAllies().Num() : 0;

	int32 BestId = NoTargetId;
	float BestScore = 0.f;
	for (int32 TargetId = -1; TargetId < AllyCount; ++TargetId)
	{
		if (!IsTargetAlive(TargetId) || !IsAware(State, TargetId, Now))
		{
			continue;
		}
		const float Score = ScoreTarget(State, TargetId, Now);
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
		return;
	}

	if (BestId != State.CurrentTargetId && BestId != NoTargetId)
	{
		const float CurrentScore = ScoreTarget(State, State.CurrentTargetId, Now);
		if (BestScore > CurrentScore * Settings.TargetSwitchMargin)
		{
			State.CurrentTargetId = BestId;
			State.RotationSpeedJitter = FMath::FRandRange(1.f - Settings.RotationSpeedVariance, 1.f + Settings.RotationSpeedVariance);
		}
	}
}

void AMGBunkerManager::UpdateRotation(FMGBunkerState& State, float DeltaSeconds)
{
	if (State.CurrentTargetId != NoTargetId)
	{
		const FMGAwareness& Aw = AwarenessFor(State, State.CurrentTargetId);
		const bool bLiveTrack = Aw.LastExposure > 0.f && IsTargetAlive(State.CurrentTargetId);
		const FVector AimPos = bLiveTrack ? GetTargetPosition(State.CurrentTargetId) : Aw.LastKnownPos;
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
			StartStop(State, EMGStop::Jam);
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
			StartStop(State, EMGStop::Reload);
		}
		else if (State.Heat >= Settings.OverheatThreshold)
		{
			StartStop(State, EMGStop::BarrelChange);
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
	Bullet.SourceBunkerIndex = BunkerIndex;
	Bullets.Add(Bullet);
}

void AMGBunkerManager::StartStop(FMGBunkerState& State, EMGStop Stop)
{
	State.Stop = Stop;
	State.StopTimer = StopDuration(State, Stop);
	SyncFiringAudio(State, false);
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
		enum class EHit { None, World, Ally, Player } HitKind = EHit::None;
		int32 HitAllyIndex = -1;

		FHitResult WorldHit;
		if (GetWorld()->LineTraceSingleByChannel(WorldHit, Start, End, ECC_Visibility, Params))
		{
			HitT = WorldHit.Time;
			HitPoint = WorldHit.ImpactPoint;
			HitKind = EHit::World;
		}

		if (AllySim.IsValid())
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

		if (Player != nullptr)
		{
			const UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
			const FVector Center = Capsule->GetComponentLocation();
			const float HalfHeight = FMath::Max(Capsule->GetScaledCapsuleHalfHeight() - Capsule->GetScaledCapsuleRadius(), 1.f);
			const FVector AxisBottom = Center - FVector(0.f, 0.f, HalfHeight);
			const FVector AxisTop = Center + FVector(0.f, 0.f, HalfHeight);
			FVector OnBullet, OnBody;
			FMath::SegmentDistToSegmentSafe(Start, End, AxisBottom, AxisTop, OnBullet, OnBody);
			if (FVector::Dist(OnBullet, OnBody) < Capsule->GetScaledCapsuleRadius())
			{
				const float T = FVector::Dist(Start, OnBullet) / FMath::Max(FVector::Dist(Start, End), 1.f);
				if (T < HitT)
				{
					HitT = T;
					HitPoint = OnBullet;
					HitKind = EHit::Player;
				}
			}
		}

		const FVector TravelEnd = HitKind == EHit::None ? End : HitPoint;

		if (!Bullet.bCrackPlayed && Player != nullptr)
		{
			const FVector NearPoint = FMath::ClosestPointOnSegment(PlayerHead, Start, TravelEnd);
			if (FVector::Dist(NearPoint, PlayerHead) < Settings.CrackRadius)
			{
				Bullet.bCrackPlayed = true;
				if (Bunkers.IsValidIndex(Bullet.SourceBunkerIndex) && Bunkers[Bullet.SourceBunkerIndex].Gun.IsValid())
				{
					if (USoundBase* Crack = Bunkers[Bullet.SourceBunkerIndex].Gun->GetCrackSound())
					{
						UGameplayStatics::PlaySoundAtLocation(GetWorld(), Crack, NearPoint);
					}
				}
			}
		}

#if ENABLE_DRAW_DEBUG
		DrawDebugLine(GetWorld(), Start, TravelEnd, FColor(255, 190, 90), false, 0.06f, 0, 1.5f);
#endif

		switch (HitKind)
		{
		case EHit::World:
#if ENABLE_DRAW_DEBUG
			DrawDebugPoint(GetWorld(), HitPoint, 6.f, FColor(240, 220, 140), false, 0.4f);
#endif
			Bullets.RemoveAtSwap(i);
			break;
		case EHit::Ally:
			AllySim->KillAlly(HitAllyIndex);
			Bullets.RemoveAtSwap(i);
			break;
		case EHit::Player:
			HandlePlayerHit();
			Bullets.RemoveAtSwap(i);
			break;
		default:
			Bullet.Position = End;
			break;
		}
	}
}

void AMGBunkerManager::HandlePlayerHit()
{
	ABreakingWaveCharacter* Player = GetPlayerCharacter();
	if (Player == nullptr)
	{
		return;
	}

	if (bNoDamage)
	{
		if (GEngine != nullptr)
		{
			GEngine->AddOnScreenDebugMessage(101, 1.f, FColor::Yellow, TEXT("MG hit (no damage)"));
		}
		return;
	}

	if (!bPlayerSpawnCaptured)
	{
		return;
	}

	Player->SetActorTransform(PlayerSpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	Player->GetCharacterMovement()->StopMovementImmediately();
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		PC->SetControlRotation(PlayerSpawnTransform.Rotator());
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
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(102, 3.f, FColor::Yellow,
			bNoDamage ? TEXT("MG damage OFF") : TEXT("MG damage ON"));
	}
}

void AMGBunkerManager::KillGunCrewMember()
{
	for (FMGBunkerState& State : Bunkers)
	{
		if (State.CrewAlive <= 0)
		{
			continue;
		}
		--State.CrewAlive;
		if (State.Gun.IsValid())
		{
			State.Gun->SetRenderedCrewCount(FMath::Min(State.CrewAlive, 2));
		}
		if (State.CrewAlive > 0)
		{
			StartStop(State, EMGStop::Takeover);
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
		return;
	}
}

void AMGBunkerManager::ToggleDebug()
{
	bDebug = !bDebug;
	if (AllySim.IsValid())
	{
		AllySim->SetDebugDraw(bDebug);
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
