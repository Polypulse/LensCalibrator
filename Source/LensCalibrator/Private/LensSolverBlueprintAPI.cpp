#include "LensSolverBlueprintAPI.h"
#include "LensCalibrator.h"
#include "LensSolver.h"

void ULensSolverBlueprintAPI::OneTimeProcessArrayOfTextureFolderZoomPairs(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	TArray<FTextureFolderZoomPair> inputTextures,
	FOneTimeProcessParameters oneTimeProcessParameters,
	FJobInfo& ouptutJobInfo)
{
	TSharedPtr<ULensSolver> lensSolver = FLensCalibratorModule::Get().GetLensSolver();
	lensSolver->OneTimeProcessArrayOfTextureFolderZoomPairs(
		eventReceiver,
		inputTextures,
		oneTimeProcessParameters,
		ouptutJobInfo
	);
}

void ULensSolverBlueprintAPI::StartMediaStreamCalibration(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FStartMediaStreamParameters mediaStreamParameters, 
	FJobInfo& ouptutJobInfo)
{
	TSharedPtr<ULensSolver> lensSolver = FLensCalibratorModule::Get().GetLensSolver();
	lensSolver->StartMediaStreamCalibration(
		eventReceiver,
		mediaStreamParameters,
		ouptutJobInfo);
}

void ULensSolverBlueprintAPI::GenerateDistortionCorrectionMap(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams)
{
	TSharedPtr<UDistortionProcessor> distortionProcessor = FLensCalibratorModule::Get().GetDistortionProcessor();
	distortionProcessor->GenerateDistortionCorrectionMap(
		eventReceiver,
		distortionCorrectionMapGenerationParams);
}

void ULensSolverBlueprintAPI::DistortTextureWithTexture(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FDistortTextureWithTextureParams distortionCorrectionParams)
{
	TSharedPtr<UDistortionProcessor> distortionProcessor = FLensCalibratorModule::Get().GetDistortionProcessor();
	distortionProcessor->DistortTextureWithTexture(
		eventReceiver,
		distortionCorrectionParams);
}

void ULensSolverBlueprintAPI::DistortTextureWithTextureFile(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FDistortTextureWithTextureFileParams distortionCorrectionParams)
{
	TSharedPtr<UDistortionProcessor> distortionProcessor = FLensCalibratorModule::Get().GetDistortionProcessor();
	distortionProcessor->DistortTextureWithTextureFile(
		eventReceiver,
		distortionCorrectionParams);
}

void ULensSolverBlueprintAPI::DistortTextureWithCoefficients(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FDistortTextureWithCoefficientsParams distortionCorrectionParams)
{
	TSharedPtr<UDistortionProcessor> distortionProcessor = FLensCalibratorModule::Get().GetDistortionProcessor();
	distortionProcessor->DistortTextureWithCoefficients(
		eventReceiver,
		distortionCorrectionParams);
}

void ULensSolverBlueprintAPI::StartBackgroundImageProcessors(
	int findCornersWorkerCount,
	int calibrateWorkerCount)
{
	TSharedPtr<ULensSolver> lensSolver = FLensCalibratorModule::Get().GetLensSolver();
	lensSolver->StartBackgroundImageProcessors(
		findCornersWorkerCount,
		calibrateWorkerCount);
}

void ULensSolverBlueprintAPI::StopBackgroundImageprocessors()
{
	TSharedPtr<ULensSolver> lensSolver = FLensCalibratorModule::Get().GetLensSolver();
	lensSolver->StopBackgroundImageprocessors();
}

/*
void ULensSolverBlueprintAPI::Poll()
{
	TSharedPtr<ULensSolver> lensSolver = FLensCalibratorModule::Get().GetLensSolver();
	lensSolver->Poll();
}
*/
