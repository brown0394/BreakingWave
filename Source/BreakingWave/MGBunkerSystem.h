#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Actor.h"
#include "PlaytestRecorder.h"
#include "MGBunkerSystem.generated.h"

class AAllySimManager;
class ABreakingWaveCharacter;
class UAudioComponent;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class USoundAttenuation;
class USoundBase;

UENUM()
enum class EMGStop : uint8
{
	None,
	Reload,
	BarrelChange,
	Jam,
	Takeover
};

USTRUCT()
struct FMGAwareness
{
	GENERATED_BODY()

	float LastSeenTime = -1000.f;

	FVector LastKnownPos = FVector::ZeroVector;

	float LastExposure = 0.f;

	float BrokeCoverTime = -1000.f;

	int32 AllyGeneration = -1;
};

USTRUCT(BlueprintType)
struct FMGSettings
{
	GENERATED_BODY()

	/** Historical MG42 cyclic rate is ~20/s; the belt and heat sim decide how much of it is usable */
	UPROPERTY(EditAnywhere, Category = "MG|Fire")
	float RoundsPerSecond = 20.f;

	/** cm/s; real 7.92mm muzzle velocity ~755 m/s — travel time to Zone 0 is the dodge window (Decision 030) */
	UPROPERTY(EditAnywhere, Category = "MG|Fire")
	float MuzzleVelocity = 75500.f;

	UPROPERTY(EditAnywhere, Category = "MG|Fire")
	float BulletLifetime = 3.f;

	/** Base dispersion half-angle in degrees, before situational multipliers */
	UPROPERTY(EditAnywhere, Category = "MG|Fire")
	float BaseDispersionDeg = 0.5f;

	UPROPERTY(EditAnywhere, Category = "MG|Fire")
	float RotatingDispersionMultiplier = 4.f;

	/** Aim error (deg) above which the gun counts as mid-rotation for accuracy */
	UPROPERTY(EditAnywhere, Category = "MG|Fire")
	float RotatingAccuracyThresholdDeg = 3.f;

	/** Dispersion ramps up to MaxBurstDispersionMultiplier over this many seconds of continuous fire */
	UPROPERTY(EditAnywhere, Category = "MG|Fire")
	float BurstDispersionRampSeconds = 4.f;

	UPROPERTY(EditAnywhere, Category = "MG|Fire")
	float MaxBurstDispersionMultiplier = 2.5f;

	UPROPERTY(EditAnywhere, Category = "MG|Fire")
	float MovingTargetDispersionMultiplier = 1.8f;

	/** Speed (cm/s) above which a target counts as moving */
	UPROPERTY(EditAnywhere, Category = "MG|Fire")
	float MovingTargetSpeedThreshold = 150.f;

	UPROPERTY(EditAnywhere, Category = "MG|Fire")
	float ProneTargetDispersionMultiplier = 1.5f;

	/** Gunners aim ahead of a mover by bullet flight time × a fraction rolled in [Min, Max] per target switch — imperfect lead keeps crossing fire beatable */
	UPROPERTY(EditAnywhere, Category = "MG|Fire")
	float LeadFractionMin = 0.6f;

	UPROPERTY(EditAnywhere, Category = "MG|Fire")
	float LeadFractionMax = 1.15f;

	/** A pause longer than this starts a new burst (jam roll, dispersion ramp reset) */
	UPROPERTY(EditAnywhere, Category = "MG|Fire")
	float BurstGapSeconds = 0.4f;

	UPROPERTY(EditAnywhere, Category = "MG|Stops")
	int32 BeltSize = 250;

	/** Each gun starts with a partial belt rolled in [this, 1] × BeltSize — desyncs the battery's reload clocks */
	UPROPERTY(EditAnywhere, Category = "MG|Stops")
	float StartingBeltFractionMin = 0.35f;

	/** Each gun starts with heat rolled in [0, this] × OverheatThreshold — desyncs the barrel-change clocks */
	UPROPERTY(EditAnywhere, Category = "MG|Stops")
	float StartingHeatFractionMax = 0.5f;

