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

	/** Simulated soldiers alive at once; the beach never truly empties while the spawner keeps up.
	    Decision 041 raised this from the Step 3 placeholder of 32, which left a median of ONE live
	    ally in a band spanning the whole beach width and forced the takeover to reach across it */
	UPROPERTY(EditAnywhere, Category = "AllySim")
	int32 MaxAlive = 128;

	/** Seconds of assault simulated before play begins — the player lands mid-wave, never on an empty beach */
	UPROPERTY(EditAnywhere, Category = "AllySim")
	float PreWarmSeconds = 60.f;

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

	/** World Y past which a simulated soldier despawns — the top of Zone 4, so takeover candidates exist at every depth the player can reach */
	UPROPERTY(EditAnywhere, Category = "AllySim")
	float DespawnY = 15600.f;

	/** Takeover reaches this far forward of the death point and never further (Decision 038: death must not gift ground) */
	UPROPERTY(EditAnywhere, Category = "AllySim|Takeover")
	float TakeoverForwardReach = 2000.f;

	/** Decision 041: the next man is within this of where you fell — real if one was there, manufactured if not.
	    MUST TRACK FOG VISIBILITY; it means "the man you could have seen", and fog is not built yet
	    (09_ALLY_NPC.md still lists 30 m vs 40 m as open). Re-check this number when fog lands */
	UPROPERTY(EditAnywhere, Category = "AllySim|Takeover")
	float TakeoverRadius = 3500.f;

	/** Random angles tried at each radius before the disc grows; guards against pathological geometry, not scarcity */
	UPROPERTY(EditAnywhere, Category = "AllySim|Takeover")
	int32 TakeoverPlacementAttempts = 8;

	/** When every angle fails the ground trace the disc GROWS — shrinking would dig further into whatever blocked it */
	UPROPERTY(EditAnywhere, Category = "AllySim|Takeover")
	float TakeoverRadiusGrowthFactor = 1.5f;

	UPROPERTY(EditAnywhere, Category = "AllySim|Takeover")
	int32 TakeoverRadiusGrowthSteps = 3;

	/** A placement whose ground sits further than this above or below the ground you died on is rejected.
	    The trace comes down from high above, so without this a man can be placed on a BUNKER ROOF —
	    reachable now that the +20 m forward clamp touches the defense line */
	UPROPERTY(EditAnywhere, Category = "AllySim|Takeover")
	float TakeoverMaxGroundStep = 300.f;

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

	/** Decision 041: the man you take over is inside TakeoverRadius of where you fell — a live one picked at
	    random if the disc holds any, otherwise one spawned at the disc's edge, which nobody can see happen
	    (09_ALLY_NPC.md §119). No candidate may sit further forward than TakeoverForwardReach.
	    Returns INDEX_NONE only when the ground refuses every placement at every radius. */
	int32 AcquireTakeoverAlly(const FVector& DeathPoint, bool& bOutManufactured, int32& OutDiscCandidates);

	/** Live allies inside a takeover disc centred on Anchor — the telemetry measure of whether the beach is populated enough to carry the loop */
	int32 CountAlliesInDisc(const FVector& Anchor) const;

	void SetDebugDraw(bool bEnabled) { bDebugDraw = bEnabled; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "AllySim", meta = (ShowOnlyInnerProperties))
	FAllySimSettings Settings;

	/** Where the wave comes ashore; defaults to the three landing craft (02_STATUS.md coordinate sheet) */
	UPROPERTY(EditAnywhere, Category = "AllySim")
	TArray<FVector> SpawnPoints;

private:
	void SimulateStep(float DeltaSeconds);

	void SpawnAlly();

	/** Spawns into a free slot, or failing that into the slot of the live ally furthest from EvictionAnchor —
	    he is beyond fog by construction, so nobody can see him go. Always returns a valid slot. */
	int32 SpawnAllyAt(const FVector& GroundPosition, const FVector& EvictionAnchor);

	int32 ManufactureTakeoverAlly(const FVector& DeathPoint);

	void InitialiseAlly(FSimAlly& Slot, const FVector& GroundPosition, bool bStartProne);

	int32 FindReusableSlot(const FVector& EvictionAnchor) const;

	void AdvanceAlly(FSimAlly& Ally, float DeltaSeconds);

	bool TraceGroundZ(const FVector& Position, float& OutZ) const;

	TArray<FSimAlly> Allies;

	float SpawnAccumulator = 0.f;

	bool bDebugDraw = false;
};
