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

#include "LensSolver.generated.h"

UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LENSCALIBRATOR_API ULensSolver : public UActorComponent
{
	GENERATED_BODY()

private:
	FTexture2DRHIRef blitRenderTexture;
	bool blitRenderTextureAllocated;

	FTexture2DRHIRef distortionCorrectionRenderTexture;
	bool distortionCorrectionRenderTextureAllocated;

	FTexture2DRHIRef correctDistortedTextureRenderTexture;
	bool correctDistortedTextureRenderTextureAllocated;

	QueueLogOutputDel * queueLogOutputDel;
	QueueFinishedJobOutputDel * queueFinishedJobOutputDel;

	TQueue<FJobInfo> queuedFinishedJobs;
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

	void QueueFinishedJob(FJobInfo jobInfo);
	bool FinishedJobIsQueued();
	void DequeuedFinishedJob(FJobInfo& jobInfo);
	void QueueLog(FString msg);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintNativeEvent, Category="Lens Calibrator")
	void OnReceiveCalibrationResult (FCalibrationResult calibrationResult);
	virtual void OnReceiveCalibrationResult_Implementation (FCalibrationResult calibrationResult) {}

	UFUNCTION(BlueprintNativeEvent, Category="Lens Calibrator")
	void OnFinishedJob (FJobInfo jobInfo);
	virtual void OnFinishedJob_Implementation (FJobInfo jobInfo) {}

	UFUNCTION(BlueprintNativeEvent, Category="Lens Calibrator")
	void OnGeneratedDistortionMap (UTexture2D * generatedDistortionMap);
	virtual void OnGeneratedDistortionMap_Implementation (UTexture2D * generatedDistortionMap) {}

	UFUNCTION(BlueprintNativeEvent, Category="Lens Calibrator")
	void OnDistortedImageCorrected (UTexture2D * correctedDistortedImage);
	virtual void OnDistortedImageCorrected_Implementation (UTexture2D * correctedDistortedImage) {}

public:

	~ULensSolver() 
	{
		if (queueLogOutputDel != nullptr)
			queueLogOutputDel->Unbind();
		if (queueFinishedJobOutputDel != nullptr)
			queueFinishedJobOutputDel->Unbind();
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool debug;

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	void OneTimeProcessArrayOfTextureFolderZoomPairs(
		TArray<FTextureFolderZoomPair> inputTextures, 
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	void StartMediaStreamCalibration(
		FStartMediaStreamParameters mediaStreamParameters,
		FJobInfo& ouptutJobInfo);

	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
	void GenerateDistortionCorrectionMap(
		FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams);

	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
	void DistortTextureWithTexture(
		FDistortTextureWithTextureParams distortionCorrectionParams);

	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
	void DistortTextureWithTextureFile(
		FDistortTextureWithTextureFileParams distortionCorrectionParams);

	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
	void DistortTextureWithCoefficients(
		FDistortTextureWithCoefficientsParams distortionCorrectionParams);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	void StartBackgroundImageProcessors(int findCornersWorkerCount, int calibrateWorkerCount);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	void StopBackgroundImageprocessors();

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	void Poll ();
};

