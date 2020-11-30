/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensSolver.h"

#include <Runtime\Engine\Classes\Engine\Texture.h>
#include <Runtime\Engine\Classes\Engine\Texture2D.h>

#include "TextureResource.h"
#include "CoreTypes.h"
#include "GlobalShader.h"
#include "RHIStaticStates.h"
#include "Engine/RendererSettings.h"
#include "PixelShaderUtils.h"
#include "Engine.h"
#include "ImagePixelData.h"
#include "ImageWriteStream.h"
#include "ImageWriteTask.h"
#include "ImageWriteQueue.h"

#include "LensSolverUtilities.h"
#include "BlitShader.h"
#include "WorkerRegistry.h"

#include "MatQueueWriter.h"
#include "WrapperInterface.h"

bool ULensSolver::ValidateMediaTexture(const UMediaTexture* inputTexture)
{
	if (inputTexture == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot process null texture."));
		return false;
	}

	if (inputTexture->GetWidth() <= 3 ||
		inputTexture->GetHeight() <= 3)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot process texture, it's to small."));
		return false;
	}

	return true;
}

void ULensSolver::QueueFinishedJob(FinishedJobQueueContainer queueContainer)
{
	queuedFinishedJobs.Enqueue(queueContainer);
}

bool ULensSolver::FinishedJobIsQueued()
{
	return queuedFinishedJobs.IsEmpty() == false;
}

void ULensSolver::DequeuedFinishedJob(FinishedJobQueueContainer& queueContainer)
{
	queuedFinishedJobs.Dequeue(queueContainer);
}

void ULensSolver::QueueLog(FString msg)
{
	logQueue.Enqueue(msg);
}

/* Determine if debug mode is enabled or not. */
bool ULensSolver::Debug()
{
	static IConsoleVariable* variable = IConsoleManager::Get().FindConsoleVariable(TEXT("LensCalibrator.Debug"));
	if (variable != nullptr && variable->GetInt() == 0)
		return false;
	return true;
}

FString ULensSolver::PrepareDebugOutputPath(const FString & debugOutputPath)
{
	static FString defaultMatFolder = LensSolverUtilities::GenerateGenericOutputPath("DebugImages");
	static FString defaultFileName = "DebugImage";
	static FString defaultExtension = "jpg";

	FString outputPath = debugOutputPath;
	if (!LensSolverUtilities::ValidateFilePath(outputPath, defaultMatFolder, defaultFileName, defaultExtension))
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to use use path: \"%s\" to write an image to file."), *outputPath);
		return FString("");
	}

	return outputPath;
}

