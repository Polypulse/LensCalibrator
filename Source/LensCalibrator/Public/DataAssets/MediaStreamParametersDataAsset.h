/*
 * Copyright (C) 2020 - LensCalibrator contributors, see Contributors.txt at the root of the project.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
