#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "UObject/Interface.h"
#include "Engine.h"

#include "ILensSolverEventReceiver.generated.h"

UINTERFACE(BlueprintType)
class LENSCALIBRATOR_API ULensSolverEventReceiver : public UInterface
{
	GENERATED_BODY()
};

class LENSCALIBRATOR_API ILensSolverEventReceiver
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintImplementableEvent, Category="Lens Calibrator")
	void OnReceiveCalibrationResult (FCalibrationResult calibrationResult);

	UFUNCTION(BlueprintImplementableEvent, Category="Lens Calibrator")
	void OnFinishedJob (FJobInfo jobInfo);

	UFUNCTION(BlueprintImplementableEvent, Category="Lens Calibrator")
	void OnGeneratedDistortionMap (UTexture2D * generatedDistortionMap);

	UFUNCTION(BlueprintImplementableEvent, Category="Lens Calibrator")
	void OnDistortedImageCorrected (UTexture2D * correctedDistortedImage);
};