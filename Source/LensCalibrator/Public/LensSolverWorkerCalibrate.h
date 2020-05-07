#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "LatchData.h"
#include "LensSolverWorker.h"

DECLARE_DELEGATE_OneParam(QueueCalibrationResultOutputDel, FCalibrationResult)
DECLARE_DELEGATE_OneParam(QueueCalibrateWorkUnitInputDel, FLensSolverCalibrationPointsWorkUnit)
DECLARE_DELEGATE_OneParam(QueueLatchInputDel, const FCalibrateLatch)

class FLensSolverWorkerCalibrate : public FLensSolverWorker
{
public:

	FLensSolverWorkerCalibrate(
		const FLensSolverWorkerParameters & inputParameters,
		QueueCalibrateWorkUnitInputDel* inputQueueCalibrateWorkUnitDel,
		QueueLatchInputDel* inputSignalLatch,
		const QueueCalibrationResultOutputDel* inputOnSolvePointsDel);

	~FLensSolverWorkerCalibrate() {}

private:
	const QueueCalibrationResultOutputDel * onSolvePointsDel;

	TMap<FString, TQueue<FLensSolverCalibrationPointsWorkUnit>*> workQueue;
	TQueue<FCalibrateLatch> latchQueue;

	FMatrix GeneratePerspectiveMatrixFromFocalLength(cv::Size& imageSize, cv::Point2d principlePoint, float focalLength);
	FTransform GenerateTransformFromRAndTVecs(std::vector<cv::Mat>& rvecs, std::vector<cv::Mat>& tvecs);
	void WriteSolvedPointsToJSONFile(const FCalibrationResult& solvePoints, FString folder, const FString fileName);
	void TransformVectorFromCVToUE4(FVector& v);

	void QueueSolvedPointsError(const FString& jobID, const float zoomLevel);
	void QueueSolvedPoints(FCalibrationResult solvedPoints);

	void QueueLatch(const FCalibrateLatch latchData);
	void DequeueLatch(FCalibrateLatch & latchDataPtr);

	bool DequeueAllWorkUnits(
		const FString calibrationID, 
		std::vector<std::vector<cv::Point2f>> & corners,
		std::vector<std::vector<cv::Point3f>> & objectPoints);

	bool LatchInQueue();

	void QueueWorkUnit(FLensSolverCalibrationPointsWorkUnit calibrateWorkUnit);

protected:
	virtual void Tick() override;
	virtual int GetWorkLoad() override;
};