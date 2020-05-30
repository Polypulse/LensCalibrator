/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/DataAsset.h"

#include "LensSolverWorkerParameters.h"

#include "TextureSearchParametersDataAsset.generated.h"

UCLASS(BlueprintType)
class UTextureSearchParametersDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FTextureSearchParameters textureSearchParameters;
};
