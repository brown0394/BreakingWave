#pragma once

#include "CoreMinimal.h"
#include "ClimbOver.generated.h"

class ACharacter;

/** Hauling yourself over a wall by walking into it (Decision 058). Jumping stays removed (Decision 023) */
USTRUCT(BlueprintType)
struct FClimbSettings
{
	GENERATED_BODY()

	/** Lowest ledge that needs a climb; the movement component already steps over anything below it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinLedgeHeight = 50.f;

	/** Tallest wall a man can haul himself over. The fire trench's rear wall runs ~2.3 m on the defense line's slope (Decision 057) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxLedgeHeight = 250.f;

	/** How far past the capsule's edge a wall still counts as walked into */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WallReach = 30.f;

	/** Steepest wall face, from the climber's heading, that still reads as "in front of him" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxWallFacingAngleDeg = 45.f;

	/** Spacing of the probes that look for the top of the wall past its face; a carved trench wall slopes back ~1 m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LedgeProbeSpacing = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 LedgeProbeCount = 6;

	/** Gap kept between the capsule and the lip while moving over it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LipClearance = 5.f;

	/** Forward must be held into the wall this long, so brushing a wall never climbs it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HoldSeconds = 0.2f;

	/** The haul up takes this long plus RiseSecondsPerMetre for every metre of wall — a deep trench is slow to leave */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RiseBaseSeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RiseSecondsPerMetre = 0.45f;

	/** Rolling over the lip once the body is above it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OverSeconds = 0.3f;
};

enum class EClimbPhase : uint8
{
	None,
	Rising,
	GoingOver,
};

/** Where a climb goes: straight up the wall to RiseTarget, then over the lip to OverTarget, then the climber drops onto whatever is below */
struct FClimbLedge
{
	FVector RiseTarget = FVector::ZeroVector;

	FVector OverTarget = FVector::ZeroVector;

	float LedgeHeight = 0.f;
};

/** True when the climber faces a wall he can haul himself over and there is room for his body on the way up and on top */
bool FindClimbLedge(const ACharacter& Climber, const FVector& Heading, const FClimbSettings& Settings, FClimbLedge& OutLedge);
