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
#include "ILensSolverEventReceiver.h"

/* Distortion map generation job data. */
struct DistortionJob
{
	/* The blueprint class that implements ILensSolverEventReceiver, the reference is stored to 
	this class in order to call methods throughout distortion map generation. */
	TScriptInterface<ILensSolverEventReceiver> eventReceiver;

	/* Job ID. */
	FString id;
};