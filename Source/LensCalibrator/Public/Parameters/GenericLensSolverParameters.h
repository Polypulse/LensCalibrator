/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/VolumeTexture.h"
#include "SolvedPoints.h"

#include "GenericLensSolverParameters.generated.h"

USTRUCT(BlueprintType)
struct FGenericLensSolverParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int cornerWorkerCount;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int calibrateWorkerCount;

	FGenericLensSolverParameters()
	{
		cornerWorkerCount = 3;
		calibrateWorkerCount = 1;
	}
};