/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraComponent.h"
#include "MediaAssets/Public/MediaTexture.h"
#include "MediaAssets/Public/MediaPlayer.h"
#include "CommonRenderResources.h"

#include "SolvedPoints.h"
#include "LensSolverWorker.h"
#include "LensSolverWorkerCalibrate.h"
#include "Job.h"
#include "LensSolverWorkDistributor.h"
#include "OneTimeProcessParameters.h"
#include "StartMediaStreamParameters.h"

#include "DistortionCorrectionMapGenerationParameters.h"
#include "DistortionCorrectionMapGenerationResults.h"
#include "DistortTextureWithCoefficientsParams.h"
#include "DistortTextureWithTextureFileParams.h"
#include "DistortTextureWithTextureParams.h"

#include "CorrectedDistortedImageResults.h"

#include "TextureArrayZoomPair.h"
#include "TextureFolderZoomPair.h"
#include "TextureZoomPair.h"

#include "LensSolverWorkerInterfaceContainer.h"
#include "QueueContainers.h"

#include "ILensSolverEventReceiver.h"

#include "LensSolver.generated.h"

UCLASS()
class LENSCALIBRATOR_API ULensSolver : public UObject
{
	GENERATED_BODY()

private:
	QueueLogOutputDel * queueLogOutputDel;
	QueueFinishedJobOutputDel * queueFinishedJobOutputDel;

	TQueue<FinishedJobQueueContainer> queuedFinishedJobs;
	TSharedPtr<TQueue<FDistortionCorrectionMapGenerationResults>> queuedDistortionCorrectionMapResults;
	TSharedPtr<TQueue<FCorrectedDistortedImageResults>> queuedCorrectedDistortedImageResults;

	TQueue<FString> logQueue;

	void GenerateDistortionCorrectionMapRenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams,
		const FString correctionFilePath,
		const FString inverseCorrectionFilePath);

	void UndistortImageRenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FDistortTextureWithTextureParams distortionCorrectionParams,
		const FString generatedOutputPath);

	bool ValidateMediaTexture(const UMediaTexture* inputTexture);

	void PollLogs();
	void PollCalibrationResults ();
	void PollFinishedJobs();
	void PollDistortionCorrectionMapGenerationResults ();
	void PollCorrectedDistortedImageResults ();

	void QueueFinishedJob(FinishedJobQueueContainer queueContainer);
	bool FinishedJobIsQueued();
	void DequeuedFinishedJob(FinishedJobQueueContainer &queueContainer);
	void QueueLog(FString msg);

public:

	ULensSolver() {}
	~ULensSolver() 
	{
		if (queueLogOutputDel != nullptr)
			queueLogOutputDel->Unbind();
		if (queueFinishedJobOutputDel != nullptr)
			queueFinishedJobOutputDel->Unbind();
	}

	bool debug;

	void OneTimeProcessArrayOfTextureFolderZoomPairs(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		TArray<FTextureFolderZoomPair> inputTextures, 
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo);

	void StartMediaStreamCalibration(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		FStartMediaStreamParameters mediaStreamParameters,
		FJobInfo& ouptutJobInfo);

	void GenerateDistortionCorrectionMap(
		FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams);

	void DistortTextureWithTexture(
		FDistortTextureWithTextureParams distortionCorrectionParams);

	void DistortTextureWithTextureFile(
		FDistortTextureWithTextureFileParams distortionCorrectionParams);

	void DistortTextureWithCoefficients(
		FDistortTextureWithCoefficientsParams distortionCorrectionParams);

	void StartBackgroundImageProcessors(int findCornersWorkerCount, int calibrateWorkerCount);
	void StopBackgroundImageprocessors();

	void Poll ();

protected:

	/*
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintImplementableEvent, Category="Lens Calibrator")
	void OnReceiveCalibrationResult (FCalibrationResult calibrationResult);
	// void OnReceiveCalibrationResult_Implementation(FCalibrationResult calibrationResult);

	UFUNCTION(BlueprintImplementableEvent, Category="Lens Calibrator")
	void OnFinishedJob (FJobInfo jobInfo);
	// virtual void OnFinishedJob_Implementation (FJobInfo jobInfo) {}

	UFUNCTION(BlueprintImplementableEvent, Category="Lens Calibrator")
	void OnGeneratedDistortionMap (UTexture2D * generatedDistortionMap);
	// virtual void OnGeneratedDistortionMap_Implementation (UTexture2D * generatedDistortionMap) {}

	UFUNCTION(BlueprintImplementableEvent, Category="Lens Calibrator")
	void OnDistortedImageCorrected (UTexture2D * correctedDistortedImage);
	// virtual void OnDistortedImageCorrected_Implementation (UTexture2D * correctedDistortedImage) {}
	*/

};

