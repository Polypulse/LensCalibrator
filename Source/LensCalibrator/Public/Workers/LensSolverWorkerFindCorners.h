/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "LensSolverWorker.h"

DECLARE_DELEGATE_OneParam(QueueTextureFileWorkUnitInputDel, FLensSolverTextureFileWorkUnit)
DECLARE_DELEGATE_OneParam(QueuePixelArrayWorkUnitInputDel, FLensSolverPixelArrayWorkUnit)
DECLARE_DELEGATE_OneParam(QueueFindCornerResultOutputDel, FLensSolverCalibrationPointsWorkUnit)

/* The purpose of this worker is to provide an parallel asynchronous method of solving 
for the corners in a calibration pattern. */
class FLensSolverWorkerFindCorners : public FLensSolverWorker
{
public:
	/* Constructor that takes in the worker parameters and a set of delegates for callbacks. */
	FLensSolverWorkerFindCorners(
		FLensSolverWorkerParameters & inputParameters,
		QueueTextureFileWorkUnitInputDel* inputQueueTextureFileWorkUnitInputDel,
		QueuePixelArrayWorkUnitInputDel* inputQueuePixelArrayWorkUnitInputDel,
		QueueFindCornerResultOutputDel* inputQueueFindCornerResultOutputDel);

	~FLensSolverWorkerFindCorners() {};

private:
	mutable int workUnitCount;

	TQueue<FLensSolverPixelArrayWorkUnit, EQueueMode::Mpsc> pixelArrayWorkQueue;
	TQueue<FLensSolverTextureFileWorkUnit, EQueueMode::Mpsc> textureFileWorkQueue;

	FResizeParameters CalculateResizeParameters (const FChessboardSearchParameters & textureSearchParameters);

	QueueTextureFileWorkUnitInputDel* queueTextureFileWorkUnitInputDel;
	QueuePixelArrayWorkUnitInputDel* queuePixelArrayWorkUnitInputDel;
	const QueueFindCornerResultOutputDel* queueFindCornerResultOutputDel;

	void DequeueTextureFileWorkUnit(FLensSolverTextureFileWorkUnit& workUnit);
	void DequeuePixelArrayWorkUnit(FLensSolverPixelArrayWorkUnit & workUnit);
	void QueueTextureFileWorkUnit(FLensSolverTextureFileWorkUnit workUnit);
	void QueuePixelArrayWorkUnit(FLensSolverPixelArrayWorkUnit workUnit);

	void QueueCalibrationPointsWorkUnit(const FLensSolverCalibrationPointsWorkUnit & calibrationPointsWorkUnit);
	void QueueEmptyCalibrationPointsWorkUnit(const FBaseParameters & baseParameters, const FResizeParameters & resizeParameters);

protected:

	virtual void Tick() override;
	virtual int GetWorkLoad() override;
	virtual void NotifyShutdown () override;
};
