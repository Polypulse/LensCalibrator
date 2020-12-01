/*
 * Copyright (C) 2020 - LensCalibrator contributors, see Contributors.txt at the root of the project.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
		bool invertDistortion;

		FDistortionCorrectionTextureContainer()
		{
			distortionMap = nullptr;
			zoomLevel = 0.0f;
			distortionMultiplier = 0.0f;
			invertDistortion = false;
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