	UPROPERTY(EditAnywhere, Category = "MG|Stops")
	float HeatPerShot = 1.f;

	UPROPERTY(EditAnywhere, Category = "MG|Stops")
	float HeatCoolPerSecond = 8.f;

	UPROPERTY(EditAnywhere, Category = "MG|Stops")
	float OverheatThreshold = 120.f;

	UPROPERTY(EditAnywhere, Category = "MG|Stops")
	float JamChancePerBurst = 0.02f;

	UPROPERTY(EditAnywhere, Category = "MG|Stops")
	float ReloadDurationMin = 2.f;

	UPROPERTY(EditAnywhere, Category = "MG|Stops")
	float ReloadDurationMax = 3.f;

	UPROPERTY(EditAnywhere, Category = "MG|Stops")
	float BarrelChangeDurationMin = 4.f;

	UPROPERTY(EditAnywhere, Category = "MG|Stops")
	float BarrelChangeDurationMax = 6.f;

	UPROPERTY(EditAnywhere, Category = "MG|Stops")
	float JamDurationMin = 2.f;

	UPROPERTY(EditAnywhere, Category = "MG|Stops")
	float JamDurationMax = 8.f;

	/** Decision 027: 6-man garrison; only the active pair is rendered */
	UPROPERTY(EditAnywhere, Category = "MG|Crew")
	int32 GarrisonSize = 6;

	/** Crew at or below this count reloads slower (ammo bearers dead) */
	UPROPERTY(EditAnywhere, Category = "MG|Crew")
	int32 ReducedCrewThreshold = 3;

	UPROPERTY(EditAnywhere, Category = "MG|Crew")
	float ReducedCrewReloadMultiplier = 1.8f;

	/** Last man alone: reload AND barrel change take this much longer */
	UPROPERTY(EditAnywhere, Category = "MG|Crew")
	float SoloStopMultiplier = 3.f;

	UPROPERTY(EditAnywhere, Category = "MG|Crew")
	float TakeoverDurationMin = 3.f;

	UPROPERTY(EditAnywhere, Category = "MG|Crew")
	float TakeoverDurationMax = 6.f;

	UPROPERTY(EditAnywhere, Category = "MG|Rotation")
	float MaxRotationSpeedDegPerSec = 40.f;

	/** Fractional jitter on rotation speed, resampled per target switch — transitions are never identical */
	UPROPERTY(EditAnywhere, Category = "MG|Rotation")
	float RotationSpeedVariance = 0.25f;

	/** Idle scan slew across the slit arc while the awareness set is empty (no firing) */
	UPROPERTY(EditAnywhere, Category = "MG|Rotation")
	float ScanSpeedDegPerSec = 10.f;

	/** Perception tick; doubles as the gunner's reaction time (Decision 031) */
	UPROPERTY(EditAnywhere, Category = "MG|Perception")
	float EvaluationInterval = 0.4f;

	UPROPERTY(EditAnywhere, Category = "MG|Perception")
	float VisibilityMaxRange = 50000.f;

	/** Full attention inside this half-angle around the current muzzle direction */
	UPROPERTY(EditAnywhere, Category = "MG|Perception")
	float AttentionFullAngleDeg = 15.f;

	/** Hard vision limit: the slit's half-arc around the bunker's facing */
	UPROPERTY(EditAnywhere, Category = "MG|Perception")
	float SlitArcHalfAngleDeg = 55.f;

	/** Attention floor at the edge of the slit arc; close targets still break through it */
	UPROPERTY(EditAnywhere, Category = "MG|Perception")
	float AttentionEdgeValue = 0.35f;

	UPROPERTY(EditAnywhere, Category = "MG|Perception")
	float AwarenessThreshold = 0.15f;

	/** How long the gunner keeps suppressing a position after losing sight of its occupant */
	UPROPERTY(EditAnywhere, Category = "MG|Perception")
	float AwarenessMemorySeconds = 4.f;

	/** Broke-cover priority bonus decays over this window (ladder rung 2) */
	UPROPERTY(EditAnywhere, Category = "MG|Priority")
	float BrokeCoverWindowSeconds = 2.f;

