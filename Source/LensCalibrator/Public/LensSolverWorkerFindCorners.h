#pragma once
#include "LensSolverWorker.h"

class FLensSolverWorkerFindCorners : public FLensSolverWorker
{
private:
protected:
	virtual void Tick() override;
	virtual bool GetImage(cv::Mat& image) = 0;
public:
	// virtual void QueueWorkUnit(TUniquePtr<FLensSolverWorkUnit> workUnit) = 0;
	// virtual bool WorkUnitInQueue() = 0;
};
