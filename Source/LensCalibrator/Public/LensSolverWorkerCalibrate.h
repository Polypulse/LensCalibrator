#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "LatchData.h"
#include "LensSolverWorker.h"

class FLensSolverWorkerCalibrate : public FLensSolverWorker
{
public:
	DECLARE_DELEGATE_OneParam(QueueCalibrationResultOutputDel, FCalibrationResult)
	DECLARE_DELEGATE_OneParam(QueueLatchInputDel, const FLatchData)

	FLensSolverWorkerCalibrate(
		FLensSolverWorkerParameters inputParameters,
		QueueLatchInputDel * inputSignalLatch,
		QueueCalibrationResultOutputDel inputOnSolvePointsDel
	);

private:
	QueueCalibrationResultOutputDel onSolvePointsDel;
	TQueue<TUniquePtr<FLatchData>> latchQueue;
	FMatrix GeneratePerspectiveMatrixFromFocalLength(cv::Size& imageSize, cv::Point2d principlePoint, float focalLength);
	FTransform GenerateTransformFromRAndTVecs(std::vector<cv::Mat>& rvecs, std::vector<cv::Mat>& tvecs);
	void WriteSolvedPointsToJSONFile(const FCalibrationResult& solvePoints, FString folder, const FString fileName);
	void TransformVectorFromCVToUE4(FVector& v);

	void QueueSolvedPointsError(FJobInfo jobInfo, float zoomLevel);
	void QueueSolvedPoints(FCalibrationResult solvedPoints);
	void QueueLatch(TUniquePtr<FLatchData> latchData);
	void DequeueLatch(TUniquePtr<FLatchData>& latchData);
	bool LatchInqueue();

protected:
	virtual void Tick() override;
	virtual bool WorkUnitInQueue() override;
};