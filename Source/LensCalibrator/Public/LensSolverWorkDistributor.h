#pragma once
#include "LensSolverWorkerInterfaceContainer.h"
#include "LensSolverWorkUnit.h"
#include "FindCornerWorkerParameters.h"
#include "CalibrationWorkerParameters.h"
#include "LatchData.h"

class FLensSolverWorkDisttributor
{
private:
	TArray<FWorkerInterfaceContainer> findCornersWorkers;
	TArray<FWorkerInterfaceContainer> calibrateWorkers;

	TMap<FString, TUniquePtr<FFindCornerWorkerParameters>> jobFindCornerWorkerParameters;
	TMap<FString, TUniquePtr<FCalibrationWorkerParameters>> jobCalibrationWorkerParameters;

protected:
public:
	void StartBackgroundWorkers(int findCornerWorkerCount, int calibrateWorkerCount);
	bool GetFindCornerWorkerParameters(TUniquePtr<FFindCornerWorkerParameters>& findCornerWorkerParameters);
	bool GetCalibrationWorkerParameters(TUniquePtr<FCalibrationWorkerParameters>& calibrationWorkerParameters);
	void StopBackgroundWorkers();
};