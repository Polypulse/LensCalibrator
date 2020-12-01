/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/DataAsset.h"

#include "MediaStreamParametersDataAsset.h"
#include "GenericLensSolverParametersDataAsset.h"
#include "CalibrationParametersDataAsset.h"
#include "TextureSearchParametersDataAsset.h"
#include "CalibrationResultsDataAsset.h"

#include "MediaStreamDataAsset.generated.h"

UCLASS(BlueprintType)
class UMediaStreamDataAssetWrapper : public UDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UMediaStreamDataAsset * mediaStreamDataAsset;
};

UCLASS(BlueprintType)
class UMediaStreamDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UMediaStreamParametersDataAsset * mediaStreamParametersDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UGenericLensSolverParametersDataAsset * genericLensSolverParametersDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UTextureSearchParametersDataAsset * textureSearchParametersDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UCalibrationParametersDataAsset * calibrationParameterDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UCalibrationResultsDataAsset * calibrationResultsDataAsset;
};
