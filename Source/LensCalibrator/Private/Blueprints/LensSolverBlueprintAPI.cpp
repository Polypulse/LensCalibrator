/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensSolverBlueprintAPI.h"
#include "LensCalibrator.h"
#include "LensSolver.h"

void ULensSolverBlueprintAPI::OneTimeProcessArrayOfTextureFolderZoomPairs(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	TArray<FTextureFolderZoomPair> inputTextures,
	FTextureSearchParameters textureSearchParameters,
	FCalibrationParameters calibrationParameters,
	FJobInfo& ouptutJobInfo)
{
	ULensSolver* lensSolver = FLensCalibratorModule::Get().GetLensSolver();
	lensSolver->OneTimeProcessArrayOfTextureFolderZoomPairs(
		eventReceiver,
		inputTextures,
		textureSearchParameters,
		calibrationParameters,
		ouptutJobInfo
	);
}

void ULensSolverBlueprintAPI::StartMediaStreamCalibration(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FTextureSearchParameters textureSearchParameters,
	FCalibrationParameters calibrationParameters,
	FMediaStreamParameters mediaStreamParameters,
	FJobInfo& ouptutJobInfo)
{
	ULensSolver* lensSolver = FLensCalibratorModule::Get().GetLensSolver();
	lensSolver->StartMediaStreamCalibration(
		eventReceiver,
		textureSearchParameters,
		calibrationParameters,
		mediaStreamParameters,
		ouptutJobInfo);
}

void ULensSolverBlueprintAPI::GenerateDistortionCorrectionMap(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams)
{
	UDistortionProcessor* distortionProcessor = FLensCalibratorModule::Get().GetDistortionProcessor();
	distortionProcessor->GenerateDistortionCorrectionMap(
		eventReceiver,
		distortionCorrectionMapGenerationParams);
}

void ULensSolverBlueprintAPI::DistortTextureWithTexture(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FDistortTextureWithTextureParams distortionCorrectionParams)
{
	UDistortionProcessor* distortionProcessor = FLensCalibratorModule::Get().GetDistortionProcessor();
	distortionProcessor->DistortTextureWithTexture(
		eventReceiver,
		distortionCorrectionParams);
}

void ULensSolverBlueprintAPI::DistortTextureWithTextureFile(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FDistortTextureWithTextureFileParams distortionCorrectionParams)
{
	UDistortionProcessor* distortionProcessor = FLensCalibratorModule::Get().GetDistortionProcessor();
	distortionProcessor->DistortTextureWithTextureFile(
		eventReceiver,
		distortionCorrectionParams);
}

void ULensSolverBlueprintAPI::DistortTextureWithCoefficients(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FDistortTextureWithCoefficientsParams distortionCorrectionParams)
{
	UDistortionProcessor* distortionProcessor = FLensCalibratorModule::Get().GetDistortionProcessor();
	distortionProcessor->DistortTextureWithCoefficients(
		eventReceiver,
		distortionCorrectionParams);
}

bool ULensSolverBlueprintAPI::PackArrayOfDistortionCorrectionMapsIntoVolumeTexture(
		TArray<UTexture2D*> distortionCorrectionMaps,
		UVolumeTexture * volumeTexture)
{
	if (distortionCorrectionMaps.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Input array of distortion correction maps is empty."));
		return false;
	}

	if (volumeTexture == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Input VolumeTexture is NULL."));
		return false;
	}

	if (distortionCorrectionMaps[0] == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("The first entry of the distortion correction map array is NULL."));
		return false;
	}

	int width = distortionCorrectionMaps[0]->GetSizeX();
	int height = distortionCorrectionMaps[0]->GetSizeY();

	for (int i = 0; i < distortionCorrectionMaps.Num(); i++)
	{
		if (distortionCorrectionMaps[i] == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("The texture at index: %d within the input array is NULL."));
			return false;
		}

		if (distortionCorrectionMaps[i]->GetSizeX() != width || distortionCorrectionMaps[i]->GetSizeY() != height)
		{
			UE_LOG(LogTemp, Error, TEXT("All textures in the input array should have the same resolution, the texture at index: %d has a resolution of: (%d, %d). The expected resolution is: (%d, %d)."),
				i,
				distortionCorrectionMaps[i]->GetSizeX(),
				distortionCorrectionMaps[i]->GetSizeY(),
				width,
				height);
			return false;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Attempting to pack: %d textures of size: (%d, %d) into volume texture."), distortionCorrectionMaps.Num(), width, height);

	TArray<FFloat16Color*> dataArray;
	dataArray.SetNum(distortionCorrectionMaps.Num());
	for (int i = 0; i < distortionCorrectionMaps.Num(); i++)
		dataArray[i] = reinterpret_cast<FFloat16Color*>(distortionCorrectionMaps[i]->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_ONLY));

	bool success = volumeTexture->UpdateSourceFromFunction([width, height, dataArray](int ix, int iy, int iz, void* value)
	{
		FFloat16* const voxel = static_cast<FFloat16*>(value);
		const FFloat16Color * data = dataArray[iz];

		voxel[0] = data[iy * width + ix].R;
		voxel[1] = data[iy * width + ix].G;
		voxel[2] = FFloat16(0.0f);
		voxel[3] = FFloat16(0.0f);

	}, width, height, distortionCorrectionMaps.Num(), TSF_RGBA16F);

	for (int i = 0; i < distortionCorrectionMaps.Num(); i++)
		distortionCorrectionMaps[i]->PlatformData->Mips[0].BulkData.Unlock();

	if (!success)
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to update VolumeTexture source."));
		return false;
	}

	volumeTexture->MarkPackageDirty();
	UE_LOG(LogTemp, Log, TEXT("Successfully packed volume texture with: %d textures."), distortionCorrectionMaps.Num());

	return true;
}

void ULensSolverBlueprintAPI::OverrideCompositingMaterialScalarParam(
	FCompositingMaterial inputCompositingMaterial,
	const FName paramName,
	float scalarValue,
	FCompositingMaterial & outputCompositingMaterial)
{
	inputCompositingMaterial.SetScalarOverride(paramName, scalarValue);
	outputCompositingMaterial = inputCompositingMaterial;
}

UFUNCTION(BlueprintCallable, Category = "Lens Calibrator")
void ULensSolverBlueprintAPI::OverrideArrayOfCompositingMaterialScalarParam(
	FCompositingMaterial inputCompositingMaterial,
	const TArray<FName> paramNames,
	TArray<float> scalarValues,
	FCompositingMaterial& outputCompositingMaterial)
{
	if (paramNames.Num() != scalarValues.Num())
		UE_LOG(LogTemp, Error, TEXT("The param name and scalar value array count does not match when attempting to override parameters in a compositing material."));

	for (int i = 0; i < paramNames.Num(); i++)
		inputCompositingMaterial.SetScalarOverride(paramNames[i], scalarValues[i]);

	outputCompositingMaterial = inputCompositingMaterial;
}

void ULensSolverBlueprintAPI::StartBackgroundImageProcessors(
	int findCornersWorkerCount,
	int calibrateWorkerCount,
	bool shutDownWorkersAfterCompletingTasks)
{
	ULensSolver* lensSolver = FLensCalibratorModule::Get().GetLensSolver();
	lensSolver->StartBackgroundImageProcessors(
		findCornersWorkerCount,
		calibrateWorkerCount,
		shutDownWorkersAfterCompletingTasks);
}

void ULensSolverBlueprintAPI::StopBackgroundImageprocessors()
{
	ULensSolver* lensSolver = FLensCalibratorModule::Get().GetLensSolver();
	lensSolver->StopBackgroundImageprocessors();
}
