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
#include "Engine/DataAsset.h"

#include "GenericLensSolverParameters.h"

#include "GenericLensSolverParametersDataAsset.generated.h"

/* Since you can perform lens calibration via a media stream or via folders of images, each of those approaches requires a different
set of parameters for configuration. Therefore, this is the base data asset that abstracts across those two calibration modes and
provides a container for a common set of parameters used across those modes. However, implementations do not derive from this 
class, instead we use the composition pattern by holding an instance of this class inside the data asset that actually performs 
the implementation. */
UCLASS(BlueprintType)
class UGenericLensSolverParametersDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FGenericLensSolverParameters genericLensSolverParameters;
};
