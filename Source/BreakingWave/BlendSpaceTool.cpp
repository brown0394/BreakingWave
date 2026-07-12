#include "BlendSpaceTool.h"
#include "Animation/BlendSpace.h"

FString UBlendSpaceTool::DescribeRuntimeTriangulation(UBlendSpace* BlendSpace)
{
#if WITH_EDITOR
	if (BlendSpace == nullptr)
	{
		return TEXT("ERROR: null blendspace");
	}
	const FBlendSpaceData& Data = BlendSpace->GetBlendSpaceData();
	int32 MaxTriangleSampleIndex = INDEX_NONE;
	for (const FBlendSpaceTriangle& Triangle : Data.Triangles)
	{
		for (const int32 SampleIndex : Triangle.SampleIndices)
		{
			MaxTriangleSampleIndex = FMath::Max(MaxTriangleSampleIndex, SampleIndex);
		}
	}
	return FString::Printf(TEXT("samples=%d triangles=%d segments=%d max_triangle_sample_index=%d"),
		BlendSpace->GetNumberOfBlendSamples(), Data.Triangles.Num(), Data.Segments.Num(), MaxTriangleSampleIndex);
#else
	return TEXT("ERROR: editor builds only");
#endif
}

FString UBlendSpaceTool::DescribeBlendOutputAt(UBlendSpace* BlendSpace, FVector BlendInput)
{
	if (BlendSpace == nullptr)
	{
		return TEXT("ERROR: null blendspace");
	}
	TArray<FBlendSampleData> SampleDataList;
	int32 CachedTriangulationIndex = INDEX_NONE;
	if (!BlendSpace->GetSamplesFromBlendInput(BlendInput, SampleDataList, CachedTriangulationIndex, true))
	{
		return FString::Printf(TEXT("input=(%s) NO SAMPLES"), *BlendInput.ToCompactString());
	}
	FString Result = FString::Printf(TEXT("input=(%s)"), *BlendInput.ToCompactString());
	for (const FBlendSampleData& Sample : SampleDataList)
	{
		const UAnimSequence* Animation = Sample.Animation;
		Result += FString::Printf(TEXT(" [idx=%d w=%.3f %s]"),
			Sample.SampleDataIndex, Sample.TotalWeight,
			Animation ? *Animation->GetName() : TEXT("<null>"));
	}
	return Result;
}

FString UBlendSpaceTool::RebuildRuntimeTriangulation(UBlendSpace* BlendSpace)
{
#if WITH_EDITOR
	if (BlendSpace == nullptr)
	{
		return TEXT("ERROR: null blendspace");
	}
	BlendSpace->ValidateSampleData();
	BlendSpace->ResampleData();
	BlendSpace->MarkPackageDirty();
	return DescribeRuntimeTriangulation(BlendSpace);
#else
	return TEXT("ERROR: editor builds only");
#endif
}
