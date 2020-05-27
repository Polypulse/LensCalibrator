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
#include "MatQueueWriter.h"

#include "WorkerRegistry.h"

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

bool ULensSolver::Debug()
{
	static IConsoleVariable* variable = IConsoleManager::Get().FindConsoleVariable(TEXT("LensCalibrator.Debug"));
	if (variable != nullptr && variable->GetInt() == 0)
		return false;
	return true;
}

std::string ULensSolver::PrepareDebugOutputPath(const FString & debugOutputPath)
{
	static FString defaultMatFolder = LensSolverUtilities::GenerateGenericOutputPath("DebugImages");
	static FString defaultFileName = "DebugImage";
	static FString defaultExtension = "jpg";

	FString outputPath = debugOutputPath;
	if (!LensSolverUtilities::ValidateFilePath(outputPath, defaultMatFolder, defaultFileName, defaultExtension))
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to use use path: \"%s\" to write an image to file."), *outputPath);
		return std::string("");
	}

	return TCHAR_TO_UTF8(*outputPath);
}

void ULensSolver::OneTimeProcessArrayOfTextureFolderZoomPairs(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	TArray<FTextureFolderZoomPair> inputTextures, 
	FOneTimeProcessParameters oneTimeProcessParameters, 
	FJobInfo& ouptutJobInfo)
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
	for (int ti = 0; ti < inputTextures.Num(); ti++)
		useCount += inputTextures[ti].use;

	TArray<TArray<FString>> imageFiles;
	TArray<int> expectedImageCounts;
	TArray<float> zoomLevels;

	imageFiles.SetNum(useCount);
	expectedImageCounts.SetNum(useCount);
	zoomLevels.SetNum(useCount);

	for (int ti = 0; ti < inputTextures.Num(); ti++)
	{
		useIndex = ti - offset;
		TArray<FString> imagesInDirectory;
		TArray<UTexture2D*> textures;

		if (!inputTextures[ti].use)
		{
			offset++;
			continue;
		}

		if (!LensSolverUtilities::GetFilesInFolder(inputTextures[ti].absoluteFolderPath, imageFiles[useIndex]))
			return;

		if (imageFiles[useIndex].Num() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("No textures in directory: \"%s\", canceled job."), *inputTextures[ti].absoluteFolderPath);
			return;
		}

		expectedImageCounts[useIndex] = imageFiles[useIndex].Num();
		zoomLevels[useIndex] = inputTextures[ti].zoomLevel;
	}

	LensSolverWorkDistributor::GetInstance().SetCalibrateWorkerParameters(oneTimeProcessParameters.calibrationParameters);
	ouptutJobInfo = LensSolverWorkDistributor::GetInstance().RegisterJob(eventReceiver, expectedImageCounts, useCount, OneTime);
	for (int ci = 0; ci < useCount; ci++)
	{
		for (int ii = 0; ii < imageFiles[ci].Num(); ii++)
		{
			FLensSolverTextureFileWorkUnit workUnit;
			workUnit.baseParameters.jobID						= ouptutJobInfo.jobID;
			workUnit.baseParameters.calibrationID				= ouptutJobInfo.calibrationIDs[ci];
			workUnit.baseParameters.zoomLevel					= zoomLevels[ci];
			workUnit.baseParameters.friendlyName				= FPaths::GetBaseFilename(imageFiles[ci][ii]);

			// workUnit.textureSearchParameters					= oneTimeProcessParameters.textureSearchParameters;

			// Ugly, but this is temporary. Currently trying to refactor a larger part of the program.
			workUnit.textureSearchParameters.nativeFullResolutionX = oneTimeProcessParameters.textureSearchParameters.nativeFullResolution.X;
			workUnit.textureSearchParameters.nativeFullResolutionY = oneTimeProcessParameters.textureSearchParameters.nativeFullResolution.Y;
			workUnit.textureSearchParameters.resizePercentage = oneTimeProcessParameters.textureSearchParameters.resizePercentage;
			workUnit.textureSearchParameters.resize = oneTimeProcessParameters.textureSearchParameters.resize;
			workUnit.textureSearchParameters.flipX = oneTimeProcessParameters.textureSearchParameters.flipX,
			workUnit.textureSearchParameters.flipY = oneTimeProcessParameters.textureSearchParameters.flipY;
			workUnit.textureSearchParameters.exhaustiveSearch = oneTimeProcessParameters.textureSearchParameters.exhaustiveSearch;
			workUnit.textureSearchParameters.checkerBoardSquareSizeMM = oneTimeProcessParameters.textureSearchParameters.checkerBoardSquareSizeMM;
			workUnit.textureSearchParameters.checkerBoardCornerCountX = oneTimeProcessParameters.textureSearchParameters.checkerBoardCornerCount.X,
			workUnit.textureSearchParameters.checkerBoardCornerCountY = oneTimeProcessParameters.textureSearchParameters.checkerBoardCornerCount.Y;
			workUnit.textureSearchParameters.writeDebugTextureToFile = oneTimeProcessParameters.textureSearchParameters.writeDebugTextureToFile;
			workUnit.textureSearchParameters.debugTextureOutputPath = PrepareDebugOutputPath(oneTimeProcessParameters.textureSearchParameters.debugTextureOutputPath);

			workUnit.textureFileParameters.absoluteFilePath		= imageFiles[ci][ii];

			LensSolverWorkDistributor::GetInstance().QueueTextureFileWorkUnit(ouptutJobInfo.jobID, workUnit);
		}
	}
}

