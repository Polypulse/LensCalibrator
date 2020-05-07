/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "CalibrationWorkerParameters.h"
#include "FindCornerWorkerParameters.h"
#include "LensSolverWorkerParameters.h"
#include "OneTimeProcessParameters.generated.h"

USTRUCT(BlueprintType)
struct FOneTimeProcessParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FTextureSearchParameters textureSearchParameters;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FCalibrationParameters calibrationParameters;

	FOneTimeProcessParameters()
	{
		// currentResolution = FIntPoint(0, 0);
	}
};
