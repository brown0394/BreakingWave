#include "BeachInfantrySystem.h"

#include "Animation/AnimSequence.h"
#include "BeachAllySim.h"
#include "BreakingWaveCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "MGBunkerSystem.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AInfantrySoldier::AInfantrySoldier()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	BodyMesh->SetupAttachment(Root);
	BodyMesh->SetCollisionProfileName(FName("NoCollision"));
	BodyMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyFinder(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (BodyFinder.Succeeded())
	{
		BodyMesh->SetSkeletalMesh(BodyFinder.Object);
	}
}

AInfantryManager::AInfantryManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AInfantryManager::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<AMGBunkerManager> It(GetWorld()); It; ++It)
	{
		BunkerManager = *It;
		break;
	}
	for (TActorIterator<AAllySimManager> It(GetWorld()); It; ++It)
	{
		AllySim = *It;
		break;
	}

	for (TActorIterator<AInfantrySoldier> It(GetWorld()); It; ++It)
	{
		FInfantrySoldierState Soldier;
		Soldier.Shell = *It;
		Soldier.RoundsInMag = Settings.Rifle.MagazineSize;
		StartCover(Soldier, FMath::FRandRange(0.f, Settings.CoverWaitMax));
		Soldiers.Add(Soldier);
	}

	if (ShotSound == nullptr)
	{
		ShotSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/RifleShotEnemy.RifleShotEnemy"));
	}
	if (BoltCycleSound == nullptr)
	{
		BoltCycleSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/RifleBoltCycle.RifleBoltCycle"));
	}

	if (CoverIdleAnim == nullptr)
	{
		CoverIdleAnim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/AnimStarterPack/Retarget/Crouch_Idle_Rifle_Hip_UE5.Crouch_Idle_Rifle_Hip_UE5"));
	}
	if (RiseAnim == nullptr)
	{
		RiseAnim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/AnimStarterPack/Retarget/Crouch_to_Stand_Rifle_Ironsights_UE5.Crouch_to_Stand_Rifle_Ironsights_UE5"));
	}
	if (AimIdleAnim == nullptr)
	{
		AimIdleAnim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/AnimStarterPack/Retarget/Idle_Rifle_Ironsights_UE5.Idle_Rifle_Ironsights_UE5"));
	}
	if (FireAnim == nullptr)
	{
		FireAnim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/AnimStarterPack/Retarget/Fire_Rifle_Ironsights_UE5.Fire_Rifle_Ironsights_UE5"));
	}
	if (DropAnim == nullptr)
	{
		DropAnim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/AnimStarterPack/Retarget/Stand_to_Crouch_Rifle_Ironsights_UE5.Stand_to_Crouch_Rifle_Ironsights_UE5"));
	}

	ShotAttenuation = NewObject<USoundAttenuation>(this);
	ShotAttenuation->Attenuation.AttenuationShapeExtents = FVector(400.f, 0.f, 0.f);
	ShotAttenuation->Attenuation.FalloffDistance = Settings.ShotSoundFalloffDistance;

	for (FInfantrySoldierState& Soldier : Soldiers)
	{
		PlaySoldierAnim(Soldier, CoverIdleAnim, true);
	}
}

void AInfantryManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (int32 i = 0; i < Soldiers.Num(); ++i)
	{
		UpdateSoldier(Soldiers[i], i, DeltaSeconds);
	}

	if (bDebug)
	{
		DrawDebugState();
	}
}

