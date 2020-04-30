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
#include "Job.h"
#include "LatchData.h"
#include "WorkerParameters.h"
#include "OneTimeProcessParameters.h"
#include "TextureArrayZoomPair.h"
#include "TextureZoomPair.h"
#include "LensSolver.generated.h"

USTRUCT()
struct FWorkerInterfaceContainer
{
	GENERATED_BODY()

	FAutoDeleteAsyncTask<FLensSolverWorker> * worker;

	FLensSolverWorker::GetWorkLoadDel getWorkLoadDel;
	FLensSolverWorker::QueueWorkUnitDel queueWorkUnitDel;
	FLensSolverWorker::IsClosingDel isClosingDel;
	FLensSolverWorker::SignalLatchDel signalLatch;
};

UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LENSCALIBRATOR_API ULensSolver : public UActorComponent
{
	GENERATED_BODY()

public:



	/*
	DECLARE_DELEGATE_OneParam(FSolvedPointsQueuedDel, bool)
	DECLARE_DELEGATE_OneParam(FDequeueSolvedPointsDel, FCalibrationResult)
	*/

private:

	FTexture2DRHIRef renderTexture;
	UTexture2D * visualizationTexture;
	bool allocated;

	TSharedPtr<TQueue<FCalibrationResult>> queuedSolvedPointsPtr;

	mutable FCriticalSection threadLock;
	TArray<FWorkerInterfaceContainer> workers;
	TMap<FString, FJob> jobs;

	FJobInfo RegisterJob (int latchedWorkUnitCount, UJobType jobType);

	int GetWorkerCount ();

	void BeginDetectPoints(
		const FJobInfo inputJobInfo,
		const FTextureZoomPair& inputTextureZoomPair,
		FOneTimeProcessParameters oneTimeProcessParameters,
		const bool inputLatch);

	/*
	void BeginDetectPoints(
		const FJobInfo inputJobInfo,
		const UMediaTexture* inputMediaTexture,
		const float inputZoomLevel,
		FOneTimeProcessParameters oneTimeProcessParameters);
	*/

	void BeginDetectPoints(
		const FJobInfo jobInfo,
		const TArray<FTextureZoomPair> & inputTextures,
		FOneTimeProcessParameters oneTimeProcessParameters);

	void BeginDetectPoints(
		const FJobInfo jobInfo,
		const FTextureArrayZoomPair& inputTextures,
		FOneTimeProcessParameters oneTimeProcessParameters,
		const bool latch);

	void BeginDetectPoints(
		const FJobInfo jobInfo,
		const TArray<FTextureArrayZoomPair> & inputTextures,
		FOneTimeProcessParameters oneTimeProcessParameters);

	/*
	void BeginDetectPoints(
		const FJobInfo jobInfo,
		TArray<UMediaTexture*> inputTextures,
		TArray<float> inputZoomLevels,
		FOneTimeProcessParameters oneTimeProcessParameters);
	*/

	void DetectPointsRenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FJobInfo jobInfo,
		const FTextureZoomPair textureZoomPair,
		FOneTimeProcessParameters oneTimeProcessParameters,
		bool latch);

	UTexture2D * CreateTexture2D(TArray<FColor> * rawData, int width, int height);
	/*
	void VisualizeCalibration(
		FRHICommandListImmediate& RHICmdList, 
		FSceneViewport* sceneViewport, 
		UTexture2D * visualizationTexture, 
		FCalibrationResult solvedPoints,
		bool flipX,
		bool flipY);
	*/

	bool ValidateZoom(const FJobInfo& jobInfo, const float zoomValue);
	bool ValidateTexture(const FJobInfo & jobInfo, const UTexture2D* inputTexture);
	bool ValidateMediaTexture(const FJobInfo & jobInfo, const UMediaTexture* inputTexture);
	bool ValidateOneTimeProcessParameters(const FOneTimeProcessParameters& oneTimeProcessParameters);
	void ReturnErrorSolvedPoints(FJobInfo jobInfo);

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintNativeEvent, Category="Lens Calibrator")
	void DequeueSolvedPoints (FCalibrationResult solvedPoints);
	virtual void DequeueSolvedPoints_Implementation (FCalibrationResult solvedPoints) {}

	UFUNCTION(BlueprintNativeEvent, Category="Lens Calibrator")
	void FinishedJob (FJobInfo jobInfo);
	virtual void FinishedJob_Implementation (FJobInfo jobInfo) {}

public:

	FLensSolverWorker::OnSolvePointsDel onSolvePointsDel;

	ULensSolver() 
	{
	}

	~ULensSolver() 
	{
	}

	// virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override {}

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	bool ValidateMediaInputs (UMediaPlayer * mediaPlayer, UMediaTexture * mediaTexture, FString url);


	/*
	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
	void OneTimeProcessMediaTexture(
		UMediaTexture* inputMediaTexture,
		float normalizedZoomValue,
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo);
	*/

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

	/*
	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	void OneTimeProcessMediaTextureArray(
		TArray<UMediaTexture*> inputTextures, 
		TArray<float> normalizedZoomValues, 
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo);
	*/

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	void OneTimeProcessTextureArrayZoomPair(
		FTextureArrayZoomPair inputTextures, 
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	void OneTimeProcessTextureArrayOfTextureArrayZoomPairs(
		TArray<FTextureArrayZoomPair> inputTextures, 
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	void StartBackgroundImageProcessors(int workerCount);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	void StopBackgroundImageprocessors();

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	void PollSolvedPoints ();

	void OnSolvedPoints(FCalibrationResult solvedPoints);
};