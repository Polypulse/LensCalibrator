#pragma once

#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "SolvedPoints.h"

#include "LensSolverWorker.generated.h"


USTRUCT(BlueprintType)
struct FLensSolverWorkUnit
{
	GENERATED_BODY()

	int width;
	int height;
	FIntPoint cornerCount;
	float zoomLevel;

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
	mutable bool exited;

	// static UTexture2D * CreateTexture2D(TArray<uint8> * rawData, int width, int height);

	void QueueSolvedPointsError(float zoomLevel);
	void QueueSolvedPoints(FSolvedPoints solvedPoints);
	bool IsClosing ();

public:
	FLensSolverWorker(
		IsClosingDel * inputIsClosingDel,
		GetWorkLoadDel * inputGetWorkLoadDel,
		QueueWorkUnitDel * inputQueueWorkUnitDel,
		OnSolvePointsDel inputOnSolvePointsDel);

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