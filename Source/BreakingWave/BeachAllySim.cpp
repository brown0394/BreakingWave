#include "BeachAllySim.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"

AAllySimManager::AAllySimManager()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnPoints = {
		FVector(-27400.f, -23400.f, 0.f),
		FVector(600.f, -23400.f, 0.f),
		FVector(28600.f, -23400.f, 0.f)
	};
}

void AAllySimManager::BeginPlay()
{
	Super::BeginPlay();
	Allies.SetNum(Settings.MaxAlive);

	const float PreWarmStepSeconds = 0.25f;
	for (float Simulated = 0.f; Simulated < Settings.PreWarmSeconds; Simulated += PreWarmStepSeconds)
	{
		SimulateStep(PreWarmStepSeconds);
	}
}

void AAllySimManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SimulateStep(DeltaSeconds);

	if (bDebugDraw)
	{
		for (const FSimAlly& Ally : Allies)
		{
			if (!Ally.bAlive)
			{
				continue;
			}
			const float Height = GetBodyHeight(Ally);
			const FVector Center = Ally.Position + FVector(0.f, 0.f, Height * 0.5f);
			DrawDebugCapsule(GetWorld(), Center, Height * 0.5f, Settings.BodyRadius,
				FQuat::Identity, FColor::Green, false, -1.f, 0, 1.5f);
		}
	}
}

void AAllySimManager::SimulateStep(float DeltaSeconds)
{
	SpawnAccumulator += Settings.SpawnsPerSecond * DeltaSeconds;
	while (SpawnAccumulator >= 1.f)
	{
		SpawnAccumulator -= 1.f;
		SpawnAlly();
	}

	for (FSimAlly& Ally : Allies)
	{
		if (!Ally.bAlive)
		{
			continue;
		}
		AdvanceAlly(Ally, DeltaSeconds);
		if (Ally.Position.Y > Settings.DespawnY)
		{
			Ally.bAlive = false;
		}
	}
}

void AAllySimManager::SpawnAlly()
{
	if (SpawnPoints.Num() == 0)
	{
		return;
	}

	FSimAlly* Slot = nullptr;
	for (FSimAlly& Ally : Allies)
	{
		if (!Ally.bAlive)
		{
			Slot = &Ally;
			break;
		}
	}
	if (Slot == nullptr)
	{
		return;
	}

	const FVector& Base = SpawnPoints[FMath::RandRange(0, SpawnPoints.Num() - 1)];
	FVector Position = Base + FVector(FMath::FRandRange(-Settings.SpawnLateralSpread, Settings.SpawnLateralSpread),
		FMath::FRandRange(0.f, 500.f), 0.f);

	float GroundZ;
	if (!TraceGroundZ(Position, GroundZ))
	{
		return;
	}
	Position.Z = GroundZ;

	InitialiseAlly(*Slot, Position, false);
}