void ULensSolver::OneTimeProcessArrayOfTextureFolderZoomPairs(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	TArray<FTextureFolderZoomPair> inputTextures, 
	FTextureSearchParameters textureSearchParameters,
	FCalibrationParameters calibrationParameters,
	FJobInfo & ouptutJobInfo)
{
	if (inputTextures.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No input texture folders."));
		return;
	}

	if (LensSolverWorkDistributor::GetInstance().GetFindCornerWorkerCount() <= 0 || LensSolverWorkDistributor::GetInstance().GetCalibrateCount() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No workers available, make sure you start both background \"FindCorner\" & \"Calibrate\" workers."));
		return;
	}

	int useCount = 0, useIndex = 0, offset = 0;
	/* Initially loop through all the texture folders to determine
	if any of the zoom levels are disabled/enabled and count the
	enabled ones so that we can pre-allocate our arrays with
	the correct size. */
	for (int ti = 0; ti < inputTextures.Num(); ti++)
		useCount += inputTextures[ti].use;

	TArray<TArray<FString>> imageFiles;
	TArray<int> expectedImageCounts;
	TArray<float> zoomLevels;

	/* Preallocate our arrays. */
	imageFiles.SetNum(useCount);
	expectedImageCounts.SetNum(useCount);
	zoomLevels.SetNum(useCount);

	/* Loop through folders each containing sets of textures representing a 
	particular zoom level of the calibration pattern. */
	for (int ti = 0; ti < inputTextures.Num(); ti++)
	{
		/* If we are skipping a folder, then shift the index so we can
		keep the index consistent. */
		useIndex = ti - offset;

		TArray<FString> imagesInDirectory;
		TArray<UTexture2D*> textures;

		if (!inputTextures[ti].use)
		{
			offset++;
			continue;
		}

		/* Fill imageFiles array at each zoom level with absolute file path to texture. */
		if (!LensSolverUtilities::GetImageFilesInFolder(inputTextures[ti].absoluteFolderPath, imageFiles[useIndex]))
			return;

		if (imageFiles[useIndex].Num() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("No textures in directory: \"%s\", canceled job."), *inputTextures[ti].absoluteFolderPath);
			return;
		}

		/* Store the expected number of images and zoom level for calibration. */
		expectedImageCounts[useIndex] = imageFiles[useIndex].Num();
		zoomLevels[useIndex] = inputTextures[ti].zoomLevel;
	}

	LensSolverWorkDistributor::GetInstance().SetCalibrateWorkerParameters(calibrationParameters);
	ouptutJobInfo = LensSolverWorkDistributor::GetInstance().RegisterJob(eventReceiver, expectedImageCounts, useCount, UJobType::OneTime);

	/* Loop through zoom levels. */
	for (int ci = 0; ci < useCount; ci++)
	{
		/* Loop through images for zoom level, build work unit and 
		queue that work unit to be consumed by the workers. */
		for (int ii = 0; ii < imageFiles[ci].Num(); ii++)
		{
			FLensSolverTextureFileWorkUnit workUnit;
			workUnit.baseParameters.jobID						= ouptutJobInfo.jobID;
			workUnit.baseParameters.calibrationID				= ouptutJobInfo.calibrationIDs[ci];
			workUnit.baseParameters.zoomLevel					= zoomLevels[ci];
			workUnit.baseParameters.friendlyName				= FPaths::GetBaseFilename(imageFiles[ci][ii]);

			/* Ugly */
			workUnit.textureSearchParameters.nativeFullResolutionX					= textureSearchParameters.nativeFullResolution.X;
			workUnit.textureSearchParameters.nativeFullResolutionY					= textureSearchParameters.nativeFullResolution.Y;
			workUnit.textureSearchParameters.resizePercentage						= textureSearchParameters.resizePercentage;
			workUnit.textureSearchParameters.resize									= textureSearchParameters.resize;
			workUnit.textureSearchParameters.flipX									= textureSearchParameters.flipX,
			workUnit.textureSearchParameters.flipY									= textureSearchParameters.flipY;
			workUnit.textureSearchParameters.exhaustiveSearch						= textureSearchParameters.exhaustiveSearch;
			workUnit.textureSearchParameters.checkerBoardSquareSizeMM				= textureSearchParameters.checkerBoardSquareSizeMM;
			workUnit.textureSearchParameters.checkerBoardCornerCountX				= textureSearchParameters.checkerBoardCornerCount.X,
			workUnit.textureSearchParameters.checkerBoardCornerCountY				= textureSearchParameters.checkerBoardCornerCount.Y;

			workUnit.textureFileParameters.absoluteFilePath							= imageFiles[ci][ii];

			/* Setup debug output texture paths. */
			workUnit.textureSearchParameters.writeCornerVisualizationTextureToFile	= textureSearchParameters.writeCornerVisualizationTextureToFile;
			FillCharArrayFromFString(workUnit.textureSearchParameters.cornerVisualizationTextureOutputPath, PrepareDebugOutputPath(textureSearchParameters.cornerVisualizationTextureOutputPath));
			workUnit.textureSearchParameters.writePreCornerDetectionTextureToFile	= textureSearchParameters.writePreCornerDetectionTextureToFile;
			FillCharArrayFromFString(workUnit.textureSearchParameters.preCornerDetectionTextureOutputPath, PrepareDebugOutputPath(textureSearchParameters.preCornerDetectionTextureOutputPath));

			/* Queue the work unit to be consumed by the workers. */
			LensSolverWorkDistributor::GetInstance().QueueTextureFileWorkUnit(ouptutJobInfo.jobID, workUnit);
		}
	}
}

