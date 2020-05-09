#pragma once
#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "Engine.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/ScriptInterface.h"

#include "DistortionCorrectionMapGenerationParameters.h"
#include "DistortTextureWithCoefficientsParams.h"
#include "DistortTextureWithTextureFileParams.h"
#include "DistortTextureWithTextureParams.h"

#include "OneTimeProcessParameters.h"
#include "StartMediaStreamParameters.h"
#include "TextureFolderZoomPair.h"

#include "ILensSolverEventReceiver.h"
#include "JobInfo.h"

#include "LensSolverBlueprintAPI.generated.h"

UCLASS(BlueprintType, Category = "Lens Calibrator", meta = (Keywords = ""))
class LENSCALIBRATOR_API ULensSolverBlueprintAPI : public UObject
{
	GENERATED_BODY()
private:
protected:
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool debug;

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	static void OneTimeProcessArrayOfTextureFolderZoomPairs(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		TArray<FTextureFolderZoomPair> inputTextures, 
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	static void StartMediaStreamCalibration(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		FStartMediaStreamParameters mediaStreamParameters,
		FJobInfo& ouptutJobInfo);

	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
	static void GenerateDistortionCorrectionMap(
		FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams);

	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
	static void DistortTextureWithTexture(
		FDistortTextureWithTextureParams distortionCorrectionParams);

	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
	static void DistortTextureWithTextureFile(
		FDistortTextureWithTextureFileParams distortionCorrectionParams);

	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
	static void DistortTextureWithCoefficients(
		FDistortTextureWithCoefficientsParams distortionCorrectionParams);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	static void StartBackgroundImageProcessors(int findCornersWorkerCount, int calibrateWorkerCount);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	static void StopBackgroundImageprocessors();

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	static void Poll ();
};
