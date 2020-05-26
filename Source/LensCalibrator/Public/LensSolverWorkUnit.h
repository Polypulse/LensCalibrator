/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Math/Vector2D.h"
#include "OpenCVWrapper.h"

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

struct FLensSolverPixelArrayWorkUnit
{
	FBaseParameters baseParameters;
	FChessboardSearchParameters textureSearchParameters;
	FResizeParameters resizeParameters;
	FPixelArrayParameters pixelArrayParameters;

	FLensSolverPixelArrayWorkUnit() 
	{
	}
};

struct FLensSolverTextureFileWorkUnit
{
	FBaseParameters baseParameters;
	FChessboardSearchParameters textureSearchParameters;
	FTextureFileParameters textureFileParameters;

	FLensSolverTextureFileWorkUnit() 
	{
	}
};

struct FCalibrationPointParameters
{
	// std::vector<cv::Point2f> corners;
	// std::vector<cv::Point3f> objectPoints;
	TArray<FVector2D> corners;
	TArray<FVector> objectPoints;

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
