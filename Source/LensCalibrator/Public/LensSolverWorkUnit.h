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

#include "LensSolverWorkerParameters.h"

/*
UENUM(BlueprintType)
enum class ELensSolverWorkUnitType : uint8
{
	PixelArray,
	TextureFile,
	Calibrate
};
*/

struct FLensSolverTextureWorkUnit
{
	FBaseParameters baseParameters;
	FTextureSearchParameters textureSearchParameters;

	FLensSolverTextureWorkUnit() 
	{
	}
};

struct FPixelArrayParameters
{
	FIntPoint sourceResolution;
	TArray<FColor> pixels;
	FPixelArrayParameters()
	{
		sourceResolution = FIntPoint(0, 0);
		pixels = TArray<FColor>();
	}
};

struct FLensSolverPixelArrayWorkUnit
{
	FBaseParameters baseParameters;
	FTextureSearchParameters textureSearchParameters;
	FPixelArrayParameters pixelArrayParameters;

	FLensSolverPixelArrayWorkUnit() 
	{
	}
};

struct FLensSolverTextureFileWorkUnit
{
	FBaseParameters baseParameters;
	FTextureSearchParameters textureSearchParameters;
	FTextureFileParameters textureFileParameters;

	FLensSolverTextureFileWorkUnit() 
	{
	}
};

struct FResizeParameters
{
	FIntPoint sourceResolution;
	FIntPoint resizeResolution;
};

struct FCalibrationPointParameters
{
	std::vector<cv::Point2f> corners;
	std::vector<cv::Point3f> objectPoints;

	FCalibrationPointParameters()
	{
	}
};

struct FLensSolverCalibrationPointsWorkUnit
{
	FBaseParameters baseParameters;
	FCalibrationPointParameters calibrationPointParameters;
	FResizeParameters resizeParameters;

	FLensSolverCalibrationPointsWorkUnit() {}
};

struct FCalibrateLatch
{
	FBaseParameters baseParameters;
	FCalibrationParameters calibrationParameters;
	FResizeParameters resizeParameters;

	FCalibrateLatch()
	{
	}
};

