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
	};

private:
	mutable int workUnitCount;

	QueueCalibrateWorkUnitInputDel* queueCalibrateWorkUnitDel;
	QueueLatchInputDel* signalLatch;
	const QueueCalibrationResultOutputDel * onSolvePointsDel;

	TMap<FString, TQueue<FLensSolverCalibrationPointsWorkUnit>*> workQueue;
	TQueue<FCalibrateLatch, EQueueMode::Mpsc> latchQueue;

	FMatrix GeneratePerspectiveMatrixFromFocalLength(const FIntPoint& imageSize, const FVector2D& principlePoint, const float focalLength);
	void WriteSolvedPointsToJSONFile(const FCalibrationResult& solvePoints, FString outputPath);

	void QueueCalibrationResultError(const FBaseParameters & baseParameters);
	void QueueCalibrationResult(FCalibrationResult solvedPoints);

	void QueueLatch(const FCalibrateLatch latchData);
	void DequeueLatch(FCalibrateLatch & latchDataPtr);

	bool DequeueAllWorkUnits(
		const FString calibrationID, 
		TArray<float> & corners,
		int & cornerCountX, int & cornerCountY,
		float & chessboardSquareSizeMM,
		int & imageCount);

	bool LatchInQueue();

	void QueueWorkUnit(FLensSolverCalibrationPointsWorkUnit calibrateWorkUnit);

protected:
	virtual void Tick() override;
	virtual int GetWorkLoad() override;
	virtual void NotifyShutdown () override;
};