/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/DataAsset.h"

#include "GenericLensSolverParameters.h"

#include "GenericLensSolverParametersDataAsset.generated.h"

/* Since you can perform lens calibration via a media stream or
via folders of images, each of those approaches requires a different
set of parameters for configuration. Therefore, this is the base
data asset that abstracts across those two calibration modes. */
UCLASS(BlueprintType)
class UGenericLensSolverParametersDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FGenericLensSolverParameters genericLensSolverParameters;
};
