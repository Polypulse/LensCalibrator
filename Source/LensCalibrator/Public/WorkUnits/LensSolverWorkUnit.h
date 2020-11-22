/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Math/Vector2D.h"
#include "OpenCVWrapper.h"

#include "LensSolverWorkerParameters.h"

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
	TArray<float> corners;
	float chessboardSquareSizeMM;
	int cornerCountX;
	int cornerCountY;

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
	FChessboardSearchParameters textureSearchParameters;
	FMediaStreamParameters mediaStreamParameters;
};
