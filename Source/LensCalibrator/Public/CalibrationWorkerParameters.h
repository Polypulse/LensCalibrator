#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "CalibrationWorkerParameters.generated.h"

USTRUCT(BlueprintType)
struct FCalibrationWorkerParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float sensorDiagonalSizeMM;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FIntPoint initialPrincipalPointPixelPosition;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool useInitialIntrinsicValues;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool keepPrincipalPixelPositionFixed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool keepAspectRatioFixed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool lensHasTangentalDistortion;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool fixRadialDistortionCoefficientK1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool fixRadialDistortionCoefficientK2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool fixRadialDistortionCoefficientK3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool fixRadialDistortionCoefficientK4;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool fixRadialDistortionCoefficientK5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool fixRadialDistortionCoefficientK6;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool writeCalibrationResultsToFile;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString calibrationResultsFolderPath;

	FCalibrationWorkerParameters()
	{
		sensorDiagonalSizeMM = 9.960784f;
		initialPrincipalPointPixelPosition = FIntPoint(0, 0);

		useInitialIntrinsicValues = false;
		keepPrincipalPixelPositionFixed = false;
		keepAspectRatioFixed = true;
		lensHasTangentalDistortion = false;
		fixRadialDistortionCoefficientK1 = false;
		fixRadialDistortionCoefficientK2 = false;
		fixRadialDistortionCoefficientK3 = false;
		fixRadialDistortionCoefficientK4 = false;
		fixRadialDistortionCoefficientK5 = false;
		fixRadialDistortionCoefficientK6 = false;

		writeCalibrationResultsToFile = false;
		calibrationResultsFolderPath = "";
	}
};