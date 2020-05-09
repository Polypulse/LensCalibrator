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
	FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams)
{
	TSharedPtr<ULensSolver> lensSolver = FLensCalibratorModule::Get().GetLensSolver();
	lensSolver->GenerateDistortionCorrectionMap(
		distortionCorrectionMapGenerationParams);
}

void ULensSolverBlueprintAPI::DistortTextureWithTexture(
	FDistortTextureWithTextureParams distortionCorrectionParams)
{
	TSharedPtr<ULensSolver> lensSolver = FLensCalibratorModule::Get().GetLensSolver();
	lensSolver->DistortTextureWithTexture(
		distortionCorrectionParams);
}

void ULensSolverBlueprintAPI::DistortTextureWithTextureFile(
	FDistortTextureWithTextureFileParams distortionCorrectionParams)
{
	TSharedPtr<ULensSolver> lensSolver = FLensCalibratorModule::Get().GetLensSolver();
	lensSolver->DistortTextureWithTextureFile(
		distortionCorrectionParams);
}

void ULensSolverBlueprintAPI::DistortTextureWithCoefficients(
	FDistortTextureWithCoefficientsParams distortionCorrectionParams)
{
	TSharedPtr<ULensSolver> lensSolver = FLensCalibratorModule::Get().GetLensSolver();
	lensSolver->DistortTextureWithCoefficients(
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

void ULensSolverBlueprintAPI::Poll()
{
	TSharedPtr<ULensSolver> lensSolver = FLensCalibratorModule::Get().GetLensSolver();
	lensSolver->Poll();
}
