/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "MediaBundle.h"
#include "Engine/DataAsset.h"

#include "MediaStreamParametersDataAsset.generated.h"

UENUM(BlueprintType)
enum class UInputMediaType : uint8
{
	MediaPlayer UMETA(DisplayName = "Media Player"),
	MediaBundle UMETA(DisplayName = "Media Bundle")
};

UCLASS(BlueprintType)
class UMediaStreamParametersDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FMediaStreamParameters mediaStreamParameters;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UInputMediaType inputMediaType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UMediaPlayer * mediaPlayer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString url;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UMediaBundle * mediaBundle;
};
