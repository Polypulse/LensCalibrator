/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "TextureFolderZoomPair.generated.h"

/* This struct provides a blueprint compatible container for associating
a folder path associated with a zoom level where what folder contains
a set of images. */
USTRUCT(BlueprintType)
struct FTextureFolderZoomPair
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString absoluteFolderPath;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float zoomLevel;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool use;

	FTextureFolderZoomPair()
	{
		zoomLevel = 0.0f;
		use = true;
	}
};
