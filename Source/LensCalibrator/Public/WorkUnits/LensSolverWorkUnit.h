/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Math/Vector2D.h"
#include "OpenCVWrapper.h"

#include "LensSolverWorkerParameters.h"

/* Work units are containers that contain both configuration parameters and data 
to process. They are feed to back and forth between ULensSolverWorkDistributor
and the background workers. */


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
