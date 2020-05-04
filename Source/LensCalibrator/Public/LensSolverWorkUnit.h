#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#pragma push_macro("check")
#undef check
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/types_c.h"
#pragma pop_macro("check")
#include <vector>

#include "LensSolverWorkUnit.generated.h"

USTRUCT(BlueprintType)
struct FLensSolverWorkUnit
{
	FString jobID;
	FString calibrationID;
	FString friendlyName;
	int index;
};

USTRUCT(BlueprintType)
struct FLensSolverTextureWorkUnit : FLensSolverWorkUnit
{
	GENERATED_BODY()
	TArray<FColor> pixels;
};

USTRUCT(BlueprintType)
struct FLensSolverFileWorkUnit : FLensSolverWorkUnit
{
	GENERATED_BODY()
	FString absoluteFilePath;
};

USTRUCT(BlueprintType)
struct FLensSolverCalibrateWorkUnit : FLensSolverWorkUnit
{
	GENERATED_BODY()

	std::vector<std::vector<cv::Point2f>> corners;
	std::vector<std::vector<cv::Point3f>> objectPoints;
};
