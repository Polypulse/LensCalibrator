/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "LensSolverWorker.h"

DECLARE_DELEGATE_OneParam(QueueTextureFileWorkUnitInputDel, FLensSolverTextureFileWorkUnit)
DECLARE_DELEGATE_OneParam(QueuePixelArrayWorkUnitInputDel, FLensSolverPixelArrayWorkUnit)
DECLARE_DELEGATE_OneParam(QueueFindCornerResultOutputDel, FLensSolverCalibrationPointsWorkUnit)

class FLensSolverWorkerFindCorners : public FLensSolverWorker
{
public:
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
