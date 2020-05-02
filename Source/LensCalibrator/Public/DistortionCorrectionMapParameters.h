#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "SolvedPoints.h"

#include "DistortionCorrectionMapParameters.generated.h"

USTRUCT(BlueprintType)
struct FDistortionCorrectionMapParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FCalibrationResult calibrationResult;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FIntPoint mapResolution;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString outputPath;
};