void AInfantryManager::UpdateSoldier(FInfantrySoldierState& Soldier, int32 Index, float DeltaSeconds)
{
	if (!Soldier.bAlive || !Soldier.Shell.IsValid())
	{
		return;
	}

	if (Soldier.BoltSoundCountdown > 0.f)
	{
		Soldier.BoltSoundCountdown -= DeltaSeconds;
		if (Soldier.BoltSoundCountdown <= 0.f && BoltCycleSound != nullptr)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), BoltCycleSound, SoldierEyePosition(Soldier),
				FRotator::ZeroRotator, 1.f, FMath::FRandRange(0.94f, 1.06f), 0.f, ShotAttenuation);
		}
	}

	Soldier.PhaseTimer -= DeltaSeconds;

	switch (Soldier.Phase)
	{
	case EInfantryPhase::Cover:
		if (Soldier.PhaseTimer <= 0.f)
		{
			if (Soldier.bReloadOnNextCover)
			{
				Soldier.bReloadOnNextCover = false;
				Soldier.RoundsInMag = Settings.Rifle.MagazineSize;
			}
			StartRising(Soldier);
		}
		break;

	case EInfantryPhase::Rising:
		if (Soldier.PhaseTimer <= 0.f)
		{
			SelectSoldierTarget(Soldier);
			StartAiming(Soldier, FMath::FRandRange(Settings.AimSecondsMin, Settings.AimSecondsMax));
		}
		break;

	case EInfantryPhase::Aiming:
		Soldier.EvalTimer -= DeltaSeconds;
		if (Soldier.EvalTimer <= 0.f)
		{
			Soldier.EvalTimer = Settings.EvaluationInterval;
			if (Soldier.CurrentTargetId == NoTargetId || !IsTargetAlive(Soldier.CurrentTargetId))
			{
				SelectSoldierTarget(Soldier);
			}
		}
		if (Soldier.PhaseTimer <= 0.f)
		{
			if (Soldier.CurrentTargetId != NoTargetId && IsTargetAlive(Soldier.CurrentTargetId)
				&& Soldier.RoundsInMag > 0)
			{
				FireSoldierRound(Soldier, Index);
			}
			else
			{
				StartDropping(Soldier);
			}
		}
		break;

	case EInfantryPhase::Firing:
		if (Soldier.PhaseTimer <= 0.f)
		{
			if (Soldier.ShotsRemaining > 0 && Soldier.RoundsInMag > 0
				&& Soldier.CurrentTargetId != NoTargetId && IsTargetAlive(Soldier.CurrentTargetId))
			{
				StartAiming(Soldier, Settings.AimBetweenShots);
			}
			else
			{
				if (Soldier.RoundsInMag <= 0)
				{
					Soldier.bReloadOnNextCover = true;
				}
				StartDropping(Soldier);
			}
		}
		break;

	case EInfantryPhase::Dropping:
		if (Soldier.PhaseTimer <= 0.f)
		{
			const float ReloadExtra = Soldier.bReloadOnNextCover ? Settings.Rifle.ReloadSeconds : 0.f;
			StartCover(Soldier, ReloadExtra);
			PlaySoldierAnim(Soldier, CoverIdleAnim, true);
		}
		break;

	default:
		break;
	}
}

void AInfantryManager::StartCover(FInfantrySoldierState& Soldier, float ExtraWait)
{
	Soldier.Phase = EInfantryPhase::Cover;
	Soldier.PhaseTimer = FMath::FRandRange(Settings.CoverWaitMin, Settings.CoverWaitMax) + ExtraWait;
	Soldier.CurrentTargetId = NoTargetId;
}

void AInfantryManager::StartRising(FInfantrySoldierState& Soldier)
{
	Soldier.Phase = EInfantryPhase::Rising;
	Soldier.PhaseTimer = Settings.RiseSeconds;
	Soldier.ShotsRemaining = FMath::RandRange(Settings.ShotsPerExposureMin, Settings.ShotsPerExposureMax);
	Soldier.LeadFraction = FMath::FRandRange(Settings.LeadFractionMin, Settings.LeadFractionMax);
	PlaySoldierAnim(Soldier, RiseAnim, false, Settings.RiseSeconds);
}

void AInfantryManager::StartAiming(FInfantrySoldierState& Soldier, float AimSeconds)
{
	Soldier.Phase = EInfantryPhase::Aiming;
	Soldier.PhaseTimer = AimSeconds;
	Soldier.EvalTimer = 0.f;
	PlaySoldierAnim(Soldier, AimIdleAnim, true);
}

