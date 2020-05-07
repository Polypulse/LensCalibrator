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
enum class ELensSolverWorkUnitType : uint8
{
	PixelArray,
	TextureFile,
	Calibrate
};

struct FLensSolverWorkUnit
{
	FString jobID;
	FString calibrationID;
	FString friendlyName;
	ELensSolverWorkUnitType workUnitType;
	FLensSolverWorkUnit () {}
};

struct FLensSolverTextureWorkUnit
{
	FLensSolverWorkUnit baseUnit;
	float resizePercentage;
	bool resize;
	bool exhaustiveSearch;
	bool writeDebugTextureToFile;
	float checkerBoardSquareSizeMM;
	FIntPoint checkerBoardCornerCount;
	FString debugTextureFolderPath;
	FLensSolverTextureWorkUnit() {}
};

struct FLensSolverPixelArrayWorkUnit
{
	FLensSolverTextureWorkUnit textureUnit;
	FIntPoint sourceResolution;
	TArray<FColor> pixels;
	FLensSolverPixelArrayWorkUnit() {}
};

struct FLensSolverTextureFileWorkUnit
{
	FLensSolverTextureWorkUnit textureUnit;
	FString absoluteFilePath;
	FLensSolverTextureFileWorkUnit() {}
};

struct FLensSolverCalibrateWorkUnit
{
	FLensSolverWorkUnit baseUnit;

	std::vector<cv::Point2f> corners;
	std::vector<cv::Point3f> objectPoints;
	FLensSolverCalibrateWorkUnit() {}
};
