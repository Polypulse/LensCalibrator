/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once
#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "Engine.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/ScriptInterface.h"

#include "Engine/VolumeTexture.h"

#include "DistortionCorrectionMapGenerationParameters.h"
#include "DistortTextureWithCoefficientsParams.h"
#include "DistortTextureWithTextureFileParams.h"
#include "DistortTextureWithTextureParams.h"
#include "CompositingMaterialPass.h"

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

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	static void OneTimeProcessArrayOfTextureFolderZoomPairs(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		TArray<FTextureFolderZoomPair> inputTextures, 
		FTextureSearchParameters textureSearchParameters,
		FCalibrationParameters calibrationParameters,
		FJobInfo & ouptutJobInfo);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	static void StartMediaStreamCalibration(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		FTextureSearchParameters textureSearchParameters,
		FCalibrationParameters calibrationParameters,
		FMediaStreamParameters mediaStreamParameters,
		FJobInfo& ouptutJobInfo);

	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
	static void GenerateDistortionCorrectionMap(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams);

	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
	static void DistortTextureWithTexture(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		FDistortTextureWithTextureParams distortionCorrectionParams);

	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
	static void DistortTextureWithTextureFile(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		FDistortTextureWithTextureFileParams distortionCorrectionParams);

	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
	static void DistortTextureWithCoefficients(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		FDistortTextureWithCoefficientsParams distortionCorrectionParams);

	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
	static bool PackArrayOfDistortionCorrectionMapsIntoVolumeTexture(
		TArray<UTexture2D*> distortionCorrectionMaps,
		UVolumeTexture * volumeTexture);

	UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
	static void OverrideCompositingMaterialScalarParam(
		FCompositingMaterial inputCompositingMaterial,
		const FName paramName, 
		float scalarValue,
		FCompositingMaterial & outputCompositingMaterial);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	static void StartBackgroundImageProcessors(
		int findCornersWorkerCount,
		int calibrateWorkerCount,
		bool shutDownWorkersAfterCompletingTasks = true);

	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	static void StopBackgroundImageprocessors();

	/*
	UFUNCTION(BlueprintCallable, Category="Lens Calibrator")
	static void Poll ();
	*/
};
