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

UENUM(BlueprintType)
enum ELensSolverWorkUnitType
{
	PixelArray,
	TextureFile,
	Calibrate
};

USTRUCT(BlueprintType)
struct FLensSolverWorkUnit
{
	FString jobID;
	FString calibrationID;
	FString friendlyName;

	ELensSolverWorkUnitType workUnitType;

	int index;
};

struct FLensSolverTextureWorkUnit : FLensSolverWorkUnit
{
	GENERATED_BODY()
	float resizePercentage;
	bool resize;

	bool exhaustiveSearch;
	bool writeDebugTextureToFile;

	float checkerBoardSquareSizeMM;
	FIntPoint checkerBoardCornerCount;

	FString debugTextureFolderPath;
};

USTRUCT(BlueprintType)
struct FLensSolverPixelArrayWorkUnit : FLensSolverTextureWorkUnit
{
	GENERATED_BODY()
	FIntPoint sourceResolution;
	TArray<FColor> pixels;
};

USTRUCT(BlueprintType)
struct FLensSolverTextureFileWorkUnit : FLensSolverTextureWorkUnit
{
	GENERATED_BODY()
	FString absoluteFilePath;
};

USTRUCT(BlueprintType)
struct FLensSolverCalibrateWorkUnit : FLensSolverWorkUnit
{
	GENERATED_BODY()

	std::vector<cv::Point2f> corners;
	std::vector<cv::Point3f> objectPoints;
};