void AInfantryManager::StartDropping(FInfantrySoldierState& Soldier)
{
	Soldier.Phase = EInfantryPhase::Dropping;
	Soldier.PhaseTimer = Settings.DropSeconds;
	Soldier.CurrentTargetId = NoTargetId;
	PlaySoldierAnim(Soldier, DropAnim, false, Settings.DropSeconds);
}

void AInfantryManager::FireSoldierRound(FInfantrySoldierState& Soldier, int32 Index)
{
	Soldier.Phase = EInfantryPhase::Firing;
	Soldier.PhaseTimer = Settings.Rifle.FireIntervalSeconds;
	--Soldier.ShotsRemaining;
	--Soldier.RoundsInMag;
	Soldier.BoltSoundCountdown = Settings.BoltSoundDelay;

	const FVector EyePos = SoldierEyePosition(Soldier);
	const FVector TargetPos = GetTargetPosition(Soldier.CurrentTargetId);
	const float Distance = FVector::Dist(EyePos, TargetPos);

	const float FlightTime = Distance / FMath::Max(Settings.Rifle.MuzzleVelocity, 1.f);
	const FVector LeadTarget = TargetPos + GetTargetVelocity(Soldier.CurrentTargetId) * FlightTime * Soldier.LeadFraction;

	const float SpreadAlpha = FMath::Clamp(Distance / Settings.SpreadFarDistance, 0.f, 1.f);
	const float SpreadDeg = FMath::Lerp(Settings.SpreadNearDeg, Settings.SpreadFarDeg, SpreadAlpha);

	const FVector AimDir = (LeadTarget - EyePos).GetSafeNormal();
	const FVector ShotDir = FMath::VRandCone(AimDir, FMath::DegreesToRadians(SpreadDeg));

	if (BunkerManager.IsValid())
	{
		BunkerManager->SpawnBullet(EMGBulletSource::InfantryRifle, Index,
			EyePos + AimDir * 60.f, ShotDir * Settings.Rifle.MuzzleVelocity, Soldier.CurrentTargetId);
	}

	if (ShotSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShotSound, EyePos,
			FRotator::ZeroRotator, 1.f, FMath::FRandRange(0.92f, 1.08f), 0.f, ShotAttenuation);
	}
	PlaySoldierAnim(Soldier, FireAnim, false);

	if (Soldier.Shell.IsValid())
	{
		const FVector Flat = FVector(AimDir.X, AimDir.Y, 0.f).GetSafeNormal();
		if (!Flat.IsNearlyZero())
		{
			Soldier.Shell->SetActorRotation(Flat.Rotation());
		}
	}
}

void AInfantryManager::SelectSoldierTarget(FInfantrySoldierState& Soldier)
{
	Soldier.CurrentTargetId = NoTargetId;
	float BestScore = 0.f;

	const int32 AllyCount = AllySim.IsValid() ? AllySim->GetAllies().Num() : 0;
	for (int32 TargetId = -1; TargetId < AllyCount; ++TargetId)
	{
		if (!IsTargetAlive(TargetId))
		{
			continue;
		}
		const FVector TargetPos = GetTargetPosition(TargetId);
		const float Distance = FVector::Dist(SoldierEyePosition(Soldier), TargetPos);
		if (Distance > Settings.MaxEngagementRange)
		{
			continue;
		}
		if (!HasLineOfSight(Soldier, TargetPos))
		{
			continue;
		}

		float Score = 100.f * (1.f - Distance / Settings.MaxEngagementRange);
		const float GroundSpeed = GetTargetVelocity(TargetId).Size2D();
		const float SpeedNorm = FMath::Clamp(GroundSpeed / Settings.MovingTargetScoreReferenceSpeed, 0.f, 1.f);
		Score *= 1.f + Settings.MovingTargetScoreBonus * SpeedNorm;
		if (TargetId == PlayerTargetId)
		{
			Score *= Settings.PlayerTargetScoreMultiplier;
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			Soldier.CurrentTargetId = TargetId;
		}
	}
}

