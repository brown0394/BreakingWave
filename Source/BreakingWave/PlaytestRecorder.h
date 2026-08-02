#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"

class ABreakingWaveCharacter;
class UScriptStruct;
class UWorld;
struct FAllySimSettings;
struct FMGSettings;
enum class EMGStop : uint8;

namespace Playtest
{
	/** 05_ZONES.md profile table; world Y = profile meters x 100 - 50400. Keep in sync with Tools/AnalyzePlaytests.py. */
	constexpr float ZoneBoundariesY[6] = { -25400.f, -19400.f, -11400.f, -2400.f, 7600.f, 15600.f };
	constexpr int32 ZoneCount = 5;
	constexpr int32 PlayerTargetId = -1;
	constexpr float SampleIntervalSeconds = 0.5f;
	constexpr float FlushIntervalSeconds = 5.f;

	int32 ZoneAtY(float WorldY);
}

struct FPlaytestRunTally
{
	int32 RunIndex = 0;
	float StartTime = 0.f;
	float StartY = 0.f;
	float MaxY = 0.f;
	int32 ShotsAtPlayer = 0;
	int32 ShotsTotal = 0;
	int32 CracksHeard = 0;
	int32 PlayerHits = 0;
	float AdvanceWhileTargetedCm = 0.f;
	float AdvanceWhileClearCm = 0.f;
	float ZoneReachSeconds[Playtest::ZoneCount] = { -1.f, -1.f, -1.f, -1.f, -1.f };
	bool bNoDamageUsed = false;
};

struct FPlaytestRecorder
{
	void BeginSession(UWorld* InWorld, const FMGSettings& MGSettings, const FAllySimSettings* AllySettings);
	void EndSession();

	void SamplePlayer(const ABreakingWaveCharacter* Player, bool bTargetedByLiveGun, int32 StoppedGunCount, float DeltaSeconds);

	void LogShot(int32 GunIndex, const FVector& FirePos, int32 TargetId);
	void LogImpact(int32 GunIndex, const FVector& HitPoint);
	void LogCrack(int32 GunIndex, const FVector& NearPoint);
	void LogAllyKilled(int32 GunIndex, const FVector& HitPoint);
	void LogPlayerHit(int32 GunIndex, const FVector& HitPoint, bool bNoDamage);
	void LogPlayerDeath(const FVector& DeathPos);
	void LogStopStart(int32 GunIndex, EMGStop Stop, float DurationSeconds);
	void LogStopEnd(int32 GunIndex, EMGStop Stop);
	void LogTargetSwitch(int32 GunIndex, int32 FromId, int32 ToId);
	void LogCrewKilled(int32 GunIndex, int32 CrewRemaining);
	void LogNoDamageToggle(bool bEnabled);

private:
	float Now() const;
	void StartRun(const FVector& PlayerPos);
	void AppendEvent(const TCHAR* Event, const FVector& Pos, int32 GunIndex, const FString& Extra);
	void AppendSettingsDump(const TCHAR* Prefix, const UScriptStruct* StructType, const void* StructValue);
	FString RunSummaryExtra() const;
	void PrintRunSummary(const FVector& DeathPos) const;
	void FlushToDisk();

	TWeakObjectPtr<UWorld> World;
	FString CsvPath;
	TArray<FString> PendingLines;
	float SampleTimer = 0.f;
	float FlushTimer = 0.f;
	float LastPlayerY = 0.f;
	bool bSessionActive = false;
	bool bRunActive = false;
	FPlaytestRunTally Run;
};
