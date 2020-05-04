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
#include "LensSolverWorkUnit.h"
#include "LensSolverWorker.generated.h"

class FLensSolverWorker : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FLensSolverWorker>;

public:
	DECLARE_DELEGATE_OneParam(OnSolvePointsDel, FCalibrationResult)
	DECLARE_DELEGATE_OneParam(QueueLogDel, FString)
	DECLARE_DELEGATE_RetVal(int, GetWorkLoadDel)
	DECLARE_DELEGATE_OneParam(QueueWorkUnitDel, FLensSolverTextureWorkUnit)
	DECLARE_DELEGATE_OneParam(SignalLatchDel, const FLatchData)
	DECLARE_DELEGATE_RetVal(bool, IsClosingDel)

private:
	FString calibrationVisualizationOutputPath;

	OnSolvePointsDel onSolvePointsDel;
	TQueue<FLensSolverWorkUnit> workQueue;
	TQueue<FLatchData> latchQueue;

	int workerID;
	mutable int workUnitCount;
	mutable bool flagToExit;

	FCriticalSection threadLock;

	// static UTexture2D * CreateTexture2D(TArray<uint8> * rawData, int width, int height);

	void TransformVectorFromCVToUE4(FVector& v);
	FMatrix GeneratePerspectiveMatrixFromFocalLength (cv::Size & imageSize, cv::Point2d principlePoint, float focalLength);
	FTransform GenerateTransformFromRAndTVecs (std::vector<cv::Mat> & rvecs, std::vector<cv::Mat> & tvecs);
	void QueueSolvedPointsError(FJobInfo jobInfo, float zoomLevel);
	void QueueSolvedPoints(FCalibrationResult solvedPoints);
	bool Exit ();
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

	int GetWorkerID();

protected:
	
	void DoWork();
	virtual void Tick() = 0;
	bool ShouldExit();

	void Lock();
	void Unlock();

	int GetWorkLoad ();
	virtual void QueueWorkUnit(TUniquePtr<FLensSolverWorkUnit> workUnit) = 0;
	virtual bool WorkUnitInQueue() = 0;
	void QueueLog(FString log);

	void Latch(const FLatchData inputLatchData);
};