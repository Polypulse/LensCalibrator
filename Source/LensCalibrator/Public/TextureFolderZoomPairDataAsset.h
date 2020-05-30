/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/DataAsset.h"

#include "GenericLensSolverParametersDataAsset.h"
#include "CalibrationParametersDataAsset.h"
#include "TextureSearchParametersDataAsset.h"

#include "TextureFolderZoomPairDataAsset.generated.h"

UCLASS(BlueprintType)
class UTextureFolderZoomPairDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UGenericLensSolverParametersDataAsset * genericLensSolverParametersDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UTextureSearchParametersDataAsset * textureSearchParametersDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UCalibrationParametersDataAsset * calibrationParameterDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<FTextureFolderZoomPair> inputs;
};
