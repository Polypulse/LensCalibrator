#pragma once
#include "LensSolverWorker.h"

class FLensSolverWorkerCalibrate : public FLensSolverWorker
{
private:
	TQueue<FLensSolverCalibrateWorkUnit> workQueue;
protected:
	virtual void Tick() override;
	virtual void QueueWorkUnit(TUniquePtr<FLensSolverWorkUnit> workUnit) override;
	virtual bool WorkUnitInQueue() override;
public:
};