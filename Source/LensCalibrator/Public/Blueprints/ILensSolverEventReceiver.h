/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "UObject/Interface.h"
#include "Engine.h"
#include "JobInfo.h"
#include "SolvedPoints.h"
#include "CalibrationResultsDataAsset.h"

#include "ILensSolverEventReceiver.generated.h"


UINTERFACE(BlueprintType)
class LENSCALIBRATOR_API ULensSolverEventReceiver : public UInterface
{
	GENERATED_BODY()
};

/* This interface provides a set of methods that a blueprint class can derive from
to receive events throughout lens calibration. */
class LENSCALIBRATOR_API ILensSolverEventReceiver
{
	GENERATED_BODY()
public:

	/* When calibration finishes, this implemented method in the class that derives
	from this interface will be called. This can be potentially called multiple times if 
	calibrations for multiple zoom levels is performed. */
	UFUNCTION(BlueprintImplementableEvent, Category="Lens Calibrator")
	void OnReceiveCalibrationResult (FCalibrationResult calibrationResult);

	/* After all calibration jobs are finished, this method is called. */
	UFUNCTION(BlueprintImplementableEvent, Category="Lens Calibrator")
	void OnFinishedJob (FJobInfo jobInfo);

	/* Each time a distortion map is generated, this method is called. */
	UFUNCTION(BlueprintImplementableEvent, Category="Lens Calibrator")
	void OnGeneratedDistortionMaps (FDistortionCorrectionTextureContainer generatedCorrectionDistortionMap, FDistortionCorrectionTextureContainer generatedUnCorrectionDistortionMap);

	UFUNCTION(BlueprintImplementableEvent, Category="Lens Calibrator")
	void OnDistortedImageCorrected (UTexture2D * correctedDistortedImage);
};