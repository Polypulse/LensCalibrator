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
	float zoomLevel;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool inverseDistortionCorrection;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int cornerWorkerCount;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int calibrateWorkerCount;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UVolumeTexture* lensDistortionVolume;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<FCalibrationResult> calibrationResults;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<float> focalLengths;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<UTexture2D*> distortionCorrectionMaps;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<UTexture2D*> distortionUncorrectionMaps;

	FGenericLensSolverParameters()
	{
		zoomLevel = 0.0f;
		cornerWorkerCount = 3;
		calibrateWorkerCount = 1;
	}
};