/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */


#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/DataAsset.h"

#include "TextureFolderParametersDataAsset.h"
#include "GenericLensSolverParametersDataAsset.h"
#include "CalibrationParametersDataAsset.h"
#include "TextureSearchParametersDataAsset.h"
#include "CalibrationResultsDataAsset.h"

#include "TextureFolderZoomPairDataAsset.generated.h"

UCLASS(BlueprintType)
class UTextureFolderZoomPairDataAssetWrapper : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UTextureFolderZoomPairDataAsset * textureFolderZoomPairDataAsset;
};

UCLASS(BlueprintType)
class UTextureFolderZoomPairDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UTextureFolderParametersDataAsset * textureFolderParametersDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UGenericLensSolverParametersDataAsset * genericLensSolverParametersDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UTextureSearchParametersDataAsset * textureSearchParametersDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UCalibrationParametersDataAsset * calibrationParameterDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UCalibrationResultsDataAsset * calibrationResultsDataAsset;
};
