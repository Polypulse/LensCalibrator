#include "LensSolverWorkerFindCornersInTexture.h"

bool FLensSolverWorkerFindCornersInTexture::GetImage(cv::Mat& image)
{
	Lock();
	Unlock();
	return false;
}

void FLensSolverWorkerFindCornersInTexture::QueueWorkUnit(TUniquePtr<FLensSolverWorkUnit> workUnit)
{
}

bool FLensSolverWorkerFindCornersInTexture::WorkUnitInQueue()
{
	return false;
}
