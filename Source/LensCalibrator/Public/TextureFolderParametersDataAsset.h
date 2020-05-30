/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/DataAsset.h"

#include "TextureFolderZoomPair.h"

#include "TextureFolderParametersDataAsset.generated.h"

UCLASS(BlueprintType)
class UTextureFolderParametersDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<FTextureFolderZoomPair> inputs;
};
