#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RifleProfile.h"
#include "BeachInfantrySystem.generated.h"

class AAllySimManager;
class ABreakingWaveCharacter;
class AMGBunkerManager;
class UAnimSequence;
class USkeletalMeshComponent;
class USoundAttenuation;
class USoundBase;

/** The rise/fire/drop cycle (08_ENEMY_AI.md): a person who shoots until he's scared, then hides */
UENUM()
enum class EInfantryPhase : uint8
{
	Cover,
	Rising,
	Aiming,
	Firing,
	Dropping,
	Dead,
};

/** ALL NUMBERS TENTATIVE (08_ENEMY_AI.md infantry; relocation layer deliberately deferred) */
USTRUCT()
struct FInfantrySettings
{
	GENERATED_BODY()

	/** Fog stand-in: infantry ignore anything beyond this until real fog owns the vision cap */
	UPROPERTY(EditAnywhere, Category = "Infantry|Perception")
	float MaxEngagementRange = 12000.f;

	/** Target re-check cadence while aiming; doubles as reaction time like the MG's */
	UPROPERTY(EditAnywhere, Category = "Infantry|Perception")
	float EvaluationInterval = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Priority")
	float MovingTargetScoreBonus = 1.f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Priority")
	float MovingTargetScoreReferenceSpeed = 900.f;

	/** Same philosophy as the MG knob: the player must feel personally hunted. UNTUNED */
	UPROPERTY(EditAnywhere, Category = "Infantry|Priority")
	float PlayerTargetScoreMultiplier = 3.f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Cycle")
	float CoverWaitMin = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Cycle")
	float CoverWaitMax = 6.f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Cycle")
	float RiseSeconds = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Cycle")
	float DropSeconds = 0.5f;

	/** He does not snap-fire: aim delay before the first round of an exposure */
	UPROPERTY(EditAnywhere, Category = "Infantry|Cycle")
	float AimSecondsMin = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Cycle")
	float AimSecondsMax = 1.6f;

	/** Re-settle time between shots of one exposure, after the bolt cycle */
	UPROPERTY(EditAnywhere, Category = "Infantry|Cycle")
	float AimBetweenShots = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Cycle")
	int32 ShotsPerExposureMin = 1;

	UPROPERTY(EditAnywhere, Category = "Infantry|Cycle")
	int32 ShotsPerExposureMax = 3;

	/** The defenders' bolt-action rifle (second data row of the shared rifle system) */
	UPROPERTY(EditAnywhere, Category = "Infantry|Rifle")
	FRifleProfile Rifle;

	/** Aimed-shot dispersion at point blank; lerps to SpreadFarDeg at SpreadFarDistance */
	UPROPERTY(EditAnywhere, Category = "Infantry|Rifle")
	float SpreadNearDeg = 1.f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Rifle")
	float SpreadFarDeg = 5.f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Rifle")
	float SpreadFarDistance = 15000.f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Rifle")
	float LeadFractionMin = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Rifle")
	float LeadFractionMax = 1.1f;

	/** Delay after each report before the bolt-cycle clack — the player's window tell */
	UPROPERTY(EditAnywhere, Category = "Infantry|Rifle")
	float BoltSoundDelay = 0.45f;

	/** Impacts within this range of a risen soldier scare him back down (flinch layer) */
	UPROPERTY(EditAnywhere, Category = "Infantry|Flinch")
	float FlinchRadius = 300.f;

	/** A comrade dying within this range triggers the same flinch */
	UPROPERTY(EditAnywhere, Category = "Infantry|Flinch")
	float ComradeDeathFlinchRadius = 1500.f;

	/** Extra cover time added on top of the normal wait after a flinch */
	UPROPERTY(EditAnywhere, Category = "Infantry|Flinch")
	float FlinchExtraCoverWaitMin = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Flinch")
	float FlinchExtraCoverWaitMax = 4.f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Body")
	float BodyRadius = 35.f;

	/** Capsule height while ducked behind the parapet */
	UPROPERTY(EditAnywhere, Category = "Infantry|Body")
	float CoverBodyHeight = 90.f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Body")
	float RisenBodyHeight = 170.f;

	/** Muzzle/eye height above the soldier's feet while risen */
	UPROPERTY(EditAnywhere, Category = "Infantry|Body")
	float RisenEyeHeight = 150.f;