void ULensSolver::StartMediaStreamCalibration(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FStartMediaStreamParameters mediaStreamParameters,
	FJobInfo& ouptutJobInfo)
{
	if (!ValidateMediaTexture(mediaStreamParameters.mediaStreamParameters.mediaTexture))
	{
		UE_LOG(LogTemp, Error, TEXT("The input MediaStreamParameters member \"Media Texture\" invalid!"));
		return;
	}

	if (mediaStreamParameters.mediaStreamParameters.expectedStreamSnapshotCount <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("The input MediaStreamParameters member \"Expected Stream Snapshot Count\" should be a positive number greater than zero!"));
		return;
	}

	if (mediaStreamParameters.mediaStreamParameters.streamSnapshotIntervalFrequencyInSeconds <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("The input MediaStreamParameters member \"Stream Snapshot Interval Frequency In Seconds\" should be a positive number greater than zero!"));
		return;
	}

	if (mediaStreamParameters.mediaStreamParameters.zoomLevel < 0.0f || mediaStreamParameters.mediaStreamParameters.zoomLevel > 1.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("The input MediaStreamParameters member \"Zoom Level\" should be a normalized value between 0 - 1!"));
		return;
	}

	TArray<int> expectedImageCounts;
	expectedImageCounts.Add(mediaStreamParameters.mediaStreamParameters.expectedStreamSnapshotCount);

	LensSolverWorkDistributor::GetInstance().SetCalibrateWorkerParameters(mediaStreamParameters.calibrationParameters);
	ouptutJobInfo = LensSolverWorkDistributor::GetInstance().RegisterJob(eventReceiver, expectedImageCounts, 1, OneTime);

	FMediaStreamWorkUnit workUnit;
	workUnit.baseParameters.jobID								= ouptutJobInfo.jobID;
	workUnit.baseParameters.calibrationID						= ouptutJobInfo.calibrationIDs[0];
	workUnit.baseParameters.friendlyName						= "stream";
	workUnit.baseParameters.zoomLevel							= mediaStreamParameters.mediaStreamParameters.zoomLevel;
	// workUnit.textureSearchParameters							= mediaStreamParameters.textureSearchParameters;

	workUnit.textureSearchParameters.nativeFullResolutionX = mediaStreamParameters.textureSearchParameters.nativeFullResolution.X;
	workUnit.textureSearchParameters.nativeFullResolutionY = mediaStreamParameters.textureSearchParameters.nativeFullResolution.Y;
	workUnit.textureSearchParameters.resizePercentage = mediaStreamParameters.textureSearchParameters.resizePercentage;
	workUnit.textureSearchParameters.resize = mediaStreamParameters.textureSearchParameters.resize;
	workUnit.textureSearchParameters.flipX = mediaStreamParameters.textureSearchParameters.flipX,
	workUnit.textureSearchParameters.flipY = mediaStreamParameters.textureSearchParameters.flipY;
	workUnit.textureSearchParameters.exhaustiveSearch = mediaStreamParameters.textureSearchParameters.exhaustiveSearch;
	workUnit.textureSearchParameters.checkerBoardSquareSizeMM = mediaStreamParameters.textureSearchParameters.checkerBoardSquareSizeMM;
	workUnit.textureSearchParameters.checkerBoardCornerCountX = mediaStreamParameters.textureSearchParameters.checkerBoardCornerCount.X,
	workUnit.textureSearchParameters.checkerBoardCornerCountY = mediaStreamParameters.textureSearchParameters.checkerBoardCornerCount.Y;
	workUnit.textureSearchParameters.writeDebugTextureToFile = mediaStreamParameters.textureSearchParameters.writeDebugTextureToFile;
	workUnit.textureSearchParameters.debugTextureOutputPath = PrepareDebugOutputPath(mediaStreamParameters.textureSearchParameters.debugTextureOutputPath);

	workUnit.mediaStreamParameters								= mediaStreamParameters.mediaStreamParameters;
	workUnit.mediaStreamParameters.currentStreamSnapshotCount	= 0;

	LensSolverWorkDistributor::GetInstance().QueueMediaStreamWorkUnit(workUnit);
}

