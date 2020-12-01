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

#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "Engine.h"

#include "LensSolverWorker.h"
#include "LensSolverWorkerCalibrate.h"
#include "LensSolverWorkerFindCorners.h"
#include "LensSolverWorkUnit.h"

struct FWorkerInterfaceContainer
{
	FString workerID;

	GetWorkLoadOutputDel getWorkLoadDel;
	IsClosingOutputDel isClosingDel;
};

struct FWorkerFindCornersInterfaceContainer
{
	FAutoDeleteAsyncTask<FLensSolverWorkerFindCorners> * worker;
	FWorkerInterfaceContainer baseContainer;

	QueuePixelArrayWorkUnitInputDel queuePixelArrayWorkUnitInputDel;
	QueueTextureFileWorkUnitInputDel queueTextureFileWorkUnitInputDel;
};

struct FWorkerCalibrateInterfaceContainer
{
	FAutoDeleteAsyncTask<FLensSolverWorkerCalibrate> * worker;
	FWorkerInterfaceContainer baseContainer;

	QueueLatchInputDel signalLatch;
	QueueCalibrateWorkUnitInputDel queueCalibrateWorkUnitDel;
};
