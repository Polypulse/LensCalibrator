#include "LensSolverWorkerFindCornersInFile.h"

bool FLensSolverWorkerFindCornersInFile::GetImage(cv::Mat& image)
{
	Lock();
	Unlock();
	return false;
}

void FLensSolverWorkerFindCornersInFile::QueueWorkUnit(TUniquePtr<FLensSolverWorkUnit> workUnit)
{
}

bool FLensSolverWorkerFindCornersInFile::WorkUnitInQueue()
{
	return false;
}
