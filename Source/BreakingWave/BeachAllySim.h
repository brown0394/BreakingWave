#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BeachAllySim.generated.h"

UENUM()
enum class ESimAllyStance : uint8
{
	Advancing,
	Prone
};

USTRUCT()
struct FSimAlly
{
	GENERATED_BODY()

	FVector Position = FVector::ZeroVector;

	float HeadingYaw = 90.f;

	float Speed = 400.f;

	ESimAllyStance Stance = ESimAllyStance::Advancing;

	float StanceTimer = 0.f;

	float WanderTimer = 0.f;

	bool bAlive = false;

	/** Bumped on every slot reuse so consumers (MG awareness) can detect a fresh soldier in an old slot */
	int32 Generation = 0;

	/** Reserved for the 09_ALLY_NPC.md personality types; unused in the Step 3 simulation */
	uint8 PersonalityType = 0;
};

USTRUCT(BlueprintType)
struct FAllySimSettings
{
	GENERATED_BODY()

	/** Simulated soldiers alive at once; the beach never truly empties while the spawner keeps up */
	UPROPERTY(EditAnywhere, Category = "AllySim")
	int32 MaxAlive = 32;

	UPROPERTY(EditAnywhere, Category = "AllySim")
	float SpawnsPerSecond = 1.5f;

	/** Lateral scatter around a spawn point, in cm */
	UPROPERTY(EditAnywhere, Category = "AllySim")
	float SpawnLateralSpread = 900.f;

	UPROPERTY(EditAnywhere, Category = "AllySim")
	float AdvanceSpeedMin = 300.f;

	UPROPERTY(EditAnywhere, Category = "AllySim")
	float AdvanceSpeedMax = 550.f;

	UPROPERTY(EditAnywhere, Category = "AllySim")
	float WanderIntervalMin = 1.5f;

	UPROPERTY(EditAnywhere, Category = "AllySim")
	float WanderIntervalMax = 4.f;

	/** Half-range of heading jitter around straight-inland, in degrees */
	UPROPERTY(EditAnywhere, Category = "AllySim")
	float WanderYawRange = 30.f;

	/** Chance per wander re-roll that the soldier drops prone for a pause (exercises the MG's broke-cover trigger) */
	UPROPERTY(EditAnywhere, Category = "AllySim")
	float PronePauseChance = 0.25f;

	UPROPERTY(EditAnywhere, Category = "AllySim")
	float PronePauseMin = 1.5f;

	UPROPERTY(EditAnywhere, Category = "AllySim")
	float PronePauseMax = 4.f;

	/** World Y past which a simulated soldier despawns (short of the Zone 3 infantry fight, which is a later system) */
	UPROPERTY(EditAnywhere, Category = "AllySim")
	float DespawnY = 5000.f;

	UPROPERTY(EditAnywhere, Category = "AllySim")
	float StandingHeight = 180.f;

	UPROPERTY(EditAnywhere, Category = "AllySim")
	float ProneHeight = 40.f;

	UPROPERTY(EditAnywhere, Category = "AllySim")
	float BodyRadius = 35.f;
};

UCLASS()
class AAllySimManager : public AActor
{
	GENERATED_BODY()

public:
	AAllySimManager();

	virtual void Tick(float DeltaSeconds) override;

	TArray<FSimAlly>& GetAllies() { return Allies; }

	const FAllySimSettings& GetSettings() const { return Settings; }

	float GetBodyHeight(const FSimAlly& Ally) const;

	void GetAimPoints(const FSimAlly& Ally, FVector OutPoints[3]) const;

	void KillAlly(int32 Index);

	void SetDebugDraw(bool bEnabled) { bDebugDraw = bEnabled; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "AllySim", meta = (ShowOnlyInnerProperties))
	FAllySimSettings Settings;

	/** Where the wave comes ashore; defaults to the three landing craft (02_STATUS.md coordinate sheet) */
	UPROPERTY(EditAnywhere, Category = "AllySim")
	TArray<FVector> SpawnPoints;

private:
	void SpawnAlly();

	void AdvanceAlly(FSimAlly& Ally, float DeltaSeconds);

	bool TraceGroundZ(const FVector& Position, float& OutZ) const;

	TArray<FSimAlly> Allies;

	float SpawnAccumulator = 0.f;

	bool bDebugDraw = false;
};