/* Start calibration from a media stream and pass in corner search parameters, calibration 
parameters and media stream texture parameters. Also, the workers need to be started
and idling before you can start this job. */
void ULensSolver::StartMediaStreamCalibration(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FTextureSearchParameters textureSearchParameters,
	FCalibrationParameters calibrationParameters,
	FMediaStreamParameters mediaStreamParameters,
	FJobInfo& ouptutJobInfo)
{
	if (LensSolverWorkDistributor::GetInstance().GetFindCornerWorkerCount() <= 0 || LensSolverWorkDistributor::GetInstance().GetCalibrateCount() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No workers available, make sure you start both background \"FindCorner\" & \"Calibrate\" workers."));
		return;
	}

	if (!ValidateMediaTexture(mediaStreamParameters.mediaTexture))
	{
		UE_LOG(LogTemp, Error, TEXT("The input MediaStreamParameters member \"Media Texture\" invalid!"));
		return;
	}

	if (mediaStreamParameters.expectedStreamSnapshotCount <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("The input MediaStreamParameters member \"Expected Stream Snapshot Count\" should be a positive number greater than zero!"));
		return;
	}

	if (mediaStreamParameters.streamSnapshotIntervalFrequencyInSeconds <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("The input MediaStreamParameters member \"Stream Snapshot Interval Frequency In Seconds\" should be a positive number greater than zero!"));
		return;
	}

	if (mediaStreamParameters.zoomLevel < 0.0f || mediaStreamParameters.zoomLevel > 1.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("The input MediaStreamParameters member \"Zoom Level\" should be a normalized value between 0 - 1!"));
		return;
	}

	TArray<int> expectedImageCounts;
	expectedImageCounts.Add(mediaStreamParameters.expectedStreamSnapshotCount);

	/* Set the calibration parameters for all workers. */
	LensSolverWorkDistributor::GetInstance().SetCalibrateWorkerParameters(calibrationParameters);
	ouptutJobInfo = LensSolverWorkDistributor::GetInstance().RegisterJob(eventReceiver, expectedImageCounts, 1, UJobType::Continuous);

	/* Setup work unit. */
	FMediaStreamWorkUnit workUnit;
	workUnit.baseParameters.jobID												= ouptutJobInfo.jobID; /* Associate this work unit with a job. */
	workUnit.baseParameters.calibrationID										= ouptutJobInfo.calibrationIDs[0]; /* Associate this work unit with a calibration ID (There can be multiple calibration IDs per job). */
	workUnit.baseParameters.friendlyName										= "stream";
	workUnit.baseParameters.zoomLevel											= mediaStreamParameters.zoomLevel;

	workUnit.textureSearchParameters.nativeFullResolutionX						= textureSearchParameters.nativeFullResolution.X;
	workUnit.textureSearchParameters.nativeFullResolutionY						= textureSearchParameters.nativeFullResolution.Y;
	workUnit.textureSearchParameters.resizePercentage							= textureSearchParameters.resizePercentage;
	workUnit.textureSearchParameters.resize										= textureSearchParameters.resize;
	workUnit.textureSearchParameters.flipX										= textureSearchParameters.flipX,
	workUnit.textureSearchParameters.flipY										= textureSearchParameters.flipY;
	workUnit.textureSearchParameters.exhaustiveSearch							= textureSearchParameters.exhaustiveSearch;
	workUnit.textureSearchParameters.checkerBoardSquareSizeMM					= textureSearchParameters.checkerBoardSquareSizeMM;
	workUnit.textureSearchParameters.checkerBoardCornerCountX					= textureSearchParameters.checkerBoardCornerCount.X,
	workUnit.textureSearchParameters.checkerBoardCornerCountY					= textureSearchParameters.checkerBoardCornerCount.Y;
	workUnit.mediaStreamParameters												= mediaStreamParameters;
	workUnit.mediaStreamParameters.currentStreamSnapshotCount					= 0;

	/* Setup debug output texture paths. */
	workUnit.textureSearchParameters.writeCornerVisualizationTextureToFile		= textureSearchParameters.writeCornerVisualizationTextureToFile;
	FillCharArrayFromFString(workUnit.textureSearchParameters.cornerVisualizationTextureOutputPath, PrepareDebugOutputPath(textureSearchParameters.cornerVisualizationTextureOutputPath));
	workUnit.textureSearchParameters.writePreCornerDetectionTextureToFile		= textureSearchParameters.writePreCornerDetectionTextureToFile;
	FillCharArrayFromFString(workUnit.textureSearchParameters.preCornerDetectionTextureOutputPath, PrepareDebugOutputPath(textureSearchParameters.preCornerDetectionTextureOutputPath));

	/* Queue the work unit for the workers to consume. */
	LensSolverWorkDistributor::GetInstance().QueueMediaStreamWorkUnit(workUnit);
}