int32 AAllySimManager::SpawnAllyAt(const FVector& GroundPosition, const FVector& EvictionAnchor)
{
	const int32 Slot = FindReusableSlot(EvictionAnchor);
	if (Slot == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	InitialiseAlly(Allies[Slot], GroundPosition, FMath::FRand() < Settings.PronePauseChance);
	return Slot;
}

void AAllySimManager::InitialiseAlly(FSimAlly& Slot, const FVector& GroundPosition, bool bStartProne)
{
	Slot.Position = GroundPosition;
	Slot.HeadingYaw = 90.f + FMath::FRandRange(-Settings.WanderYawRange, Settings.WanderYawRange);
	Slot.Speed = FMath::FRandRange(Settings.AdvanceSpeedMin, Settings.AdvanceSpeedMax);
	Slot.Stance = bStartProne ? ESimAllyStance::Prone : ESimAllyStance::Advancing;
	Slot.StanceTimer = bStartProne ? FMath::FRandRange(Settings.PronePauseMin, Settings.PronePauseMax) : 0.f;
	Slot.WanderTimer = FMath::FRandRange(Settings.WanderIntervalMin, Settings.WanderIntervalMax);
	Slot.bAlive = true;
	Slot.Generation++;
}

int32 AAllySimManager::FindReusableSlot(const FVector& EvictionAnchor) const
{
	int32 Furthest = INDEX_NONE;
	float FurthestDistanceSquared = -1.f;
	for (int32 Index = 0; Index < Allies.Num(); ++Index)
	{
		if (!Allies[Index].bAlive)
		{
			return Index;
		}
		const float DistanceSquared = FVector::DistSquared2D(Allies[Index].Position, EvictionAnchor);
		if (DistanceSquared > FurthestDistanceSquared)
		{
			FurthestDistanceSquared = DistanceSquared;
			Furthest = Index;
		}
	}
	return Furthest;
}

void AAllySimManager::AdvanceAlly(FSimAlly& Ally, float DeltaSeconds)
{
	if (Ally.Stance == ESimAllyStance::Prone)
	{
		Ally.StanceTimer -= DeltaSeconds;
		if (Ally.StanceTimer <= 0.f)
		{
			Ally.Stance = ESimAllyStance::Advancing;
			Ally.WanderTimer = FMath::FRandRange(Settings.WanderIntervalMin, Settings.WanderIntervalMax);
		}
		return;
	}

	Ally.WanderTimer -= DeltaSeconds;
	if (Ally.WanderTimer <= 0.f)
	{
		if (FMath::FRand() < Settings.PronePauseChance)
		{
			Ally.Stance = ESimAllyStance::Prone;
			Ally.StanceTimer = FMath::FRandRange(Settings.PronePauseMin, Settings.PronePauseMax);
			return;
		}
		Ally.HeadingYaw = 90.f + FMath::FRandRange(-Settings.WanderYawRange, Settings.WanderYawRange);
		Ally.WanderTimer = FMath::FRandRange(Settings.WanderIntervalMin, Settings.WanderIntervalMax);
	}

	const float HeadingRad = FMath::DegreesToRadians(Ally.HeadingYaw);
	Ally.Position += FVector(FMath::Cos(HeadingRad), FMath::Sin(HeadingRad), 0.f) * Ally.Speed * DeltaSeconds;

	float GroundZ;
	if (TraceGroundZ(Ally.Position, GroundZ))
	{
		Ally.Position.Z = GroundZ;
	}
}

bool AAllySimManager::TraceGroundZ(const FVector& Position, float& OutZ) const
{
	FHitResult Hit;
	const FVector Start = Position + FVector(0.f, 0.f, 5000.f);
	const FVector End = Position - FVector(0.f, 0.f, 5000.f);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
	{
		OutZ = Hit.ImpactPoint.Z;
		return true;
	}
	return false;
}

float AAllySimManager::GetBodyHeight(const FSimAlly& Ally) const
{
	return Ally.Stance == ESimAllyStance::Prone ? Settings.ProneHeight : Settings.StandingHeight;
}

void AAllySimManager::GetAimPoints(const FSimAlly& Ally, FVector OutPoints[3]) const
{
	const float Height = GetBodyHeight(Ally);
	OutPoints[0] = Ally.Position + FVector(0.f, 0.f, Height * 0.9f);
	OutPoints[1] = Ally.Position + FVector(0.f, 0.f, Height * 0.6f);
	OutPoints[2] = Ally.Position + FVector(0.f, 0.f, Height * 0.3f);
}

void AAllySimManager::KillAlly(int32 Index)
{
	if (Allies.IsValidIndex(Index))
	{
		Allies[Index].bAlive = false;
	}
}

int32 AAllySimManager::AcquireTakeoverAlly(const FVector& DeathPoint, bool& bOutManufactured, int32& OutDiscCandidates)
{
	bOutManufactured = false;

	const float ForwardEdge = DeathPoint.Y + Settings.TakeoverForwardReach;
	const float RadiusSquared = FMath::Square(Settings.TakeoverRadius);

	TArray<int32, TInlineAllocator<16>> Candidates;
	for (int32 Index = 0; Index < Allies.Num(); ++Index)
	{
		const FSimAlly& Ally = Allies[Index];
		if (Ally.bAlive && Ally.Position.Y <= ForwardEdge
			&& FVector::DistSquared2D(Ally.Position, DeathPoint) <= RadiusSquared)
		{
			Candidates.Add(Index);
		}
	}

	OutDiscCandidates = Candidates.Num();
	if (Candidates.Num() > 0)
	{
		return Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	}

	bOutManufactured = true;
	return ManufactureTakeoverAlly(DeathPoint);
}

int32 AAllySimManager::ManufactureTakeoverAlly(const FVector& DeathPoint)
{
	const float ForwardEdge = DeathPoint.Y + Settings.TakeoverForwardReach;
	float Radius = Settings.TakeoverRadius;

	float AnchorGroundZ;
	const bool bHaveAnchorGround = TraceGroundZ(DeathPoint, AnchorGroundZ);

	for (int32 Growth = 0; Growth <= Settings.TakeoverRadiusGrowthSteps; ++Growth)
	{
		for (int32 Attempt = 0; Attempt < Settings.TakeoverPlacementAttempts; ++Attempt)
		{
			const float Angle = FMath::FRandRange(0.f, UE_TWO_PI);
			FVector Position = DeathPoint
				+ FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
			Position.Y = FMath::Min(Position.Y, ForwardEdge);

			float GroundZ;
			if (!TraceGroundZ(Position, GroundZ))
			{
				continue;
			}
			if (bHaveAnchorGround
				&& FMath::Abs(GroundZ - AnchorGroundZ) > Settings.TakeoverMaxGroundStep)
			{
				continue;
			}
			Position.Z = GroundZ;

			return SpawnAllyAt(Position, DeathPoint);
		}
		Radius *= Settings.TakeoverRadiusGrowthFactor;
	}

	return INDEX_NONE;
}

int32 AAllySimManager::CountAlliesInDisc(const FVector& Anchor) const
{
	const float RadiusSquared = FMath::Square(Settings.TakeoverRadius);
	int32 Count = 0;
	for (const FSimAlly& Ally : Allies)
	{
		if (Ally.bAlive && FVector::DistSquared2D(Ally.Position, Anchor) <= RadiusSquared)
		{
			++Count;
		}
	}
	return Count;
}
