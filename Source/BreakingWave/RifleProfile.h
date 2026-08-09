#pragma once

#include "CoreMinimal.h"
#include "RifleProfile.generated.h"

/** One rifle system, two data rows: the player's semi-auto and the defenders' bolt-action
    are the same code with different numbers. ALL NUMBERS TENTATIVE. */
USTRUCT(BlueprintType)
struct FRifleProfile
{
	GENERATED_BODY()

	/** True = a manual bolt cycle separates shots; the audible pause is the defender's tell */
	UPROPERTY(EditAnywhere, Category = "Rifle")
	bool bBoltAction = false;

	/** Floor between shots: semi-auto trigger reset, or the full bolt cycle when bBoltAction */
	UPROPERTY(EditAnywhere, Category = "Rifle")
	float FireIntervalSeconds = 0.16f;

	UPROPERTY(EditAnywhere, Category = "Rifle")
	int32 MagazineSize = 8;

	UPROPERTY(EditAnywhere, Category = "Rifle")
	float ReloadSeconds = 2.6f;

	UPROPERTY(EditAnywhere, Category = "Rifle")
	float MuzzleVelocity = 80000.f;

	UPROPERTY(EditAnywhere, Category = "Rifle")
	float HipSpreadDeg = 3.f;

	UPROPERTY(EditAnywhere, Category = "Rifle")
	float AimSpreadDeg = 0.35f;
};
