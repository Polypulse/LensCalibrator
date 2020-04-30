/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once

#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "SolvedPoints.h"

#pragma push_macro("check")
#undef check
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/types_c.h"
#pragma pop_macro("check")

#include "JobInfo.h"
#include "LatchData.h"
#include "WorkerParameters.h"
#include "LensSolverWorker.generated.h"


USTRUCT(BlueprintType)
struct FLensSolverWorkUnit
{
	GENERATED_BODY()

	FString unitName;

	FIntPoint cornerCount;
	float zoomLevel;
	int index;
	float squareSize;

	TArray<FColor> pixels;
};

class FLensSolverWorker : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FLensSolverWorker>;

public:
	DECLARE_DELEGATE_OneParam(OnSolvePointsDel, FCalibrationResult)
	DECLARE_DELEGATE_RetVal(int, GetWorkLoadDel)
	DECLARE_DELEGATE_OneParam(QueueWorkUnitDel, FLensSolverWorkUnit)
	DECLARE_DELEGATE_OneParam(SignalLatchDel, const FLatchData)
	DECLARE_DELEGATE_RetVal(bool, IsClosingDel)

private:
	FString calibrationVisualizationOutputPath;

	OnSolvePointsDel onSolvePointsDel;
	// FCriticalSection threadLock;

	TQueue<FLensSolverWorkUnit> workQueue;

	mutable int latchedWorkUnitCount;
	mutable int workUnitCount;
	int workerID;

	mutable bool latched;
	mutable bool exited;

	mutable FLatchData latchData;

	// static UTexture2D * CreateTexture2D(TArray<uint8> * rawData, int width, int height);

	void TransformVectorFromCVToUE4(FVector& v);
	FMatrix GeneratePerspectiveMatrixFromFocalLength (cv::Size & imageSize, cv::Point2d principlePoint, float focalLength);
	FTransform GenerateTransformFromRAndTVecs (std::vector<cv::Mat> & rvecs, std::vector<cv::Mat> & tvecs);
	void QueueSolvedPointsError(FJobInfo jobInfo, float zoomLevel);
	void QueueSolvedPoints(FCalibrationResult solvedPoints);
	bool IsClosing ();
	FString GenerateIndexedFilePath(const FString& folder, const FString& fileName, const FString & extension);
	bool ValidateFolder(FString& folder, const FString & workerMessage);
	void WriteMatToFile(cv::Mat image, FString folder, FString fileName, const FString & workerMessage);
	void WriteSolvedPointsToJSONFile(const FCalibrationResult& solvePoints, FString folder, FString fileName, const FString workerMessage);

public:
	FLensSolverWorker(
		IsClosingDel * inputIsClosingDel,
		GetWorkLoadDel * inputGetWorkLoadDel,
		QueueWorkUnitDel * inputQueueWorkUnitDel,
		SignalLatchDel * inputSignalLatch,
		OnSolvePointsDel inputOnSolvePointsDel,
		int inputWorkerID);

	~FLensSolverWorker() 
	{
		// workQueue.Empty();
		// onSolvePointsDel.Unbind();
		// exited = true;
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLensSolverWorker, STATGROUP_ThreadPoolAsyncTasks);
	}

protected:
	
	void DoWork();
	int GetWorkLoad ();
	void QueueWorkUnit(FLensSolverWorkUnit workUnit);
	void Latch(const FLatchData inputLatchData);
};