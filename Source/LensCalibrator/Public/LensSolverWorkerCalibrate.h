/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "LensSolverWorker.h"

DECLARE_DELEGATE_OneParam(QueueCalibrationResultOutputDel, FCalibrationResult)
DECLARE_DELEGATE_OneParam(QueueCalibrateWorkUnitInputDel, FLensSolverCalibrationPointsWorkUnit)
DECLARE_DELEGATE_OneParam(QueueLatchInputDel, const FCalibrateLatch)

class FLensSolverWorkerCalibrate : public FLensSolverWorker
{
public:

	FLensSolverWorkerCalibrate(
		FLensSolverWorkerParameters & inputParameters,
		QueueCalibrateWorkUnitInputDel* inputQueueCalibrateWorkUnitDel,
		QueueLatchInputDel* inputSignalLatch,
		QueueCalibrationResultOutputDel* inputOnSolvePointsDel);

	~FLensSolverWorkerCalibrate()
	{
		/*/
		queueCalibrateWorkUnitDel->Unbind();
		signalLatch->Unbind();

		queueCalibrateWorkUnitDel = nullptr;
		signalLatch = nullptr;
		/*
		if (queueCalibrateWorkUnitDel != nullptr && queueCalibrateWorkUnitDel->IsBound())
		{
			queueCalibrateWorkUnitDel->Unbind();
			queueCalibrateWorkUnitDel = nullptr;
		}

		if (signalLatch != nullptr && signalLatch->IsBound())
		{
			signalLatch->Unbind();
			signalLatch = nullptr;
		}
		*/
	};

private:
	mutable int workUnitCount;

	QueueCalibrateWorkUnitInputDel* queueCalibrateWorkUnitDel;
	QueueLatchInputDel* signalLatch;
	const QueueCalibrationResultOutputDel * onSolvePointsDel;

	TMap<FString, TQueue<FLensSolverCalibrationPointsWorkUnit>*> workQueue;
	TQueue<FCalibrateLatch, EQueueMode::Mpsc> latchQueue;

	FMatrix GeneratePerspectiveMatrixFromFocalLength(cv::Size& imageSize, cv::Point2d principlePoint, float focalLength);
	FTransform GenerateTransformFromRAndTVecs(std::vector<cv::Mat>& rvecs, std::vector<cv::Mat>& tvecs);
	void WriteSolvedPointsToJSONFile(const FCalibrationResult& solvePoints, FString outputPath);
	void TransformVectorFromCVToUE4(FVector& v);

	void QueueCalibrationResultError(const FBaseParameters & baseParameters);
	void QueueCalibrationResult(FCalibrationResult solvedPoints);

	void QueueLatch(const FCalibrateLatch latchData);
	void DequeueLatch(FCalibrateLatch & latchDataPtr);

	bool DequeueAllWorkUnits(
		const FString calibrationID, 
		TArray<TArray<FVector2D>> & corners,
		TArray<TArray<FVector>> & objectPoints,
		int & cornerCountX, int & cornerCountY);
		// std::vector<std::vector<cv::Point2f>> & corners,
		// std::vector<std::vector<cv::Point3f>> & objectPoints);

	bool LatchInQueue();

	void QueueWorkUnit(FLensSolverCalibrationPointsWorkUnit calibrateWorkUnit);

protected:
	virtual void Tick() override;
	virtual int GetWorkLoad() override;
};