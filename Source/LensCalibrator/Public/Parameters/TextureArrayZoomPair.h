/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "TextureArrayZoomPair.generated.h"

USTRUCT(BlueprintType)
struct FTextureArrayZoomPair
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<UTexture2D*> textures;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float zoomLevel;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool use;

	FTextureArrayZoomPair()
	{
		zoomLevel = 0.0f;
		use = true;
	}
};