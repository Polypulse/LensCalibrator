/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/DataAsset.h"

#include "SolvedPoints.h"

#include "CalibrationResultsDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FDistortionCorrectionTextureContainer
{
	GENERATED_BODY()
	public:
		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
		UTexture2D* distortionMap;

		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
		float zoomLevel;

		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
		float distortionMultiplier;

		FDistortionCorrectionTextureContainer()
		{
			distortionMap = nullptr;
			zoomLevel = 0.0f;
			distortionMultiplier = 0.0f;
		}
};

UCLASS(BlueprintType)
class UCalibrationResultsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float currentZoomLevel;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool inverseDistortionCorrection;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UVolumeTexture* lensDistortionVolume;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<FCalibrationResult> calibrationResults;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<float> focalLengths;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<FDistortionCorrectionTextureContainer> distortionCorrectionMaps;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<FDistortionCorrectionTextureContainer> distortionUncorrectionMaps;
};
