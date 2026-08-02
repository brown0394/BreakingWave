#include "PlaytestRecorder.h"

#include "BeachAllySim.h"
#include "BreakingWaveCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "MGBunkerSystem.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"

namespace Playtest
{
	int32 ZoneAtY(float WorldY)
	{
		int32 Zone = 0;
		for (int32 i = 1; i <= ZoneCount; ++i)
		{
			if (WorldY >= ZoneBoundariesY[i])
			{
				Zone = i;
			}
		}
		return FMath::Min(Zone, ZoneCount - 1);
	}
}

static const TCHAR* PlaytestStopName(EMGStop Stop)
{
	switch (Stop)
	{
	case EMGStop::Reload: return TEXT("Reload");
	case EMGStop::BarrelChange: return TEXT("BarrelChange");
	case EMGStop::Jam: return TEXT("Jam");
	case EMGStop::Takeover: return TEXT("Takeover");
	default: return TEXT("None");
	}
}

void FPlaytestRecorder::BeginSession(UWorld* InWorld, const FMGSettings& MGSettings, const FAllySimSettings* AllySettings)
{
	World = InWorld;
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("Playtests");
	IFileManager::Get().MakeDirectory(*Dir, true);
	CsvPath = Dir / FString::Printf(TEXT("session_%s.csv"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	PendingLines.Add(TEXT("t,run,event,x,y,z,gun,extra"));
	bSessionActive = true;
	bRunActive = false;
	Run = FPlaytestRunTally();
	AppendSettingsDump(TEXT("MG"), FMGSettings::StaticStruct(), &MGSettings);
	if (AllySettings != nullptr)
	{
		AppendSettingsDump(TEXT("Ally"), FAllySimSettings::StaticStruct(), AllySettings);
	}
}

void FPlaytestRecorder::EndSession()
{
	if (!bSessionActive)
	{
		return;
	}
	if (bRunActive)
	{
		AppendEvent(TEXT("run_abort"), FVector(0.f, LastPlayerY, 0.f), -1, RunSummaryExtra());
	}
	FlushToDisk();
	bSessionActive = false;
	bRunActive = false;
}

void FPlaytestRecorder::SamplePlayer(const ABreakingWaveCharacter* Player, bool bTargetedByLiveGun, int32 StoppedGunCount, float DeltaSeconds)
{
	if (!bSessionActive || Player == nullptr)
	{
		return;
	}

	const FVector Pos = Player->GetActorLocation();
	if (!bRunActive)
	{
		StartRun(Pos);
	}

	const float AdvanceCm = Pos.Y - LastPlayerY;
	if (AdvanceCm > 0.f)
	{
		(bTargetedByLiveGun ? Run.AdvanceWhileTargetedCm : Run.AdvanceWhileClearCm) += AdvanceCm;
	}
	LastPlayerY = Pos.Y;
	Run.MaxY = FMath::Max(Run.MaxY, static_cast<float>(Pos.Y));

	const int32 StartZone = Playtest::ZoneAtY(Run.StartY);
	const int32 CurrentZone = Playtest::ZoneAtY(Pos.Y);
	for (int32 Zone = StartZone + 1; Zone <= CurrentZone; ++Zone)
	{
		if (Run.ZoneReachSeconds[Zone] < 0.f)
		{
			const float Split = Now() - Run.StartTime;
			Run.ZoneReachSeconds[Zone] = Split;
			AppendEvent(TEXT("zone_cross"), Pos, -1, FString::Printf(TEXT("zone=%d;split=%.1f"), Zone, Split));
			if (GEngine != nullptr)
			{
				GEngine->AddOnScreenDebugMessage(110 + Zone, 5.f, FColor::Cyan,
					FString::Printf(TEXT("Zone %d — %.1fs"), Zone, Split));
			}
		}
	}

	SampleTimer += DeltaSeconds;
	if (SampleTimer >= Playtest::SampleIntervalSeconds)
	{
		SampleTimer = 0.f;
		const TCHAR* Stance =
			Player->IsProne() ? TEXT("prone") :
			Player->IsSliding() ? TEXT("slide") :
			Player->GetCharacterMovement()->IsFalling() ? TEXT("air") : TEXT("foot");
		AppendEvent(TEXT("sample"), Pos, -1, FString::Printf(TEXT("stance=%s;speed=%.0f;targeted=%d;stopped=%d"),
			Stance, Player->GetVelocity().Size2D(), bTargetedByLiveGun ? 1 : 0, StoppedGunCount));
	}

	FlushTimer += DeltaSeconds;
	if (FlushTimer >= Playtest::FlushIntervalSeconds)
	{
		FlushTimer = 0.f;
		FlushToDisk();
	}
}

void FPlaytestRecorder::LogShot(int32 GunIndex, const FVector& FirePos, int32 TargetId)
{
	if (bRunActive)
	{
		++Run.ShotsTotal;
		if (TargetId == Playtest::PlayerTargetId)
		{
			++Run.ShotsAtPlayer;
		}
	}
	AppendEvent(TEXT("shot"), FirePos, GunIndex, FString::Printf(TEXT("tgt=%d"), TargetId));
}

void FPlaytestRecorder::LogImpact(int32 GunIndex, const FVector& HitPoint)
{
	AppendEvent(TEXT("impact"), HitPoint, GunIndex, FString());
}

void FPlaytestRecorder::LogCrack(int32 GunIndex, const FVector& NearPoint)
{
	if (bRunActive)
	{
		++Run.CracksHeard;
	}
	AppendEvent(TEXT("crack"), NearPoint, GunIndex, FString());
}

void FPlaytestRecorder::LogAllyKilled(int32 GunIndex, const FVector& HitPoint)
{
	AppendEvent(TEXT("ally_death"), HitPoint, GunIndex, FString());
}

void FPlaytestRecorder::LogPlayerHit(int32 GunIndex, const FVector& HitPoint, bool bNoDamage)
{
	if (bRunActive)
	{
		++Run.PlayerHits;
		if (bNoDamage)
		{
			Run.bNoDamageUsed = true;
		}
	}
	AppendEvent(TEXT("hit_player"), HitPoint, GunIndex, FString::Printf(TEXT("nodmg=%d"), bNoDamage ? 1 : 0));
}

void FPlaytestRecorder::LogPlayerDeath(const FVector& DeathPos)
{
	if (!bSessionActive || !bRunActive)
	{
		return;
	}
	AppendEvent(TEXT("death"), DeathPos, -1, RunSummaryExtra());
	PrintRunSummary(DeathPos);
	FlushToDisk();
	bRunActive = false;
}

void FPlaytestRecorder::LogStopStart(int32 GunIndex, EMGStop Stop, float DurationSeconds)
{
	AppendEvent(TEXT("stop_start"), FVector::ZeroVector, GunIndex,
		FString::Printf(TEXT("kind=%s;dur=%.1f"), PlaytestStopName(Stop), DurationSeconds));
}

void FPlaytestRecorder::LogStopEnd(int32 GunIndex, EMGStop Stop)
{
	AppendEvent(TEXT("stop_end"), FVector::ZeroVector, GunIndex,
		FString::Printf(TEXT("kind=%s"), PlaytestStopName(Stop)));
}

void FPlaytestRecorder::LogTargetSwitch(int32 GunIndex, int32 FromId, int32 ToId)
{
	AppendEvent(TEXT("target_switch"), FVector::ZeroVector, GunIndex,
		FString::Printf(TEXT("from=%d;to=%d"), FromId, ToId));
}

void FPlaytestRecorder::LogCrewKilled(int32 GunIndex, int32 CrewRemaining)
{
	AppendEvent(TEXT("crew_killed"), FVector::ZeroVector, GunIndex,
		FString::Printf(TEXT("remain=%d"), CrewRemaining));
}

void FPlaytestRecorder::LogNoDamageToggle(bool bEnabled)
{
	if (bEnabled && bRunActive)
	{
		Run.bNoDamageUsed = true;
	}
	AppendEvent(TEXT("nodamage"), FVector::ZeroVector, -1,
		FString::Printf(TEXT("enabled=%d"), bEnabled ? 1 : 0));
}

float FPlaytestRecorder::Now() const
{
	return World.IsValid() ? World->GetTimeSeconds() : 0.f;
}

void FPlaytestRecorder::StartRun(const FVector& PlayerPos)
{
	const int32 NextIndex = Run.RunIndex + 1;
	Run = FPlaytestRunTally();
	Run.RunIndex = NextIndex;
	Run.StartTime = Now();
	Run.StartY = PlayerPos.Y;
	Run.MaxY = PlayerPos.Y;
	LastPlayerY = PlayerPos.Y;
	SampleTimer = Playtest::SampleIntervalSeconds;
	bRunActive = true;
	AppendEvent(TEXT("run_start"), PlayerPos, -1, FString());
}

void FPlaytestRecorder::AppendEvent(const TCHAR* Event, const FVector& Pos, int32 GunIndex, const FString& Extra)
{
	if (!bSessionActive)
	{
		return;
	}
	PendingLines.Add(FString::Printf(TEXT("%.2f,%d,%s,%.0f,%.0f,%.0f,%d,%s"),
		Now(), Run.RunIndex, Event, Pos.X, Pos.Y, Pos.Z, GunIndex, *Extra));
}

void FPlaytestRecorder::AppendSettingsDump(const TCHAR* Prefix, const UScriptStruct* StructType, const void* StructValue)
{
	for (TFieldIterator<FProperty> It(StructType); It; ++It)
	{
		FString ValueText;
		It->ExportText_InContainer(0, ValueText, StructValue, StructValue, nullptr, PPF_None);
		AppendEvent(TEXT("setting"), FVector::ZeroVector, -1,
			FString::Printf(TEXT("%s.%s=%s"), Prefix, *It->GetName(), *ValueText));
	}
}

FString FPlaytestRecorder::RunSummaryExtra() const
{
	return FString::Printf(TEXT("dur=%.1f;maxy=%.0f;shots_at=%d;hits=%d;cracks=%d;adv_targeted=%.0f;adv_clear=%.0f;nodmg=%d"),
		Now() - Run.StartTime, Run.MaxY, Run.ShotsAtPlayer, Run.PlayerHits, Run.CracksHeard,
		Run.AdvanceWhileTargetedCm, Run.AdvanceWhileClearCm, Run.bNoDamageUsed ? 1 : 0);
}

void FPlaytestRecorder::PrintRunSummary(const FVector& DeathPos) const
{
	const float Duration = Now() - Run.StartTime;
	const int32 DeathZone = Playtest::ZoneAtY(DeathPos.Y);
	FString Splits;
	for (int32 Zone = 1; Zone < Playtest::ZoneCount; ++Zone)
	{
		Splits += Run.ZoneReachSeconds[Zone] >= 0.f
			? FString::Printf(TEXT("Z%d %.1fs  "), Zone, Run.ZoneReachSeconds[Zone])
			: FString::Printf(TEXT("Z%d --  "), Zone);
	}
	const float HitPercent = Run.ShotsAtPlayer > 0 ? 100.f * Run.PlayerHits / Run.ShotsAtPlayer : 0.f;
	const FString Text = FString::Printf(
		TEXT("RUN %d  %.1fs  died Zone %d  advanced %.0fm\nshots at you %d  hits %d (%.1f%%)  cracks %d\nadvance while targeted %.0fm | while clear %.0fm\nsplits: %s%s"),
		Run.RunIndex, Duration, DeathZone, (Run.MaxY - Run.StartY) / 100.f,
		Run.ShotsAtPlayer, Run.PlayerHits, HitPercent, Run.CracksHeard,
		Run.AdvanceWhileTargetedCm / 100.f, Run.AdvanceWhileClearCm / 100.f,
		*Splits, Run.bNoDamageUsed ? TEXT("\n(no-damage used this run)") : TEXT(""));
	UE_LOG(LogTemp, Log, TEXT("Playtest %s"), *Text);
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(105, 12.f, FColor::Green, Text);
	}
}

void FPlaytestRecorder::FlushToDisk()
{
	if (PendingLines.Num() == 0)
	{
		return;
	}
	const FString Block = FString::Join(PendingLines, TEXT("\n")) + TEXT("\n");
	FFileHelper::SaveStringToFile(Block, *CsvPath, FFileHelper::EEncodingOptions::AutoDetect,
		&IFileManager::Get(), FILEWRITE_Append);
	PendingLines.Reset();
}