	UPROPERTY(EditAnywhere, Category = "Infantry|Audio")
	float ShotSoundFalloffDistance = 30000.f;
};

USTRUCT()
struct FInfantrySoldierState
{
	GENERATED_BODY()

	TWeakObjectPtr<class AInfantrySoldier> Shell;

	EInfantryPhase Phase = EInfantryPhase::Cover;

	float PhaseTimer = 0.f;

	float EvalTimer = 0.f;

	int32 ShotsRemaining = 0;

	int32 RoundsInMag = 5;

	/** Ally index, PlayerTargetId, or NoTargetId */
	int32 CurrentTargetId = -2;

	float LeadFraction = 1.f;

	float BoltSoundCountdown = -1.f;

	bool bReloadOnNextCover = false;

	bool bAlive = true;
};

/** Visual shell only (Decision 021): mesh and ragdoll. All behavior lives in AInfantryManager. */
UCLASS()
class AInfantrySoldier : public AActor
{
	GENERATED_BODY()

public:
	AInfantrySoldier();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* BodyMesh;
};

UCLASS()
class AInfantryManager : public AActor
{
	GENERATED_BODY()

public:
	static constexpr int32 PlayerTargetId = -1;

	static constexpr int32 NoTargetId = -2;

	AInfantryManager();

	virtual void Tick(float DeltaSeconds) override;

	int32 GetSoldierCount() const { return Soldiers.Num(); }

	bool IsSoldierAlive(int32 Index) const;

	/** Capsule segment for the shared bullet loop's hit tests (height tracks the phase) */
	bool GetSoldierBodySegment(int32 Index, FVector& OutBottom, FVector& OutTop, float& OutRadius) const;

	/** One hit downs a soldier (08_ENEMY_AI.md); ragdolls and stays as a corpse */
	void NotifySoldierHit(int32 Index, const FVector& HitPoint);

	/** Player fire landing near a risen soldier scares him down (flinch layer) */
	void NotifyImpactNear(const FVector& ImpactPoint);

	/** Riflemen also need time to pick out a new man; a flat exclusion, since infantry carry no awareness model */
	void NotifyPlayerTakeover(float BlockedUntilTime);

	void SetDebugDraw(bool bEnabled) { bDebug = bEnabled; }

	const FInfantrySettings& GetSettings() const { return Settings; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Infantry", meta = (ShowOnlyInnerProperties))
	FInfantrySettings Settings;

	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundBase* ShotSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundBase* BoltCycleSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimSequence* CoverIdleAnim = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimSequence* RiseAnim = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimSequence* AimIdleAnim = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimSequence* FireAnim = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimSequence* DropAnim = nullptr;

private:
	void UpdateSoldier(FInfantrySoldierState& Soldier, int32 Index, float DeltaSeconds);

	void StartCover(FInfantrySoldierState& Soldier, float ExtraWait);

	void StartRising(FInfantrySoldierState& Soldier);

	void StartAiming(FInfantrySoldierState& Soldier, float AimSeconds);

	void StartDropping(FInfantrySoldierState& Soldier);

	void FireSoldierRound(FInfantrySoldierState& Soldier, int32 Index);

	void SelectSoldierTarget(FInfantrySoldierState& Soldier);

	void FlinchSoldier(FInfantrySoldierState& Soldier);

	bool IsTargetAlive(int32 TargetId) const;

	FVector GetTargetPosition(int32 TargetId) const;

	FVector GetTargetVelocity(int32 TargetId) const;

	FVector SoldierEyePosition(const FInfantrySoldierState& Soldier) const;

	bool HasLineOfSight(const FInfantrySoldierState& Soldier, const FVector& TargetPoint) const;

	void PlaySoldierAnim(FInfantrySoldierState& Soldier, UAnimSequence* Anim, bool bLoop, float FitToSeconds = 0.f);

	ABreakingWaveCharacter* GetPlayerCharacter() const;

	void DrawDebugState() const;

	UPROPERTY(Transient)
	USoundAttenuation* ShotAttenuation = nullptr;

	TArray<FInfantrySoldierState> Soldiers;

	TWeakObjectPtr<AMGBunkerManager> BunkerManager;

	TWeakObjectPtr<AAllySimManager> AllySim;

	float PlayerAcquireBlockedUntil = -1.f;

	bool bDebug = false;
};
