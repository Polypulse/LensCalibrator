/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "CalibrationWorkerParameters.h"
#include "FindCornerWorkerParameters.h"
#include "OneTimeProcessParameters.generated.h"

USTRUCT(BlueprintType)
struct FOneTimeProcessParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FFindCornerWorkerParameters findCornerWorkerParameters;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FCalibrationWorkerParameters calibrationWorkerParameters;

	FOneTimeProcessParameters()
	{
		// currentResolution = FIntPoint(0, 0);
	}
};
