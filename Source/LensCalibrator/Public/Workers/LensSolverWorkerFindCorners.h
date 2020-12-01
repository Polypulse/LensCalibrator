/*
 * Copyright (C) 2020 - LensCalibrator contributors, see Contributors.txt at the root of the project.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
