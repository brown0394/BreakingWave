#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BlendSpaceTool.generated.h"

/**
 * Editing a blendspace's sample_data from Python does not rebuild its serialized runtime
 * triangulation (BlendSpaceData) — the engine only rebuilds it inside the Persona editor
 * widget, which crashes when opened offscreen. These entry points let headless tool scripts
 * rebuild and inspect that data directly.
 */
UCLASS()
class UBlendSpaceTool : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "BreakingWave|Tools")
	static FString DescribeRuntimeTriangulation(class UBlendSpace* BlendSpace);

	UFUNCTION(BlueprintCallable, Category = "BreakingWave|Tools")
	static FString RebuildRuntimeTriangulation(class UBlendSpace* BlendSpace);

	UFUNCTION(BlueprintCallable, Category = "BreakingWave|Tools")
	static FString DescribeBlendOutputAt(class UBlendSpace* BlendSpace, FVector BlendInput);
};
