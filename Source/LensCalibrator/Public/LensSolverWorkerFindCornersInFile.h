#pragma once
#include "LensSolverWorker.h"
#include "LensSolverWorkerFindCorners.h"

class FLensSolverWorkerFindCornersInFile : public FLensSolverWorkerFindCorners
{
private:
	TQueue<FLensSolverWorkerFindCornersInFile> workQueue;
protected:
	virtual bool GetImage(cv::Mat& image) override;
	virtual void QueueWorkUnit(TUniquePtr<FLensSolverWorkUnit> workUnit) override;
	virtual bool WorkUnitInQueue() override;
public:
};