void ULensSolver::StartBackgroundImageProcessors(int findCornersWorkerCount, int calibrateWorkerCount)
{
	if (WorkerRegistry::Get().WorkersRunning())
	{
		UE_LOG(LogTemp, Error, TEXT("You already have workers running, stop them before starting more."));
		return;
	}

	LensSolverWorkDistributor::GetInstance().Configure(queueLogOutputDel, queueFinishedJobOutputDel);

	if (queueLogOutputDel->IsBound())
		queueLogOutputDel->Unbind();

	queueLogOutputDel->BindUObject(this, &ULensSolver::QueueLog);

	UE_LOG(LogTemp, Log, TEXT("Binded log queue."));

	// queueFinishedJobOutputDel->Unbind();
	if (queueFinishedJobOutputDel->IsBound())
		queueFinishedJobOutputDel->Unbind();

	queueFinishedJobOutputDel->BindUObject(this, &ULensSolver::QueueFinishedJob);

	UE_LOG(LogTemp, Log, TEXT("Binded finished queue."));

	LensSolverWorkDistributor::GetInstance().PrepareFindCornerWorkers(findCornersWorkerCount);
	LensSolverWorkDistributor::GetInstance().PrepareCalibrateWorkers(calibrateWorkerCount);
}

void ULensSolver::StopBackgroundImageprocessors()
{
	LensSolverWorkDistributor::GetInstance().StopBackgroundWorkers();
}

/*
#pragma optimize("", off)
void ULensSolver::OnReceiveCalibrationResult_Implementation(FCalibrationResult calibrationResult)
{
	UE_LOG(LogTemp, Log, TEXT("On Received Calibration Result Implementation"));
}
#pragma optimize("", on)
*/

void ULensSolver::PollLogs()
{
	while (!logQueue.IsEmpty())
	{
		FString msg;
		logQueue.Dequeue(msg);
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

void ULensSolver::Poll()
{
	PollCalibrationResults();
	PollFinishedJobs();

	LensSolverWorkDistributor::GetInstance().PollMediaTextureStreams();
	GetMatQueueWriter().Poll();

	PollLogs();
}