void AInfantryManager::FlinchSoldier(FInfantrySoldierState& Soldier)
{
	if (!Soldier.bAlive)
	{
		return;
	}
	const float ExtraWait = FMath::FRandRange(Settings.FlinchExtraCoverWaitMin, Settings.FlinchExtraCoverWaitMax);
	switch (Soldier.Phase)
	{
	case EInfantryPhase::Rising:
	case EInfantryPhase::Aiming:
	case EInfantryPhase::Firing:
		StartDropping(Soldier);
		Soldier.PhaseTimer = Settings.DropSeconds;
		Soldier.bReloadOnNextCover = Soldier.bReloadOnNextCover || Soldier.RoundsInMag <= 0;
		break;
	case EInfantryPhase::Cover:
	case EInfantryPhase::Dropping:
		Soldier.PhaseTimer += ExtraWait;
		break;
	default:
		break;
	}
}

bool AInfantryManager::IsSoldierAlive(int32 Index) const
{
	return Soldiers.IsValidIndex(Index) && Soldiers[Index].bAlive;
}

bool AInfantryManager::GetSoldierBodySegment(int32 Index, FVector& OutBottom, FVector& OutTop, float& OutRadius) const
{
	if (!Soldiers.IsValidIndex(Index) || !Soldiers[Index].bAlive || !Soldiers[Index].Shell.IsValid())
	{
		return false;
	}
	const FInfantrySoldierState& Soldier = Soldiers[Index];
	const FVector Feet = Soldier.Shell->GetActorLocation();
	const bool bRisen = Soldier.Phase == EInfantryPhase::Aiming || Soldier.Phase == EInfantryPhase::Firing
		|| Soldier.Phase == EInfantryPhase::Rising;
	const float Height = bRisen ? Settings.RisenBodyHeight : Settings.CoverBodyHeight;
	OutRadius = Settings.BodyRadius;
	OutBottom = Feet + FVector(0.f, 0.f, Settings.BodyRadius);
	OutTop = Feet + FVector(0.f, 0.f, FMath::Max(Height - Settings.BodyRadius, Settings.BodyRadius + 1.f));
	return true;
}

void AInfantryManager::NotifySoldierHit(int32 Index, const FVector& HitPoint)
{
	if (!Soldiers.IsValidIndex(Index) || !Soldiers[Index].bAlive)
	{
		return;
	}
	FInfantrySoldierState& Soldier = Soldiers[Index];
	Soldier.bAlive = false;
	Soldier.Phase = EInfantryPhase::Dead;

	if (Soldier.Shell.IsValid() && Soldier.Shell->BodyMesh != nullptr)
	{
		USkeletalMeshComponent* Body = Soldier.Shell->BodyMesh;
		Body->Stop();
		Body->SetCollisionProfileName(FName("Ragdoll"));
		Body->SetSimulatePhysics(true);
	}

	const FVector DeathPos = Soldier.Shell.IsValid() ? Soldier.Shell->GetActorLocation() : HitPoint;
	for (FInfantrySoldierState& Other : Soldiers)
	{
		if (&Other != &Soldier && Other.bAlive && Other.Shell.IsValid()
			&& FVector::Dist(Other.Shell->GetActorLocation(), DeathPos) < Settings.ComradeDeathFlinchRadius)
		{
			FlinchSoldier(Other);
		}
	}
}

void AInfantryManager::NotifyImpactNear(const FVector& ImpactPoint)
{
	for (FInfantrySoldierState& Soldier : Soldiers)
	{
		if (Soldier.bAlive && Soldier.Shell.IsValid()
			&& FVector::Dist(Soldier.Shell->GetActorLocation(), ImpactPoint) < Settings.FlinchRadius)
		{
			FlinchSoldier(Soldier);
		}
	}
}

bool AInfantryManager::IsTargetAlive(int32 TargetId) const
{
	if (TargetId == PlayerTargetId)
	{
		return GetPlayerCharacter() != nullptr && GetWorld()->GetTimeSeconds() >= PlayerAcquireBlockedUntil;
	}
	if (TargetId >= 0 && AllySim.IsValid() && AllySim->GetAllies().IsValidIndex(TargetId))
	{
		return AllySim->GetAllies()[TargetId].bAlive;
	}
	return false;
}

