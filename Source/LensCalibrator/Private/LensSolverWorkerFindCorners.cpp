/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensSolverWorkerFindCorners.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "MatQueueWriter.h"
#include "OpenCVWrapper.h"

FLensSolverWorkerFindCorners::FLensSolverWorkerFindCorners(
	FLensSolverWorkerParameters & inputParameters,
	QueueTextureFileWorkUnitInputDel* inputQueueTextureFileWorkUnitInputDel,
	QueuePixelArrayWorkUnitInputDel* inputQueuePixelArrayWorkUnitInputDel,
	QueueFindCornerResultOutputDel* inputQueueFindCornerResultOutputDel) :
	FLensSolverWorker(inputParameters),
	queueFindCornerResultOutputDel(inputQueueFindCornerResultOutputDel)
{
	inputQueueTextureFileWorkUnitInputDel->BindRaw(this, &FLensSolverWorkerFindCorners::QueueTextureFileWorkUnit);
	inputQueuePixelArrayWorkUnitInputDel->BindRaw(this, &FLensSolverWorkerFindCorners::QueuePixelArrayWorkUnit);

	workUnitCount = 0;
}

int FLensSolverWorkerFindCorners::GetWorkLoad()
{
	int count = 0;
	Lock();
	count = workUnitCount;
	Unlock();

	return count;
}

void FLensSolverWorkerFindCorners::QueueTextureFileWorkUnit(FLensSolverTextureFileWorkUnit workUnit)
{
	textureFileWorkQueue.Enqueue(workUnit);
	Lock();
	workUnitCount++;
	Unlock();

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Queued TextureFileWorkUnit with path: \"%s\", total currently queued: %d."),
		*JobDataToString(workUnit.baseParameters),
		*workUnit.textureFileParameters.absoluteFilePath,
		workUnitCount));
}

void FLensSolverWorkerFindCorners::QueuePixelArrayWorkUnit(FLensSolverPixelArrayWorkUnit workUnit)
{
	pixelArrayWorkQueue.Enqueue(workUnit);
	Lock();
	workUnitCount++;
	Unlock();

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Queued PixelArrayWorkUnit of resolution: (%d, %d), total currently queued: %d."),
		*JobDataToString(workUnit.baseParameters),
		workUnit.resizeParameters.sourceX,
		workUnit.resizeParameters.sourceY,
		workUnitCount));
}

void FLensSolverWorkerFindCorners::DequeueTextureFileWorkUnit(FLensSolverTextureFileWorkUnit& workUnit)
{
	textureFileWorkQueue.Dequeue(workUnit);
	Lock();
	workUnitCount--;
	Unlock();

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Dequeued TextureFileWorkUnit with path: \"%s\"."),
		*JobDataToString(workUnit.baseParameters),
		*workUnit.textureFileParameters.absoluteFilePath));
}

void FLensSolverWorkerFindCorners::DequeuePixelArrayWorkUnit(FLensSolverPixelArrayWorkUnit & workUnit)
{
	pixelArrayWorkQueue.Dequeue(workUnit);
	Lock();
	workUnitCount--;
	Unlock();

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Dequeued PixelArrayWorkUnit of resolution: (%d, %d)."),
		*JobDataToString(workUnit.baseParameters),
		workUnit.resizeParameters.sourceX,
		workUnit.resizeParameters.sourceY));
}