	/** New target must beat the current one by this factor — no flickering between equals */
	UPROPERTY(EditAnywhere, Category = "MG|Priority")
	float TargetSwitchMargin = 1.2f;

	/** Movement draws the gunner's eye: score scales up to (1 + this) at MovingTargetScoreReferenceSpeed */
	UPROPERTY(EditAnywhere, Category = "MG|Priority")
	float MovingTargetScoreBonus = 1.5f;

	/** Ground speed (cm/s) earning the full moving-target bonus; player sprint is 900 */
	UPROPERTY(EditAnywhere, Category = "MG|Priority")
	float MovingTargetScoreReferenceSpeed = 900.f;

	/** Score multiplier for a target another gun is already working — the pair spreads its fire instead of doubling up */
	UPROPERTY(EditAnywhere, Category = "MG|Priority")
	float SharedTargetScorePenalty = 0.5f;

	/** Bullet passing within this range of the player's head snaps a supersonic crack */
	UPROPERTY(EditAnywhere, Category = "MG|Audio")
	float CrackRadius = 1000.f;

	/** Crack volume fades from full at a head-graze down to this at CrackRadius */
	UPROPERTY(EditAnywhere, Category = "MG|Audio")
	float CrackVolumeAtEdge = 0.35f;

	UPROPERTY(EditAnywhere, Category = "MG|Audio")
	float CrackPitchVariance = 0.08f;

	/** Bullets striking ground/objects within this range of the player play an impact sound */
	UPROPERTY(EditAnywhere, Category = "MG|Audio")
	float ImpactSoundRadius = 5000.f;

	/** Impact plays at full volume inside this, then falls off toward ImpactSoundRadius */
	UPROPERTY(EditAnywhere, Category = "MG|Audio")
	float ImpactSoundInnerRadius = 400.f;

	UPROPERTY(EditAnywhere, Category = "MG|Audio")
	float ImpactPitchVariance = 0.15f;

	/** Floor between consecutive impact sounds — caps mixer load when 3 guns land 60 rounds/sec */
	UPROPERTY(EditAnywhere, Category = "MG|Audio")
	float ImpactSoundMinInterval = 0.03f;

	UPROPERTY(EditAnywhere, Category = "MG|Audio")
	float SpeedOfSound = 34300.f;
};

USTRUCT()
struct FMGBullet
{
	GENERATED_BODY()

	FVector Position = FVector::ZeroVector;

	FVector Velocity = FVector::ZeroVector;

	float RemainingLife = 0.f;

	int32 SourceBunkerIndex = 0;

	bool bCrackPlayed = false;
};

USTRUCT()
struct FMGBunkerState
{
	GENERATED_BODY()

	TWeakObjectPtr<class AMGBunkerGun> Gun;

	int32 CrewAlive = 6;

	int32 BeltRounds = 250;

	float Heat = 0.f;

	EMGStop Stop = EMGStop::None;

	float StopTimer = 0.f;

	float AimYaw = 0.f;

	float AimPitch = 0.f;

	float TargetYaw = 0.f;

	float TargetPitch = 0.f;

	/** The bunker's facing (out through the slit); center of the slit arc and the scan */
	float RestYaw = 0.f;

	float RotationSpeedJitter = 1.f;

	float LeadFraction = 1.f;

	float ScanPhase = 0.f;

	/** Ally index, PlayerTargetId, or NoTargetId */
	int32 CurrentTargetId = -2;

	float EvalTimer = 0.f;

	float FireAccumulator = 0.f;

	float BurstSeconds = 0.f;

	float TimeSinceLastShot = 100.f;

	bool bJamRolledThisBurst = false;

	bool bAudioFiring = false;

	TArray<FMGAwareness> Awareness;
};

/** Visual shell only (Decision 021): meshes, muzzle transform, audio. All behavior lives in AMGBunkerManager. */
UCLASS()
class AMGBunkerGun : public AActor
{
	GENERATED_BODY()

public:
	AMGBunkerGun();

