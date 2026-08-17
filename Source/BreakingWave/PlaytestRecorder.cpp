#include "PlaytestRecorder.h"

#include "BeachAllySim.h"
#include "BeachInfantrySystem.h"
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

void FPlaytestRecorder::BeginSession(UWorld* InWorld, const FMGSettings& MGSettings, const FAllySimSettings* AllySettings,
	const FInfantrySettings* InfantrySettings)
{
	World = InWorld;
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("Playtests");
	IFileManager::Get().MakeDirectory(*Dir, true);
	CsvPath = Dir / FString::Printf(TEXT("session_%s.csv"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	PendingLines.Add(TEXT("t,life,event,x,y,z,gun,extra"));
	bSessionActive = true;
	bLifeActive = false;
	Life = FPlaytestLifeTally();
	Session = FPlaytestSessionTally();
	Session.StartTime = Now();
	AppendSettingsDump(TEXT("MG"), FMGSettings::StaticStruct(), &MGSettings);
	if (AllySettings != nullptr)
	{
		AppendSettingsDump(TEXT("Ally"), FAllySimSettings::StaticStruct(), AllySettings);
	}
	if (InfantrySettings != nullptr)
	{
		AppendSettingsDump(TEXT("Infantry"), FInfantrySettings::StaticStruct(), InfantrySettings);
	}
}

void FPlaytestRecorder::EndSession()
{
	if (!bSessionActive)
	{
		return;
	}
	if (bLifeActive)
	{
		AppendEvent(TEXT("life_abort"), FVector(0.f, LastPlayerY, 0.f), -1, LifeSummaryExtra());
	}

	const float ManufacturedFraction = Session.Takeovers > 0
		? static_cast<float>(Session.Manufactured) / Session.Takeovers : 0.f;
	AppendEvent(TEXT("session_end"), FVector(0.f, Session.MaxY, 0.f), -1, FString::Printf(
		TEXT("dur=%.1f;lives=%d;takeovers=%d;first_y=%.0f;max_y=%.0f;net_advance=%.0f;given_back=%.0f;made=%d;made_frac=%.2f"),
		Now() - Session.StartTime, Session.Lives, Session.Takeovers, Session.FirstY, Session.MaxY,
		Session.MaxY - Session.FirstY, Session.GivenBackCm, Session.Manufactured, ManufacturedFraction));

	FlushToDisk();
	bSessionActive = false;
	bLifeActive = false;
}

void FPlaytestRecorder::SamplePlayer(const ABreakingWaveCharacter* Player, bool bTargetedByLiveGun, int32 StoppedGunCount,
	int32 AlliesInDisc, float DeltaSeconds)
{
	if (!bSessionActive || Player == nullptr)
	{
		return;
	}

	const FVector Pos = Player->GetActorLocation();
	if (!bLifeActive)
	{
		StartLife(Pos);
	}

	const float AdvanceCm = Pos.Y - LastPlayerY;
	if (AdvanceCm > 0.f)
	{
		(bTargetedByLiveGun ? Life.AdvanceWhileTargetedCm : Life.AdvanceWhileClearCm) += AdvanceCm;
	}
	LastPlayerY = Pos.Y;
	Life.MaxY = FMath::Max(Life.MaxY, static_cast<float>(Pos.Y));
	Session.MaxY = FMath::Max(Session.MaxY, static_cast<float>(Pos.Y));

	const int32 StartZone = Playtest::ZoneAtY(Life.StartY);
	const int32 CurrentZone = Playtest::ZoneAtY(Pos.Y);
	for (int32 Zone = StartZone + 1; Zone <= CurrentZone; ++Zone)
	{
		if (Life.ZoneReachSeconds[Zone] < 0.f)
		{
			const float Split = Now() - Life.StartTime;
			Life.ZoneReachSeconds[Zone] = Split;
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
		AppendEvent(TEXT("sample"), Pos, -1, FString::Printf(
			TEXT("stance=%s;speed=%.0f;targeted=%d;stopped=%d;wounds=%d;disc_allies=%d"),
			Stance, Player->GetVelocity().Size2D(), bTargetedByLiveGun ? 1 : 0, StoppedGunCount,
			Life.Wounds, AlliesInDisc));
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
	if (bLifeActive)
	{
		++Life.ShotsTotal;
		if (TargetId == Playtest::PlayerTargetId)
		{
			++Life.ShotsAtPlayer;
		}
	}
	AppendEvent(TEXT("shot"), FirePos, GunIndex, FString::Printf(TEXT("tgt=%d"), TargetId));
}

void FPlaytestRecorder::LogPlayerShot(const FVector& FirePos)
{
	if (bLifeActive)
	{
		++Life.PlayerShotsFired;
	}
	AppendEvent(TEXT("pshot"), FirePos, Playtest::PlayerTargetId, FString());
}

void FPlaytestRecorder::LogInfantryShot(int32 SoldierIndex, const FVector& FirePos, int32 TargetId)
{
	if (bLifeActive)
	{
		++Life.ShotsTotal;
		if (TargetId == Playtest::PlayerTargetId)
		{
			++Life.ShotsAtPlayer;
		}
	}
	AppendEvent(TEXT("inf_shot"), FirePos, Playtest::InfantryShooterIdBase + SoldierIndex,
		FString::Printf(TEXT("tgt=%d"), TargetId));
}

void FPlaytestRecorder::LogInfantryDown(int32 SoldierIndex, const FVector& HitPoint)
{
	if (bLifeActive)
	{
		++Life.InfantryDowned;
	}
	AppendEvent(TEXT("inf_down"), HitPoint, Playtest::InfantryShooterIdBase + SoldierIndex, FString());
}

void FPlaytestRecorder::LogImpact(int32 GunIndex, const FVector& HitPoint)
{
	AppendEvent(TEXT("impact"), HitPoint, GunIndex, FString());
}

void FPlaytestRecorder::LogCrack(int32 GunIndex, const FVector& NearPoint)
{
	if (bLifeActive)
	{
		++Life.CracksHeard;
	}
	AppendEvent(TEXT("crack"), NearPoint, GunIndex, FString());
}

void FPlaytestRecorder::LogWhizz(int32 GunIndex, const FVector& NearPoint)
{
	if (bLifeActive)
	{
		++Life.WhizzesHeard;
	}
	AppendEvent(TEXT("whizz"), NearPoint, GunIndex, FString());
}

void FPlaytestRecorder::LogAllyKilled(int32 GunIndex, const FVector& HitPoint)
{
	AppendEvent(TEXT("ally_death"), HitPoint, GunIndex, FString());
}

void FPlaytestRecorder::LogPlayerHit(int32 GunIndex, const FVector& HitPoint, bool bNoDamage, FName BoneName, bool bHeadshot)
{
	if (bLifeActive)
	{
		++Life.PlayerHits;
		if (bNoDamage)
		{
			Life.bNoDamageUsed = true;
		}
		else
		{
			++Life.Wounds;
			Life.bHeadshotDeath = bHeadshot;
		}
	}
	AppendEvent(TEXT("hit_player"), HitPoint, GunIndex, FString::Printf(TEXT("nodmg=%d;bone=%s;head=%d"),
		bNoDamage ? 1 : 0, *BoneName.ToString(), bHeadshot ? 1 : 0));
}

void FPlaytestRecorder::LogPlayerDeath(const FVector& DeathPos)
{
	if (!bSessionActive || !bLifeActive)
	{
		return;
	}
	AppendEvent(TEXT("death"), DeathPos, -1, LifeSummaryExtra());
	PrintLifeSummary(DeathPos);
	FlushToDisk();
	bLifeActive = false;
}

void FPlaytestRecorder::LogTakeover(const FVector& DeathAnchor, const FVector& TakeoverPosition,
	bool bManufactured, int32 DiscCandidates)
{
	++Session.Takeovers;
	Session.Manufactured += bManufactured ? 1 : 0;
	Session.GivenBackCm += FMath::Max(0.f, static_cast<float>(DeathAnchor.Y - TakeoverPosition.Y));

	AppendEvent(TEXT("takeover"), TakeoverPosition, -1, FString::Printf(
		TEXT("death_y=%.0f;given_back=%.0f;made=%d;disc_allies=%d;dist=%.0f"),
		DeathAnchor.Y, DeathAnchor.Y - TakeoverPosition.Y, bManufactured ? 1 : 0, DiscCandidates,
		FVector::Dist2D(DeathAnchor, TakeoverPosition)));
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
	if (bEnabled && bLifeActive)
	{
		Life.bNoDamageUsed = true;
	}
	AppendEvent(TEXT("nodamage"), FVector::ZeroVector, -1,
		FString::Printf(TEXT("enabled=%d"), bEnabled ? 1 : 0));
}

float FPlaytestRecorder::Now() const
{
	return World.IsValid() ? World->GetTimeSeconds() : 0.f;
}

void FPlaytestRecorder::StartLife(const FVector& PlayerPos)
{
	const int32 NextIndex = Life.LifeIndex + 1;
	Life = FPlaytestLifeTally();
	Life.LifeIndex = NextIndex;
	Life.StartTime = Now();
	Life.StartY = PlayerPos.Y;
	Life.MaxY = PlayerPos.Y;
	LastPlayerY = PlayerPos.Y;
	SampleTimer = Playtest::SampleIntervalSeconds;
	bLifeActive = true;

	++Session.Lives;
	if (Session.Lives == 1)
	{
		Session.FirstY = PlayerPos.Y;
		Session.MaxY = PlayerPos.Y;
	}

	AppendEvent(TEXT("life_start"), PlayerPos, -1, FString());
}

void FPlaytestRecorder::AppendEvent(const TCHAR* Event, const FVector& Pos, int32 GunIndex, const FString& Extra)
{
	if (!bSessionActive)
	{
		return;
	}
	PendingLines.Add(FString::Printf(TEXT("%.2f,%d,%s,%.0f,%.0f,%.0f,%d,%s"),
		Now(), Life.LifeIndex, Event, Pos.X, Pos.Y, Pos.Z, GunIndex, *Extra));
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

FString FPlaytestRecorder::LifeSummaryExtra() const
{
	return FString::Printf(TEXT("dur=%.1f;starty=%.0f;maxy=%.0f;ground=%.0f;shots_at=%d;hits=%d;wounds=%d;head=%d;cracks=%d;whizzes=%d;adv_targeted=%.0f;adv_clear=%.0f;nodmg=%d;pshots=%d;inf_down=%d"),
		Now() - Life.StartTime, Life.StartY, Life.MaxY, Life.MaxY - Life.StartY,
		Life.ShotsAtPlayer, Life.PlayerHits, Life.Wounds, Life.bHeadshotDeath ? 1 : 0, Life.CracksHeard, Life.WhizzesHeard,
		Life.AdvanceWhileTargetedCm, Life.AdvanceWhileClearCm, Life.bNoDamageUsed ? 1 : 0, Life.PlayerShotsFired, Life.InfantryDowned);
}

void FPlaytestRecorder::PrintLifeSummary(const FVector& DeathPos) const
{
	const float Duration = Now() - Life.StartTime;
	const int32 DeathZone = Playtest::ZoneAtY(DeathPos.Y);
	FString Splits;
	for (int32 Zone = 1; Zone < Playtest::ZoneCount; ++Zone)
	{
		Splits += Life.ZoneReachSeconds[Zone] >= 0.f
			? FString::Printf(TEXT("Z%d %.1fs  "), Zone, Life.ZoneReachSeconds[Zone])
			: FString::Printf(TEXT("Z%d --  "), Zone);
	}
	const float HitPercent = Life.ShotsAtPlayer > 0 ? 100.f * Life.PlayerHits / Life.ShotsAtPlayer : 0.f;
	const FString Text = FString::Printf(
		TEXT("LIFE %d  %.1fs  %s in Zone %d  ground %+.0fm (session best %.0fm)\nshots at you %d  hits %d (%.1f%%)  cracks %d  whizzes %d  you fired %d  infantry downed %d\nadvance while targeted %.0fm | while clear %.0fm\nsplits: %s%s"),
		Life.LifeIndex, Duration, Life.bHeadshotDeath ? TEXT("headshot") : TEXT("bled out"), DeathZone,
		(Life.MaxY - Life.StartY) / 100.f, (Session.MaxY - Session.FirstY) / 100.f,
		Life.ShotsAtPlayer, Life.PlayerHits, HitPercent, Life.CracksHeard, Life.WhizzesHeard, Life.PlayerShotsFired, Life.InfantryDowned,
		Life.AdvanceWhileTargetedCm / 100.f, Life.AdvanceWhileClearCm / 100.f,
		*Splits, Life.bNoDamageUsed ? TEXT("\n(no-damage used this life)") : TEXT(""));
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
