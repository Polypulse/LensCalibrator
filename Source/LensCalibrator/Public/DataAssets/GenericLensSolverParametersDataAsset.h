/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/DataAsset.h"

#include "GenericLensSolverParameters.h"

#include "GenericLensSolverParametersDataAsset.generated.h"

/* Since you can perform lens calibration via a media stream or via folders of images, each of those approaches requires a different
set of parameters for configuration. Therefore, this is the base data asset that abstracts across those two calibration modes and
provides a container for a common set of parameters used across those modes. However, implementations do not derive from this 
class, instead we use the composition pattern by holding an instance of this class inside the data asset that actually performs 
the implementation. */
UCLASS(BlueprintType)
class UGenericLensSolverParametersDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FGenericLensSolverParameters genericLensSolverParameters;
};
