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

	TQueue<FDistortionCorrectionMapGenerationResults> queuedDistortionCorrectionMapResults;
	TQueue<FCorrectedDistortedImageResults> queuedCorrectedDistortedImageResults;
	TMap<FString, DistortionJob> cachedEvents;

	void GenerateDistortionCorrectionMapRenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams,
		const FString correctionFilePath,
		const FString inverseCorrectionFilePath);

	void UndistortImageRenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FDistortTextureWithTextureParams distortionCorrectionParams,
		const FString generatedOutputPath);

	void PollDistortionCorrectionMapGenerationResults();
	void PollCorrectedDistortedImageResults();

public:

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