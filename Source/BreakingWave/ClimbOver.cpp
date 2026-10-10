#include "ClimbOver.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	constexpr float RestingContactSkin = 2.f;

	struct FClimberQuery
	{
		const UWorld* World = nullptr;
		ECollisionChannel Channel = ECC_Pawn;
		FCollisionQueryParams Params;
		FCollisionResponseParams Responses;
		FCollisionShape Body;
	};

	bool TraceWorld(const FClimberQuery& Query, const FVector& Start, const FVector& End, FHitResult& OutHit)
	{
		return Query.World->LineTraceSingleByChannel(OutHit, Start, End, Query.Channel, Query.Params, Query.Responses);
	}

	bool IsBodyPathBlocked(const FClimberQuery& Query, const FVector& From, const FVector& To)
	{
		return Query.World->SweepTestByChannel(From, To, FQuat::Identity, Query.Channel, Query.Body, Query.Params, Query.Responses);
	}
}

bool FindClimbLedge(const ACharacter& Climber, const FVector& Heading, const FClimbSettings& Settings, FClimbLedge& OutLedge)
{
	const UCapsuleComponent* Capsule = Climber.GetCapsuleComponent();
	const float WalkableFloorZ = Climber.GetCharacterMovement()->GetWalkableFloorZ();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector Centre = Climber.GetActorLocation();
	const float FeetZ = Centre.Z - HalfHeight;
	const FVector Forward = FVector(Heading.X, Heading.Y, 0.f).GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		return false;
	}

	FClimberQuery Query;
	Query.World = Climber.GetWorld();
	Query.Channel = Capsule->GetCollisionObjectType();
	Query.Params = FCollisionQueryParams(SCENE_QUERY_STAT(ClimbOver), false, &Climber);
	Query.Body = FCollisionShape::MakeCapsule(Radius - RestingContactSkin, HalfHeight - RestingContactSkin);
	Capsule->InitSweepCollisionParams(Query.Params, Query.Responses);

	const FVector WallProbeStart(Centre.X, Centre.Y, FeetZ + Settings.MinLedgeHeight);
	FHitResult WallHit;
	if (!TraceWorld(Query, WallProbeStart, WallProbeStart + Forward * (Radius + Settings.WallReach), WallHit)
		|| WallHit.ImpactNormal.Z >= WalkableFloorZ)
	{
		return false;
	}
	const FVector WallFacing = FVector(WallHit.ImpactNormal.X, WallHit.ImpactNormal.Y, 0.f).GetSafeNormal();
	if (FVector::DotProduct(-Forward, WallFacing) < FMath::Cos(FMath::DegreesToRadians(Settings.MaxWallFacingAngleDeg)))
	{
		return false;
	}

	const float ProbeTopZ = FeetZ + Settings.MaxLedgeHeight + Settings.LipClearance;
	const float ProbeBottomZ = FeetZ + Settings.MinLedgeHeight;
	const FVector WallFoot(WallHit.ImpactPoint.X, WallHit.ImpactPoint.Y, 0.f);
	for (int32 Probe = 1; Probe <= Settings.LedgeProbeCount; ++Probe)
	{
		const FVector ProbeXY = WallFoot + Forward * (Settings.LedgeProbeSpacing * Probe);
		FHitResult LipHit;
		if (!TraceWorld(Query, FVector(ProbeXY.X, ProbeXY.Y, ProbeTopZ), FVector(ProbeXY.X, ProbeXY.Y, ProbeBottomZ), LipHit)
			|| LipHit.ImpactNormal.Z < WalkableFloorZ)
		{
			continue;
		}

		const FVector OverXY = ProbeXY + Forward * (Radius + RestingContactSkin);
		FHitResult GroundPastLip;
		const float GroundPastLipZ = TraceWorld(Query, FVector(OverXY.X, OverXY.Y, ProbeTopZ), FVector(OverXY.X, OverXY.Y, ProbeBottomZ), GroundPastLip)
			? GroundPastLip.ImpactPoint.Z
			: LipHit.ImpactPoint.Z;
		const float LedgeZ = FMath::Max(LipHit.ImpactPoint.Z, GroundPastLipZ);
		if (LedgeZ - FeetZ > Settings.MaxLedgeHeight)
		{
			return false;
		}

		const float BodyCentreZ = LedgeZ + HalfHeight + Settings.LipClearance;
		OutLedge.RiseTarget = FVector(Centre.X, Centre.Y, BodyCentreZ);
		OutLedge.OverTarget = FVector(OverXY.X, OverXY.Y, BodyCentreZ);
		OutLedge.LedgeHeight = LedgeZ - FeetZ;
		return !IsBodyPathBlocked(Query, Centre + FVector(0.f, 0.f, RestingContactSkin), OutLedge.RiseTarget)
			&& !IsBodyPathBlocked(Query, OutLedge.RiseTarget, OutLedge.OverTarget);
	}
	return false;
}
