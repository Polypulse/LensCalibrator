/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "WorkerParameters.generated.h"

USTRUCT(BlueprintType)
struct FWorkerParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	bool exhaustiveSearch;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	bool writeDebugTextureToFile;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	FString debugTextureFolderPath;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	bool writeCalibrationResultsToFile;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	FString calibrationResultsFolderPath;
};

