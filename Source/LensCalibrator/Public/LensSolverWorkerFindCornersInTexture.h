
#pragma once
#include "LensSolverWorker.h"
#include "LensSolverWorkerFindCorners.h"

class FLensSolverWorkerFindCornersInTexture : public FLensSolverWorkerFindCorners
{
private:
	TQueue<FLensSolverTextureWorkUnit> workQueue;
protected:
	virtual bool GetImage(cv::Mat& image) override;
	virtual void QueueWorkUnit(TUniquePtr<FLensSolverWorkUnit> workUnit) override;
	virtual bool WorkUnitInQueue() override;
public:
};
