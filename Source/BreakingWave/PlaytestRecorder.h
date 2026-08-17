#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"

class ABreakingWaveCharacter;
class UScriptStruct;
class UWorld;
struct FAllySimSettings;
struct FInfantrySettings;
struct FMGSettings;
enum class EMGStop : uint8;

namespace Playtest
{
	/** 05_ZONES.md profile table; world Y = profile meters x 100 - 50400. Keep in sync with Tools/AnalyzePlaytests.py. */
	constexpr float ZoneBoundariesY[6] = { -25400.f, -19400.f, -11400.f, -2400.f, 7600.f, 15600.f };
	constexpr int32 ZoneCount = 5;
	constexpr int32 PlayerTargetId = -1;
	/** Shooter ids in the CSV's gun column: MG bunkers are 0..n, infantry soldiers sit above this offset.
	    Tools/AnalyzePlaytests.py attributes hits by this boundary — keep the two in sync. */
	constexpr int32 InfantryShooterIdBase = 1000;
	constexpr float SampleIntervalSeconds = 0.5f;
	constexpr float FlushIntervalSeconds = 5.f;

	int32 ZoneAtY(float WorldY);
}

/** One life. Death now chains into a takeover instead of resetting to the craft, so the
    session-level advance is tracked separately in FPlaytestSessionTally. */
struct FPlaytestLifeTally
{
	int32 LifeIndex = 0;
	float StartTime = 0.f;
	float StartY = 0.f;
	float MaxY = 0.f;
	int32 ShotsAtPlayer = 0;
	int32 ShotsTotal = 0;
	int32 PlayerShotsFired = 0;
	int32 InfantryDowned = 0;
	int32 CracksHeard = 0;
	int32 WhizzesHeard = 0;
	int32 PlayerHits = 0;
	float AdvanceWhileTargetedCm = 0.f;
	float AdvanceWhileClearCm = 0.f;
	float ZoneReachSeconds[Playtest::ZoneCount] = { -1.f, -1.f, -1.f, -1.f, -1.f };
	int32 Wounds = 0;
	bool bHeadshotDeath = false;
	bool bNoDamageUsed = false;
};

/** The continuous advance across chained lives — does the loop ratchet forward, or does the give-back cancel it */
struct FPlaytestSessionTally
{
	float StartTime = 0.f;
	float FirstY = 0.f;
	float MaxY = 0.f;
	int32 Lives = 0;
	int32 Takeovers = 0;
	int32 Manufactured = 0;
	float GivenBackCm = 0.f;
};

struct FPlaytestRecorder
{
	void BeginSession(UWorld* InWorld, const FMGSettings& MGSettings, const FAllySimSettings* AllySettings,
		const FInfantrySettings* InfantrySettings);
	void EndSession();

	void SamplePlayer(const ABreakingWaveCharacter* Player, bool bTargetedByLiveGun, int32 StoppedGunCount,
		int32 AlliesInDisc, float DeltaSeconds);

	void LogShot(int32 GunIndex, const FVector& FirePos, int32 TargetId);
	void LogPlayerShot(const FVector& FirePos);
	void LogInfantryShot(int32 SoldierIndex, const FVector& FirePos, int32 TargetId);
	void LogInfantryDown(int32 SoldierIndex, const FVector& HitPoint);
	void LogImpact(int32 GunIndex, const FVector& HitPoint);
	void LogCrack(int32 GunIndex, const FVector& NearPoint);
	void LogWhizz(int32 GunIndex, const FVector& NearPoint);
	void LogAllyKilled(int32 GunIndex, const FVector& HitPoint);
	void LogPlayerHit(int32 GunIndex, const FVector& HitPoint, bool bNoDamage, FName BoneName, bool bHeadshot);
	void LogPlayerDeath(const FVector& DeathPos);
	void LogTakeover(const FVector& DeathAnchor, const FVector& TakeoverPosition, bool bManufactured, int32 DiscCandidates);
	void LogStopStart(int32 GunIndex, EMGStop Stop, float DurationSeconds);
	void LogStopEnd(int32 GunIndex, EMGStop Stop);
	void LogTargetSwitch(int32 GunIndex, int32 FromId, int32 ToId);
	void LogCrewKilled(int32 GunIndex, int32 CrewRemaining);
	void LogNoDamageToggle(bool bEnabled);

private:
	float Now() const;
	void StartLife(const FVector& PlayerPos);
	void AppendEvent(const TCHAR* Event, const FVector& Pos, int32 GunIndex, const FString& Extra);
	void AppendSettingsDump(const TCHAR* Prefix, const UScriptStruct* StructType, const void* StructValue);
	FString LifeSummaryExtra() const;
	void PrintLifeSummary(const FVector& DeathPos) const;
	void FlushToDisk();

	TWeakObjectPtr<UWorld> World;
	FString CsvPath;
	TArray<FString> PendingLines;
	float SampleTimer = 0.f;
	float FlushTimer = 0.f;
	float LastPlayerY = 0.f;
	bool bSessionActive = false;
	bool bLifeActive = false;
	FPlaytestLifeTally Life;
	FPlaytestSessionTally Session;
};
