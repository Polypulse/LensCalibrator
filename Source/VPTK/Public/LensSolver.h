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

	int GetWorkerCount ();

	bool ValidateCommonVariables(
		FIntPoint cornerCount,
		float inputZoomLevel,
		float inputSquareSize
	);

	void BeginDetectPoints(
		UTexture2D* inputTexture,
		float inputZoomLevel, 
		FIntPoint cornerCount, 
		float inputSquareSize,
		TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints);

	void BeginDetectPoints(
		UMediaTexture* inputMediaTexture,
		float inputZoomLevel,
		FIntPoint cornerCount,
		float inputSquaresize,
		TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints);

	void BeginDetectPoints(
		TArray<UTexture2D*> inputTextures,
		TArray<float> inputZoomLevels, 
		FIntPoint cornerCount,
		float inputSquareSize,
		TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints);

	void BeginDetectPoints(
		TArray<UMediaTexture*> inputTextures, 
		TArray<float> inputZoomLevels, 
		FIntPoint cornerCount, 
		float inputSquareSize, 
		TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints);

	void DetectPointsRenderThread(
		FRHICommandListImmediate& RHICmdList, 
		UTexture* texture, 
		int width, 
		int height, 
		float zoomLevel, 
		float squareSize, 
		FIntPoint cornerCount,
		TSharedPtr<TQueue<FSolvedPoints>> queuedSolvedPoints);

	UTexture2D * CreateTexture2D(TArray<FColor> * rawData, int width, int height);
	void VisualizeCalibration(FRHICommandListImmediate& RHICmdList, FSceneViewport* sceneViewport, UTexture2D * visualizationTexture, FSolvedPoints solvedPoints);

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
	void SolvedPointsQueued (bool & isQueued);
	virtual void SolvedPointsQueued_Implementation (bool & isQueued) {}

	UFUNCTION(BlueprintNativeEvent, Category="VPTK")
	void DequeueSolvedPoints (FSolvedPoints solvedPoints);
	virtual void DequeueSolvedPoints_Implementation (FSolvedPoints solvedPoints) {}

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

	UFUNCTION(BlueprintCallable, Category="VPTK")
	void ProcessMediaTexture(UMediaTexture* inputMediaTexture, float normalizedZoomValue, FIntPoint cornerCount, float squareSize);

	UFUNCTION(BlueprintCallable, Category="VPTK")
	void ProcessTexture2D(UTexture2D* inputTexture, float normalizedZoomValue, FIntPoint cornerCount, float squareSize);

	UFUNCTION(BlueprintCallable, Category="VPTK")
	void ProcessTexture2DArray(TArray<UTexture2D*> inputTextures, TArray<float> normalizedZoomValues, FIntPoint cornerCount, float squareSize);

	UFUNCTION(BlueprintCallable, Category="VPTK")
	void ProcessMediaTextureArray(TArray<UMediaTexture*> inputTextures, TArray<float> normalizedZoomValues, FIntPoint cornerCount, float squareSize);

	UFUNCTION(BlueprintCallable, Category="VPTK")
	void StartBackgroundImageProcessors();

	UFUNCTION(BlueprintCallable, Category="VPTK")
	void StopBackgroundImageprocessors();

	UFUNCTION(BlueprintCallable, Category="VPTK")
	void PollSolvedPoints ();

	void OnSolvedPoints(FSolvedPoints solvedPoints);
};