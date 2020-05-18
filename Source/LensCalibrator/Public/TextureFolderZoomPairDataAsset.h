/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/DataAsset.h"

#include "GenericLensSolverParameters.h"
#include "OneTimeProcessParameters.h"
#include "TextureFolderZoomPair.h"

#include "TextureFolderZoomPairDataAsset.generated.h"

UCLASS(BlueprintType)
class UTextureFolderZoomPairDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FGenericLensSolverParameters genericLensSolverParameters;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FOneTimeProcessParameters oneTimeProcessParameters;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<FTextureFolderZoomPair> inputs;
};