void FLensSolverWorkerFindCorners::Tick()
{
	FBaseParameters baseParameters;
	FResizeParameters resizeParameters;
	FChessboardSearchParameters textureSearchParameters;

	double * data = nullptr;
	if (!textureFileWorkQueue.IsEmpty())
	{
		FLensSolverTextureFileWorkUnit textureFileWorkUnit;

		DequeueTextureFileWorkUnit(textureFileWorkUnit);
		baseParameters = textureFileWorkUnit.baseParameters;
		textureSearchParameters = textureFileWorkUnit.textureSearchParameters;
		resizeParameters.nativeX = textureFileWorkUnit.textureSearchParameters.nativeFullResolutionX;
		resizeParameters.nativeY = textureFileWorkUnit.textureSearchParameters.nativeFullResolutionY;

		if (!GetOpenCVWrapper().ProcessImageFromFile(
			resizeParameters,
			textureFileWorkUnit.textureSearchParameters,
			std::string(TCHAR_TO_UTF8(*textureFileWorkUnit.textureFileParameters.absoluteFilePath)),
			data))
		{
			QueueEmptyCalibrationPointsWorkUnit(baseParameters, resizeParameters);
			return;
		}
	}

	else if (!pixelArrayWorkQueue.IsEmpty())
	{
		FLensSolverPixelArrayWorkUnit texturePixelArrayUnit;
		DequeuePixelArrayWorkUnit(texturePixelArrayUnit);
		baseParameters = texturePixelArrayUnit.baseParameters;
		textureSearchParameters = texturePixelArrayUnit.textureSearchParameters;
		resizeParameters = CalculateResizeParameters(textureSearchParameters);
		resizeParameters.resizeX = texturePixelArrayUnit.resizeParameters.resizeX;
		resizeParameters.resizeY = texturePixelArrayUnit.resizeParameters.resizeY;

		if (!GetOpenCVWrapper().ProcessImageFromPixels(
			resizeParameters,
			texturePixelArrayUnit.textureSearchParameters,
			reinterpret_cast<uint8_t*>(texturePixelArrayUnit.pixelArrayParameters.pixels.GetData()),
			4,
			resizeParameters.resizeX,
			resizeParameters.resizeY,
			data))
		{
			QueueEmptyCalibrationPointsWorkUnit(baseParameters, resizeParameters);
			return;
		}
	}

	else return;

	TArray<double> corners;

	float inverseResizeRatio = resizeParameters.nativeX / (float)resizeParameters.resizeX;

	corners.SetNum(textureSearchParameters.checkerBoardCornerCountX * textureSearchParameters.checkerBoardCornerCountY * 2);

	for (int ci = 0; ci < textureSearchParameters.checkerBoardCornerCountX * textureSearchParameters.checkerBoardCornerCountY; ci++)
	{
		corners[ci * 2] = *(data + ci * 2) * inverseResizeRatio;
		corners[ci * 2 + 1] = *(data + ci * 2 + 1) * inverseResizeRatio;
	}

	FLensSolverCalibrationPointsWorkUnit calibrationPointsWorkUnit;

	calibrationPointsWorkUnit.baseParameters								= baseParameters;
	calibrationPointsWorkUnit.calibrationPointParameters.corners			= corners;

	calibrationPointsWorkUnit.calibrationPointParameters.cornerCountX				= textureSearchParameters.checkerBoardCornerCountX;
	calibrationPointsWorkUnit.calibrationPointParameters.cornerCountY				= textureSearchParameters.checkerBoardCornerCountY;
	calibrationPointsWorkUnit.calibrationPointParameters.chessboardSquareSizeMM		= textureSearchParameters.checkerBoardSquareSizeMM;

	calibrationPointsWorkUnit.resizeParameters								= resizeParameters;

	QueueCalibrationPointsWorkUnit(calibrationPointsWorkUnit);
}

void FLensSolverWorkerFindCorners::QueueCalibrationPointsWorkUnit(const FLensSolverCalibrationPointsWorkUnit & calibrationPointsWorkUnit)
{
	if (!queueFindCornerResultOutputDel->IsBound())
		return;
	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Queuing calibration points work unit."), *JobDataToString(calibrationPointsWorkUnit.baseParameters)));
	queueFindCornerResultOutputDel->Execute(calibrationPointsWorkUnit);
}

void FLensSolverWorkerFindCorners::QueueEmptyCalibrationPointsWorkUnit(const FBaseParameters & baseParameters, const FResizeParameters & resizeParameters)
{
	if (!queueFindCornerResultOutputDel->IsBound())
		return;

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Queuing EMPTY calibration points work unit."), *JobDataToString(baseParameters)));

	FLensSolverCalibrationPointsWorkUnit calibrationPointsWorkUnit;
	calibrationPointsWorkUnit.baseParameters = baseParameters;
	calibrationPointsWorkUnit.resizeParameters = resizeParameters;
	queueFindCornerResultOutputDel->Execute(calibrationPointsWorkUnit);
}

FResizeParameters FLensSolverWorkerFindCorners::CalculateResizeParameters(const FChessboardSearchParameters & textureSearchParameters)
{
	FResizeParameters resizeParameters;

	resizeParameters.nativeX = textureSearchParameters.nativeFullResolutionX;
	resizeParameters.nativeY = textureSearchParameters.nativeFullResolutionY;

	if (textureSearchParameters.resize)
	{
		resizeParameters.resizeX = FMath::FloorToInt(resizeParameters.sourceX * textureSearchParameters.resizePercentage);
		resizeParameters.resizeY = FMath::FloorToInt(resizeParameters.sourceY * textureSearchParameters.resizePercentage);
	}

	else
	{
		resizeParameters.resizeX = resizeParameters.sourceX;
		resizeParameters.resizeY = resizeParameters.sourceY;
	}

	return resizeParameters;
}