FVector AInfantryManager::GetTargetPosition(int32 TargetId) const
{
	if (TargetId == PlayerTargetId)
	{
		const ABreakingWaveCharacter* Player = GetPlayerCharacter();
		return Player != nullptr ? Player->GetActorLocation() : FVector::ZeroVector;
	}
	const FSimAlly& Ally = AllySim->GetAllies()[TargetId];
	return Ally.Position + FVector(0.f, 0.f, AllySim->GetBodyHeight(Ally) * 0.6f);
}

FVector AInfantryManager::GetTargetVelocity(int32 TargetId) const
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

FVector AInfantryManager::SoldierEyePosition(const FInfantrySoldierState& Soldier) const
{
	if (!Soldier.Shell.IsValid())
	{
		return FVector::ZeroVector;
	}
	return Soldier.Shell->GetActorLocation() + FVector(0.f, 0.f, Settings.RisenEyeHeight);
}

bool AInfantryManager::HasLineOfSight(const FInfantrySoldierState& Soldier, const FVector& TargetPoint) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(InfantryLOS), false);
	Params.AddIgnoredActor(Soldier.Shell.Get());
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, SoldierEyePosition(Soldier), TargetPoint, ECC_Visibility, Params))
	{
		return true;
	}
	if (const ABreakingWaveCharacter* Player = GetPlayerCharacter())
	{
		if (Hit.GetActor() == Player)
		{
			return true;
		}
	}
	return false;
}

void AInfantryManager::PlaySoldierAnim(FInfantrySoldierState& Soldier, UAnimSequence* Anim, bool bLoop, float FitToSeconds)
{
	if (Anim == nullptr || !Soldier.Shell.IsValid() || Soldier.Shell->BodyMesh == nullptr)
	{
		return;
	}
	USkeletalMeshComponent* Body = Soldier.Shell->BodyMesh;
	Body->PlayAnimation(Anim, bLoop);
	if (FitToSeconds > 0.f && Anim->GetPlayLength() > 0.f)
	{
		Body->SetPlayRate(Anim->GetPlayLength() / FitToSeconds);
	}
	else
	{
		Body->SetPlayRate(1.f);
	}
}

ABreakingWaveCharacter* AInfantryManager::GetPlayerCharacter() const
{
	ABreakingWaveCharacter* Player = Cast<ABreakingWaveCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	return (Player != nullptr && !Player->IsDead()) ? Player : nullptr;
}

void AInfantryManager::NotifyPlayerTakeover(float BlockedUntilTime)
{
	PlayerAcquireBlockedUntil = BlockedUntilTime;

	for (FInfantrySoldierState& Soldier : Soldiers)
	{
		if (Soldier.CurrentTargetId == PlayerTargetId)
		{
			Soldier.CurrentTargetId = NoTargetId;
		}
	}
}

void AInfantryManager::DrawDebugState() const
{
#if ENABLE_DRAW_DEBUG
	for (const FInfantrySoldierState& Soldier : Soldiers)
	{
		if (!Soldier.Shell.IsValid())
		{
			continue;
		}
		const FVector Pos = Soldier.Shell->GetActorLocation() + FVector(0.f, 0.f, 220.f);
		const TCHAR* PhaseName = TEXT("?");
		switch (Soldier.Phase)
		{
		case EInfantryPhase::Cover: PhaseName = TEXT("cover"); break;
		case EInfantryPhase::Rising: PhaseName = TEXT("rise"); break;
		case EInfantryPhase::Aiming: PhaseName = TEXT("aim"); break;
		case EInfantryPhase::Firing: PhaseName = TEXT("fire"); break;
		case EInfantryPhase::Dropping: PhaseName = TEXT("drop"); break;
		case EInfantryPhase::Dead: PhaseName = TEXT("dead"); break;
		}
		DrawDebugString(GetWorld(), Pos,
			FString::Printf(TEXT("%s tgt=%d mag=%d"), PhaseName, Soldier.CurrentTargetId, Soldier.RoundsInMag),
			nullptr, Soldier.bAlive ? FColor::Yellow : FColor::Red, 0.f, true);
	}
#endif
}
