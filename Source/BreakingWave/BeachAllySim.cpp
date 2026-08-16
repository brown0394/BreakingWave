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

	Slot->Position = Position;
	Slot->HeadingYaw = 90.f + FMath::FRandRange(-Settings.WanderYawRange, Settings.WanderYawRange);
	Slot->Speed = FMath::FRandRange(Settings.AdvanceSpeedMin, Settings.AdvanceSpeedMax);
	Slot->Stance = ESimAllyStance::Advancing;
	Slot->StanceTimer = 0.f;
	Slot->WanderTimer = FMath::FRandRange(Settings.WanderIntervalMin, Settings.WanderIntervalMax);
	Slot->bAlive = true;
	Slot->Generation++;
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

int32 AAllySimManager::SelectTakeoverSlot(float AnchorY, int32& OutLadderSteps) const
{
	const float ForwardEdge = AnchorY + Settings.TakeoverForwardReach;

	float RearLimit = ForwardEdge;
	for (const FVector& Spawn : SpawnPoints)
	{
		RearLimit = FMath::Min(RearLimit, static_cast<float>(Spawn.Y));
	}
	RearLimit -= Settings.SpawnLateralSpread;

	TArray<int32, TInlineAllocator<32>> Candidates;
	OutLadderSteps = 0;

	for (;;)
	{
		const float RearEdge = AnchorY - Settings.TakeoverForwardReach - OutLadderSteps * Settings.TakeoverRearStep;

		Candidates.Reset();
		for (int32 Index = 0; Index < Allies.Num(); ++Index)
		{
			const FSimAlly& Ally = Allies[Index];
			if (Ally.bAlive && Ally.Position.Y >= RearEdge && Ally.Position.Y <= ForwardEdge)
			{
				Candidates.Add(Index);
			}
		}

		if (Candidates.Num() > 0)
		{
			return Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
		}

		if (RearEdge <= RearLimit)
		{
			return INDEX_NONE;
		}
		++OutLadderSteps;
	}
}

int32 AAllySimManager::CountAlliesInSlab(float AnchorY) const
{
	int32 Count = 0;
	for (const FSimAlly& Ally : Allies)
	{
		if (Ally.bAlive && FMath::Abs(Ally.Position.Y - AnchorY) <= Settings.TakeoverForwardReach)
		{
			++Count;
		}
	}
	return Count;
}
