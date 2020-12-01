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

#include "DistortionCorrectionMapGenerationParameters.h"
#include "DistortionCorrectionMapGenerationResults.h"
#include "DistortTextureWithCoefficientsParams.h"
#include "DistortTextureWithTextureFileParams.h"
#include "DistortTextureWithTextureParams.h"
#include "CorrectedDistortedImageResults.h"

#include "DistortionJob.h"
#include "ILensSolverEventReceiver.h"

#include "DistortionProcessor.generated.h"

UCLASS()
class LENSCALIBRATOR_API UDistortionProcessor : public UObject
{
	GENERATED_BODY()
private:

	TQueue<FDistortionCorrectionMapGenerationResults, EQueueMode::Mpsc> queuedDistortionCorrectionMapResults;
	TQueue<FCorrectedDistortedImageResults, EQueueMode::Mpsc> queuedCorrectedDistortedImageResults;
	TMap<FString, DistortionJob> cachedEvents;

	void GenerateDistortionCorrectionMapRenderThread(
		FRHICommandListImmediate& RHICmdList,
		FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams,
		const FString correctionFilePath,
		const FString inverseCorrectionFilePath);

	void UndistortImageRenderThread(
		FRHICommandListImmediate& RHICmdList,
		FDistortTextureWithTextureParams distortionCorrectionParams,
		const FString generatedOutputPath);

	void PollDistortionCorrectionMapGenerationResults();
	void PollCorrectedDistortedImageResults();

public:

	UDistortionProcessor() {}
	~UDistortionProcessor() {}

	void GenerateDistortionCorrectionMap(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams);

	void DistortTextureWithTexture(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		FDistortTextureWithTextureParams distortionCorrectionParams);

	void DistortTextureWithTextureFile(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		FDistortTextureWithTextureFileParams distortionCorrectionParams);

	void DistortTextureWithCoefficients(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		FDistortTextureWithCoefficientsParams distortionCorrectionParams);

	void Poll();
};