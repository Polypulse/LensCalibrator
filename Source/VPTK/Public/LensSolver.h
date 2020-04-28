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
#include "WorkerParameters.h"
#include "OneTimeProcessParameters.h"
#include "LensSolver.generated.h"

USTRUCT()
struct FWorkerInterfaceContainer
{
	GENERATED_BODY()

	FAutoDeleteAsyncTask<FLensSolverWorker> * worker;

	FLensSolverWorker::GetWorkLoadDel getWorkLoadDel;
	FLensSolverWorker::QueueWorkUnitDel queueWorkUnitDel;
	FLensSolverWorker::IsClosingDel isClosingDel;
};

UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VPTK_API ULensSolver : public UActorComponent
{
	GENERATED_BODY()

public:



	/*
	DECLARE_DELEGATE_OneParam(FSolvedPointsQueuedDel, bool)
	DECLARE_DELEGATE_OneParam(FDequeueSolvedPointsDel, FSolvedPoints)
	*/

private:

	FTexture2DRHIRef renderTexture;
	UTexture2D * visualizationTexture;
	bool allocated;

	TQueue<FSolvedPoints> queuedSolvedPoints;
	TSharedPtr<TQueue<FSolvedPoints>> queuedSolvedPointsPtr;

	mutable FCriticalSection threadLock;
	TArray<FWorkerInterfaceContainer> workers;
	TMap<FString, FJob> jobs;

	FJobInfo RegisterJob (int workUnitCount, UJobType jobType);

	int GetWorkerCount ();

	bool ValidateCommonVariables(
		FIntPoint cornerCount,
		float inputZoomLevel,
		float inputSquareSize
	);

	void BeginDetectPoints(
		FJobInfo inputJobInfo,
		UTexture2D* inputTexture, 
		float inputZoomLevel,
		FOneTimeProcessParameters firstPassParameters,
		TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints);

	void BeginDetectPoints(
		FJobInfo inputJobInfo,
		UMediaTexture* inputMediaTexture, 
		float inputZoomLevel,
		FOneTimeProcessParameters firstPassParameters,
		TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints);

	void BeginDetectPoints(
		FJobInfo jobInfo,
		TArray<UTexture2D*> inputTextures,
		TArray<float> inputZoomLevels,
		FOneTimeProcessParameters oneTimeProcessParameters,
		TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints);

	void BeginDetectPoints(
		FJobInfo jobInfo,
		TArray<UMediaTexture*> inputTextures, 
		TArray<float> inputZoomLevels, 
		FOneTimeProcessParameters oneTimeProcessParameters,
		TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints);

	void DetectPointsRenderThread(
		FRHICommandListImmediate& RHICmdList, 
		FJobInfo jobInfo,
		UTexture* texture, 
		float normalizedZoomLevel,
		FOneTimeProcessParameters firstPassParameters,
		TSharedPtr<TQueue<FSolvedPoints>> queuedSolvedPoints);

	UTexture2D * CreateTexture2D(TArray<FColor> * rawData, int width, int height);
	void VisualizeCalibration(
		FRHICommandListImmediate& RHICmdList, 
		FSceneViewport* sceneViewport, 
		UTexture2D * visualizationTexture, 
		FSolvedPoints solvedPoints,
		bool flipX,
		bool flipY);

	void FireWorkers();

protected:

	/*
	UPROPERTY(BlueprintAssignable, Category="VPTK")
	FSolvedPointsQueuedDel cachedSolvePointsQueuedDel;

	UPROPERTY(BlueprintAssignable, Category="VPTK")
	FDequeueSolvedPointsDel cachedDequeueSolvedPointsDel;
	*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintNativeEvent, Category="VPTK")
	void DequeueSolvedPoints (FSolvedPoints solvedPoints);
	virtual void DequeueSolvedPoints_Implementation (FSolvedPoints solvedPoints) {}

	UFUNCTION(BlueprintNativeEvent, Category="VPTK")
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

	UFUNCTION(BlueprintCallable, Category="VPTK")
	bool ValidateMediaInputs (UMediaPlayer * mediaPlayer, UMediaTexture * mediaTexture, FString url);

	UFUNCTION(BlueprintCallable, Category = "VPTK")
	FJobInfo OneTimeProcessMediaTexture(
		UMediaTexture* inputMediaTexture,
		float normalizedZoomValue,
		FOneTimeProcessParameters oneTimeProcessParameters);

	UFUNCTION(BlueprintCallable, Category="VPTK")
	FJobInfo OneTimeProcessTexture2D(
		UTexture2D* inputTexture, 
		float normalizedZoomValue, 
		FOneTimeProcessParameters oneTimeProcessParameters);

	UFUNCTION(BlueprintCallable, Category="VPTK")
	FJobInfo OneTimeProcessTexture2DArray(
		TArray<UTexture2D*> inputTextures, 
		TArray<float> normalizedZoomValues, 
		FOneTimeProcessParameters oneTimeProcessParameters);

	UFUNCTION(BlueprintCallable, Category="VPTK")
	FJobInfo OneTimeProcessMediaTextureArray(
		TArray<UMediaTexture*> inputTextures, 
		TArray<float> normalizedZoomValues, 
		FOneTimeProcessParameters oneTimeProcessParameters);

	UFUNCTION(BlueprintCallable, Category="VPTK")
	void StartBackgroundImageProcessors(int workerCount);

	UFUNCTION(BlueprintCallable, Category="VPTK")
	void StopBackgroundImageprocessors();

	UFUNCTION(BlueprintCallable, Category="VPTK")
	void PollSolvedPoints ();

	void OnSolvedPoints(FSolvedPoints solvedPoints);
};