	void SetAimAngles(float YawDegrees, float PitchDegrees);

	FVector GetMuzzleLocation() const;

	/** Fixed exterior firing point at the slit opening — does NOT rotate with the barrel.
	    Perception and bullet spawn use this so aiming off-center never fires into the
	    bunker's own walls (the visual Muzzle swings with the barrel and would). */
	FVector GetFirePosition() const;

	void SetRenderedCrewCount(int32 Count);

	void SetFiringAudioState(bool bFiring, float DelaySeconds);

	USoundBase* GetCrackSound() const { return CrackSound; }

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* YawPivot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* Barrel;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* Muzzle;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* FirePort;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* GunnerMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* LoaderMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UAudioComponent* FireLoopAudio;

	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundBase* FireLoopSound;

	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundBase* CrackSound;

private:
	FTimerHandle AudioDelayTimer;
};

UCLASS()
class AMGBunkerManager : public AActor
{
	GENERATED_BODY()

public:
	static constexpr int32 PlayerTargetId = -1;

	static constexpr int32 NoTargetId = -2;

	AMGBunkerManager();

	virtual void Tick(float DeltaSeconds) override;

	void ToggleNoDamage();

	void KillGunCrewMember();

	void ToggleDebug();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "MG", meta = (ShowOnlyInnerProperties))
	FMGSettings Settings;

	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundBase* ImpactSound = nullptr;

private:
	void UpdateBunker(FMGBunkerState& State, int32 BunkerIndex, float DeltaSeconds);

	void EvaluatePerception(FMGBunkerState& State);

	float ComputeVisibility(const FMGBunkerState& State, const FVector& MuzzlePos,
		const FVector AimPoints[3], float& OutExposure) const;

	float ScoreTarget(const FMGBunkerState& State, int32 TargetId, float Now) const;

	void SelectTarget(FMGBunkerState& State, int32 BunkerIndex, float Now);

	bool IsTargetedByAnotherGun(int32 BunkerIndex, int32 TargetId) const;

	void UpdateRotation(FMGBunkerState& State, float DeltaSeconds);

	void UpdateFiring(FMGBunkerState& State, int32 BunkerIndex, float DeltaSeconds);

	void FireRound(FMGBunkerState& State, int32 BunkerIndex);

	void StartStop(FMGBunkerState& State, int32 BunkerIndex, EMGStop Stop);

	float StopDuration(const FMGBunkerState& State, EMGStop Stop) const;

	void UpdateBullets(float DeltaSeconds);

	void HandlePlayerHit(int32 SourceBunkerIndex, const FVector& HitPoint);

	void SyncFiringAudio(FMGBunkerState& State, bool bActuallyFiring);

	bool IsTargetAlive(int32 TargetId) const;

	FVector GetTargetPosition(int32 TargetId) const;

	FVector GetTargetVelocity(int32 TargetId) const;

	bool IsTargetProne(int32 TargetId) const;

	void GetTargetAimPoints(int32 TargetId, FVector OutPoints[3]) const;

	FMGAwareness& AwarenessFor(FMGBunkerState& State, int32 TargetId);

	const FMGAwareness& AwarenessFor(const FMGBunkerState& State, int32 TargetId) const;

	bool IsAware(const FMGBunkerState& State, int32 TargetId, float Now) const;

	ABreakingWaveCharacter* GetPlayerCharacter() const;

	void DrawDebugState() const;

	UPROPERTY(Transient)
	USoundAttenuation* ImpactAttenuation = nullptr;

	UPROPERTY(Transient)
	USoundAttenuation* CrackAttenuation = nullptr;

	float LastImpactSoundTime = -1.f;

	TArray<FMGBunkerState> Bunkers;

	TArray<FMGBullet> Bullets;

	FPlaytestRecorder Recorder;

	TWeakObjectPtr<AAllySimManager> AllySim;

	mutable TWeakObjectPtr<ABreakingWaveCharacter> CachedPlayer;

	FTransform PlayerSpawnTransform;

	bool bPlayerSpawnCaptured = false;

	bool bNoDamage = false;

	bool bDebug = false;
};