/* Start find corner and calibration background workers. */
void ULensSolver::StartBackgroundImageProcessors(int findCornersWorkerCount, int calibrateWorkerCount, bool shutDownWorkersAfterCompletingTasks)
{
	/* Are the workers already running? */
	if (WorkerRegistry::Get().WorkersRunning())
	{
		UE_LOG(LogTemp, Error, TEXT("You already have workers running, stop them before starting more."));
		return;
	}

	LensSolverWorkDistributor::GetInstance().Configure(queueLogOutputDel, queueFinishedJobOutputDel, shutDownWorkersAfterCompletingTasks);

	if (queueLogOutputDel->IsBound())
		queueLogOutputDel->Unbind();

	queueLogOutputDel->BindUObject(this, &ULensSolver::QueueLog);

	UE_LOG(LogTemp, Log, TEXT("Binded log queue."));

	if (queueFinishedJobOutputDel->IsBound())
		queueFinishedJobOutputDel->Unbind();

	queueFinishedJobOutputDel->BindUObject(this, &ULensSolver::QueueFinishedJob);

	UE_LOG(LogTemp, Log, TEXT("Binded finished queue."));

	LensSolverWorkDistributor::GetInstance().PrepareWorkers(findCornersWorkerCount, calibrateWorkerCount);
}

void ULensSolver::StopBackgroundImageprocessors()
{
	LensSolverWorkDistributor::GetInstance().StopBackgroundWorkers();
}

void ULensSolver::PollLogs()
{
	/* Keep looping until we have dequeued the all the logs from the workers. */
	while (!logQueue.IsEmpty())
	{
		FString msg;
		logQueue.Dequeue(msg);
		/* Print the dequeued log from the worker threads. */
		UE_LOG(LogTemp, Log, TEXT("%s"), *msg);
	}
}

void ULensSolver::PollCalibrationResults()
{
	bool isQueued = LensSolverWorkDistributor::GetInstance().CalibrationResultIsQueued();
	ULensSolver * lensSolver = this;
	while (isQueued)
	{
		CalibrationResultQueueContainer queueContainer;
		LensSolverWorkDistributor::GetInstance().DequeueCalibrationResult(queueContainer);

		UE_LOG(LogTemp, Log, TEXT("(INFO): Dequeued calibration result of id: \"%s\" for job of id: \"%s\"."), 
			*queueContainer.calibrationResult.baseParameters.calibrationID, 
			*queueContainer.calibrationResult.baseParameters.jobID);

		if (queueContainer.eventReceiver.GetObject()->IsValidLowLevel())
			ILensSolverEventReceiver::Execute_OnReceiveCalibrationResult(queueContainer.eventReceiver.GetObject(), queueContainer.calibrationResult);

		isQueued = LensSolverWorkDistributor::GetInstance().CalibrationResultIsQueued();
	}
}

void ULensSolver::PollFinishedJobs()
{
	bool isQueued = queuedFinishedJobs.IsEmpty() == false;
	while (isQueued)
	{
		FinishedJobQueueContainer queueContainer;
		DequeuedFinishedJob(queueContainer);
		UE_LOG(LogTemp, Log, TEXT("Completed job: \"%s\", job will be unregistered."), *queueContainer.jobInfo.jobID);

		if (queueContainer.eventReceiver.GetObject()->IsValidLowLevel())
			ILensSolverEventReceiver::Execute_OnFinishedJob(queueContainer.eventReceiver.GetObject(), queueContainer.jobInfo);

		isQueued = queuedFinishedJobs.IsEmpty() == false;
	}
}

/* Called each frame from blueprints to keep the calibration system ticking. */
void ULensSolver::Poll()
{
	PollCalibrationResults();
	PollFinishedJobs();

	LensSolverWorkDistributor::GetInstance().PollMediaTextureStreams();
	GetMatQueueWriter().Poll(Debug());

	PollLogs();
	WrapperInterface::PollLog();
}
