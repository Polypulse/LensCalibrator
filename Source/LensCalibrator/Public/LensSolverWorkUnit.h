/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
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
	TArray<FColor> pixels;
	FPixelArrayParameters()
	{
		pixels = TArray<FColor>();
	}
};

struct FResizeParameters
{
	FIntPoint sourceResolution;
	FIntPoint resizeResolution;
	FResizeParameters()
	{
		sourceResolution = FIntPoint(0, 0);
		resizeResolution = FIntPoint(0, 0);
	}
};


struct FLensSolverPixelArrayWorkUnit
{
	FBaseParameters baseParameters;
	FTextureSearchParameters textureSearchParameters;
	FResizeParameters resizeParameters;
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

struct FMediaStreamWorkUnit
{
	FBaseParameters baseParameters;
	FTextureSearchParameters textureSearchParameters;
	FMediaStreamParameters mediaStreamParameters;
};
