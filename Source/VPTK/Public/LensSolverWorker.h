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

#include "LensSolverWorker.generated.h"


USTRUCT(BlueprintType)
struct FLensSolverWorkUnit
{
	GENERATED_BODY()

	int width;
	int height;
	FIntPoint cornerCount;
	float zoomLevel;
	float squareSize;

	TArray<FColor> pixels;
};

class FLensSolverWorker : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FLensSolverWorker>;

public:
	DECLARE_DELEGATE_OneParam(OnSolvePointsDel, FSolvedPoints)
	DECLARE_DELEGATE_RetVal(int, GetWorkLoadDel)
	DECLARE_DELEGATE_OneParam(QueueWorkUnitDel, FLensSolverWorkUnit)
	DECLARE_DELEGATE_RetVal(bool, IsClosingDel)

private:
	OnSolvePointsDel onSolvePointsDel;
	// FCriticalSection threadLock;

	TQueue<FLensSolverWorkUnit> workQueue;

	mutable int workUnitCount;
	int workerID;
	mutable bool exited;

	// static UTexture2D * CreateTexture2D(TArray<uint8> * rawData, int width, int height);

	void TransformVectorFromCVToUE4(FVector& v);
	FMatrix GeneratePerspectiveMatrixFromFocalLength (cv::Size & imageSize, cv::Point2d principlePoint, float focalLength);
	FTransform GenerateTransformFromRAndTVecs (std::vector<cv::Mat> & rvecs, std::vector<cv::Mat> & tvecs);
	void QueueSolvedPointsError(float zoomLevel);
	void QueueSolvedPoints(FSolvedPoints solvedPoints);
	bool IsClosing ();

public:
	FLensSolverWorker(
		IsClosingDel * inputIsClosingDel,
		GetWorkLoadDel * inputGetWorkLoadDel,
		QueueWorkUnitDel * inputQueueWorkUnitDel,
		OnSolvePointsDel inputOnSolvePointsDel,
		int inputWorkerID);

	~FLensSolverWorker() 
	{
		// workQueue.Empty();
		// onSolvePointsDel.Unbind();
		exited = true;
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLensSolverWorker, STATGROUP_ThreadPoolAsyncTasks);
	}

protected:

	
	void DoWork();
	int GetWorkLoad ();
	void QueueWorkUnit(FLensSolverWorkUnit workUnit);
};