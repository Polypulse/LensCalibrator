/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */


#include "LensSolverWorkerFindCorners.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "MatQueueWriter.h"
#include "OpenCVWrapper.h"

#include "WorkerRegistry.h"

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
	WorkerRegistry::Get().CountFindCornerWorker();
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

	TArray<float> corners;

	if (!textureFileWorkQueue.IsEmpty())
	{
		FLensSolverTextureFileWorkUnit textureFileWorkUnit;

		DequeueTextureFileWorkUnit(textureFileWorkUnit);
		baseParameters				= textureFileWorkUnit.baseParameters;
		textureSearchParameters		= textureFileWorkUnit.textureSearchParameters;
		resizeParameters.nativeX	= textureFileWorkUnit.textureSearchParameters.nativeFullResolutionX;
		resizeParameters.nativeY	= textureFileWorkUnit.textureSearchParameters.nativeFullResolutionY;

		corners.SetNum(textureSearchParameters.checkerBoardCornerCountX * textureSearchParameters.checkerBoardCornerCountY * 2);

		DeclareCharArrayFromFString(absoluteFilePath, textureFileWorkUnit.textureFileParameters.absoluteFilePath);

		if (!GetOpenCVWrapper().ProcessImageFromFile(
			resizeParameters,
			textureFileWorkUnit.textureSearchParameters,
			absoluteFilePath,
			corners.GetData(),
			Debug()))
		{
			QueueEmptyCalibrationPointsWorkUnit(baseParameters, resizeParameters);
			return;
		}
	}

	else if (!pixelArrayWorkQueue.IsEmpty())
	{
		FLensSolverPixelArrayWorkUnit texturePixelArrayUnit;
		DequeuePixelArrayWorkUnit(texturePixelArrayUnit);

		baseParameters				= texturePixelArrayUnit.baseParameters;
		textureSearchParameters		= texturePixelArrayUnit.textureSearchParameters;
		resizeParameters			= CalculateResizeParameters(textureSearchParameters);
		resizeParameters.resizeX	= texturePixelArrayUnit.resizeParameters.resizeX;
		resizeParameters.resizeY	= texturePixelArrayUnit.resizeParameters.resizeY;

		corners.SetNum(textureSearchParameters.checkerBoardCornerCountX * textureSearchParameters.checkerBoardCornerCountY * 2);

		uint8_t * pixelData = reinterpret_cast<uint8_t*>(texturePixelArrayUnit.pixelArrayParameters.pixels.GetData());
		float * cornersData = corners.GetData();
		bool debug = Debug();

		if (!GetOpenCVWrapper().ProcessImageFromPixels(
			resizeParameters,
			texturePixelArrayUnit.textureSearchParameters,
			pixelData,
			4,
			resizeParameters.resizeX,
			resizeParameters.resizeY,
			cornersData,
			debug))
		{
			QueueEmptyCalibrationPointsWorkUnit(baseParameters, resizeParameters);
			return;
		}
	}

	else return;

	FLensSolverCalibrationPointsWorkUnit calibrationPointsWorkUnit;

	calibrationPointsWorkUnit.baseParameters										= baseParameters;
	calibrationPointsWorkUnit.calibrationPointParameters.corners					= corners;
	calibrationPointsWorkUnit.calibrationPointParameters.cornerCountX				= textureSearchParameters.checkerBoardCornerCountX;
	calibrationPointsWorkUnit.calibrationPointParameters.cornerCountY				= textureSearchParameters.checkerBoardCornerCountY;
	calibrationPointsWorkUnit.calibrationPointParameters.chessboardSquareSizeMM		= textureSearchParameters.checkerBoardSquareSizeMM;
	calibrationPointsWorkUnit.resizeParameters										= resizeParameters;

	if (ShouldExit())
		return;

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

	calibrationPointsWorkUnit.baseParameters	= baseParameters;
	calibrationPointsWorkUnit.resizeParameters	= resizeParameters;

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

void FLensSolverWorkerFindCorners::NotifyShutdown()
{
	WorkerRegistry::Get().UncountFindCornerWorker();
}
