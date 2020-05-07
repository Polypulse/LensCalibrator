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

public:

private:

	FTexture2DRHIRef blitRenderTexture;
	bool blitRenderTextureAllocated;

	FTexture2DRHIRef distortionCorrectionRenderTexture;
	bool distortionCorrectionRenderTextureAllocated;

	FTexture2DRHIRef correctDistortedTextureRenderTexture;
	bool correctDistortedTextureRenderTextureAllocated;

	FLensSolverWorkerCalibrate::QueueCalibrationResultOutputDel onSolvePointsDel;
	FLensSolverWorkerParameters::QueueLogOutputDel queueLogOutputDel;

	TSharedPtr<TQueue<FCalibrationResult>> queuedSolvedPointsPtr;
	TSharedPtr<TQueue<FDistortionCorrectionMapGenerationResults>> queuedDistortionCorrectionMapResults;
	TSharedPtr<TQueue<FCorrectedDistortedImageResults>> queuedCorrectedDistortedImageResults;

	TQueue<FString> logQueue;

	TUniquePtr<LensSolverWorkDistributor> workDistributor;

	void RandomSortTArray(TArray<UTexture2D*>& arr);

	void BeginDetectPoints(
		const FJobInfo inputJobInfo,
		const FTextureZoomPair& inputTextureZoomPair,
		FOneTimeProcessParameters oneTimeProcessParameters,
		const bool inputLatch);

	void BeginDetectPoints(
		const FJobInfo jobInfo,
		const TArray<FTextureZoomPair> & inputTextures,
		FOneTimeProcessParameters oneTimeProcessParameters);

	void BeginDetectPoints(
		const FJobInfo inputJobInfo,
		FTextureArrayZoomPair& inputTextures,
		FOneTimeProcessParameters inputOneTimeProcessParameters);

	void BeginDetectPoints(
		const FJobInfo jobInfo,
		TArray<FTextureArrayZoomPair> & inputTextures,
		FOneTimeProcessParameters oneTimeProcessParameters);

	void DetectPointsRenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FJobInfo jobInfo,
		const FTextureZoomPair textureZoomPair,
		FOneTimeProcessParameters oneTimeProcessParameters,
		const int latchImageCount,
		const bool latch);

	void GenerateDistortionCorrectionMapRenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams,
		const FString correctionFilePath,
		const FString inverseCorrectionFilePath);

	void UndistortImageRenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FDistortTextureWithTextureParams distortionCorrectionParams,
		const FString generatedOutputPath);

	bool ValidateZoom(const FJobInfo& jobInfo, const float zoomValue);
	bool ValidateTexture(const FJobInfo & jobInfo, const UTexture2D* inputTexture, const int textureIndex, const FIntPoint targetResolution);
	bool ValidateMediaTexture(const FJobInfo & jobInfo, const UMediaTexture* inputTexture);
	bool ValidateOneTimeProcessParameters(const FOneTimeProcessParameters& oneTimeProcessParameters);
	void ReturnErrorSolvedPoints(FJobInfo jobInfo);

	void PollLogs();
	void PollCalibrationResults ();
	void PollDistortionCorrectionMapGenerationResults ();
	void PollCorrectedDistortedImageResults ();

	void QueueLog(FString msg);

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintNativeEvent, Category="Lens Calibrator")
	void DequeueSolvedPoints (FCalibrationResult solvedPoints);
	virtual void DequeueSolvedPoints_Implementation (FCalibrationResult solvedPoints) {}

	UFUNCTION(BlueprintNativeEvent, Category="Lens Calibrator")
	void FinishedJob (FJobInfo jobInfo);
	virtual void FinishedJob_Implementation (FJobInfo jobInfo) {}

	UFUNCTION(BlueprintNativeEvent, Category="Lens Calibrator")
	void OnGeneratedDistortionMap (UTexture2D * generatedDistortionMap);
	virtual void OnGeneratedDistortionMap_Implementation (UTexture2D * generatedDistortionMap) {}

	UFUNCTION(BlueprintNativeEvent, Category="Lens Calibrator")
	void OnDistortedImageCorrected (UTexture2D * correctedDistortedImage);
	virtual void OnDistortedImageCorrected_Implementation (UTexture2D * correctedDistortedImage) {}

public:

	// ULensSolver() {}

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	bool ValidateMediaInputs (UMediaPlayer * mediaPlayer, UMediaTexture * mediaTexture, FString url);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	void OneTimeProcessTextureZoomPair(
		FTextureZoomPair textureZoomPair,
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo);

	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
		void OneTimeProcessArrayOfTextureZoomPairs(
		TArray<FTextureZoomPair> textureZoomPairArray,
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	void OneTimeProcessTextureArrayZoomPair(
		FTextureArrayZoomPair inputTextures, 
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	void OneTimeProcessArrayOfTextureArrayZoomPairs(
		TArray<FTextureArrayZoomPair> inputTextures, 
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	void OneTimeProcessArrayOfTextureFolderZoomPairs(
		TArray<FTextureFolderZoomPair> inputTextures, 
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo);

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

	void OnSolvedPoints(FCalibrationResult solvedPoints);
};

