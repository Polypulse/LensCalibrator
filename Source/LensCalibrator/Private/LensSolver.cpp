/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensSolver.h"

#include <Runtime\Engine\Classes\Engine\Texture.h>
#include <Runtime\Engine\Classes\Engine\Texture2D.h>

#include "TextureResource.h"
#include "CoreTypes.h"
#include "GlobalShader.h"
#include "RHIStaticStates.h"
#include "Engine/RendererSettings.h"
#include "PixelShaderUtils.h"
#include "Engine.h"
#include "ImagePixelData.h"
#include "ImageWriteStream.h"
#include "ImageWriteTask.h"
#include "ImageWriteQueue.h"

#include "LensSolverUtilities.h"
#include "BlitShader.h"
#include "DistortionCorrectionMapGenerationShader.h"
#include "DistortionCorrectionShader.h"

void ULensSolver::BeginPlay()
{
	Super::BeginPlay();
}

void ULensSolver::EndPlay(const EEndPlayReason::Type EndPlayReason) 
{
	StopBackgroundImageprocessors();
	Super::EndPlay(EndPlayReason);
}

void ULensSolver::GenerateDistortionCorrectionMapRenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams,
	const FString correctionFilePath,
	const FString inverseCorrectionFilePath)
{
	FIntRect rect = FIntRect(0, 0, distortionCorrectionMapGenerationParams.outputMapResolution.X, distortionCorrectionMapGenerationParams.outputMapResolution.Y);
	int width = distortionCorrectionMapGenerationParams.outputMapResolution.X;
	int height = distortionCorrectionMapGenerationParams.outputMapResolution.Y;

	if (!distortionCorrectionRenderTextureAllocated)
	{
		FRHIResourceCreateInfo createInfo;
		FTexture2DRHIRef dummyTexRef;
		RHICreateTargetableShaderResource2D(
			width,
			height,
			EPixelFormat::PF_FloatRGBA,
			1,
			TexCreate_None,
			TexCreate_RenderTargetable,
			false,
			createInfo,
			distortionCorrectionRenderTexture,
			dummyTexRef);

		distortionCorrectionRenderTextureAllocated = true;
	}

	FVector2D normalizedPrincipalPoint = FVector2D(
		distortionCorrectionMapGenerationParams.sourcePrincipalPixelPoint.X / (float)distortionCorrectionMapGenerationParams.sourceResolution.X,
		distortionCorrectionMapGenerationParams.sourcePrincipalPixelPoint.Y / (float)distortionCorrectionMapGenerationParams.sourceResolution.Y);

	FString message = FString("Submitting the following parameters to distortion corretion map generation shader:\n{");
	message += FString::Printf(TEXT("\n\tNormalized principal point: (%f, %f),\n\tDistortion Coefficients: [k1: %f, k2: %f, p1: %f, p2: %f, k3: %f]\n}"), 
		normalizedPrincipalPoint.X, 
		normalizedPrincipalPoint.Y,
		distortionCorrectionMapGenerationParams.distortionCoefficients[0],
		distortionCorrectionMapGenerationParams.distortionCoefficients[1],
		distortionCorrectionMapGenerationParams.distortionCoefficients[2],
		distortionCorrectionMapGenerationParams.distortionCoefficients[3],
		distortionCorrectionMapGenerationParams.distortionCoefficients[4]);

	UE_LOG(LogTemp, Log, TEXT("%s"), *message);

	const ERHIFeatureLevel::Type RenderFeatureLevel = GMaxRHIFeatureLevel;
	const auto GlobalShaderMap = GetGlobalShaderMap(RenderFeatureLevel);

	TShaderMapRef<FDistortionCorrectionMapGenerationVS> VertexShader(GlobalShaderMap);
	TShaderMapRef<FDistortionCorrectionMapGenerationPS> PixelShader(GlobalShaderMap);

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGB, BO_Add, BF_One, BF_SourceAlpha>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	FRHIRenderPassInfo RPInfo(distortionCorrectionRenderTexture, ERenderTargetActions::Clear_DontStore);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("GenerateDistortionCorrectionMapPass"));
	{
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		RHICmdList.SetViewport(0, 0, 0.0f, width, height, 1.0f);
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
		PixelShader->SetParameters(RHICmdList, normalizedPrincipalPoint, distortionCorrectionMapGenerationParams.distortionCoefficients, false);
		FPixelShaderUtils::DrawFullscreenQuad(RHICmdList, 1);
	}
	RHICmdList.EndRenderPass();

	FRHITexture2D * texture2D = distortionCorrectionRenderTexture->GetTexture2D();
	TUniquePtr<TImagePixelData<FFloat16Color>> pixelData = MakeUnique<TImagePixelData<FFloat16Color>>(rect.Size());

	RHICmdList.ReadSurfaceFloatData(texture2D, rect, pixelData->Pixels, (ECubeFace)0, 0, 0);
	check(pixelData->IsDataWellFormed());

	TArray<FFloat16Color> distortionCorrectionPixels = pixelData->Pixels;
	if (!LensSolverUtilities::WriteTexture16(correctionFilePath, width, height, MoveTemp(pixelData)))
		return;

	UE_LOG(LogTemp, Log, TEXT("Wrote distortion correction map to path: \"%s\"."), *correctionFilePath);

	RHICmdList.BeginRenderPass(RPInfo, TEXT("GenerateInverseDistortionCorrectionMapPass"));
	{
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		RHICmdList.SetViewport(0, 0, 0.0f, width, height, 1.0f);
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
		PixelShader->SetParameters(RHICmdList, normalizedPrincipalPoint, distortionCorrectionMapGenerationParams.distortionCoefficients, true);
		FPixelShaderUtils::DrawFullscreenQuad(RHICmdList, 1);
	}
	RHICmdList.EndRenderPass();

	texture2D = distortionCorrectionRenderTexture->GetTexture2D();
	pixelData = MakeUnique<TImagePixelData<FFloat16Color>>(rect.Size());

	RHICmdList.ReadSurfaceFloatData(texture2D, rect, pixelData->Pixels, (ECubeFace)0, 0, 0);
	check(pixelData->IsDataWellFormed());

	TArray<FFloat16Color> inverseDistortionCorrectionPixels = pixelData->Pixels;
	if (!LensSolverUtilities::WriteTexture16(inverseCorrectionFilePath, width, height, MoveTemp(pixelData)))
		return;

	UE_LOG(LogTemp, Log, TEXT("Wrote inverse distortion correction map to path: \"%s\"."), *inverseCorrectionFilePath);

	if (!queuedDistortionCorrectionMapResults.IsValid())
		return;

	FDistortionCorrectionMapGenerationResults distortionCorrectionMapGenerationResults;
	distortionCorrectionMapGenerationResults.distortionCorrectionPixels = distortionCorrectionPixels;
	distortionCorrectionMapGenerationResults.inverseDistortionCorrectionPixels = inverseDistortionCorrectionPixels;
	distortionCorrectionMapGenerationResults.width = texture2D->GetSizeX();
	distortionCorrectionMapGenerationResults.height = texture2D->GetSizeY();

	queuedDistortionCorrectionMapResults->Enqueue(distortionCorrectionMapGenerationResults);
}

void ULensSolver::UndistortImageRenderThread(
	FRHICommandListImmediate& RHICmdList, 
	const FDistortTextureWithTextureParams distortionCorrectionParams, 
	const FString generatedOutputPath)
{
	int width = distortionCorrectionParams.distortedTexture->GetSizeX();
	int height = distortionCorrectionParams.distortedTexture->GetSizeY();
	if (!correctDistortedTextureRenderTextureAllocated)
	{
		FRHIResourceCreateInfo createInfo;
		FTexture2DRHIRef dummyTexRef;
		RHICreateTargetableShaderResource2D(
			width,
			height,
			EPixelFormat::PF_B8G8R8A8,
			1,
			TexCreate_SRGB,
			TexCreate_RenderTargetable,
			false,
			createInfo,
			correctDistortedTextureRenderTexture,
			dummyTexRef);

		correctDistortedTextureRenderTextureAllocated = true;
	}

	FRHIRenderPassInfo RPInfo(correctDistortedTextureRenderTexture, ERenderTargetActions::Clear_DontStore);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("CorrectImageDistortionPass"));
	{
		const ERHIFeatureLevel::Type RenderFeatureLevel = GMaxRHIFeatureLevel;
		const auto GlobalShaderMap = GetGlobalShaderMap(RenderFeatureLevel);

		TShaderMapRef<FDistortionCorrectionShaderVS> VertexShader(GlobalShaderMap);
		TShaderMapRef<FDistortionCorrectionShaderPS> PixelShader(GlobalShaderMap);

		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		RHICmdList.SetViewport(0, 0, 0.0f, width, height, 1.0f);

		GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGB, BO_Add, BF_One, BF_SourceAlpha>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
		PixelShader->SetParameters(
			RHICmdList,
			distortionCorrectionParams.distortedTexture->TextureReference.TextureReferenceRHI.GetReference(),
			distortionCorrectionParams.distortionCorrectionTexture->TextureReference.TextureReferenceRHI.GetReference(),
			distortionCorrectionParams.reverseOperation);

		FPixelShaderUtils::DrawFullscreenQuad(RHICmdList, 1);
	}

	RHICmdList.EndRenderPass();

	FRHITexture2D * texture2D = correctDistortedTextureRenderTexture->GetTexture2D();
	TArray<FColor> surfaceData;

	FReadSurfaceDataFlags ReadDataFlags;
	ReadDataFlags.SetLinearToGamma(false);
	ReadDataFlags.SetOutputStencil(false);
	ReadDataFlags.SetMip(0);

	RHICmdList.ReadSurfaceData(texture2D, FIntRect(0, 0, width, height), surfaceData, ReadDataFlags);

	uint32 ExtendXWithMSAA = surfaceData.Num() / texture2D->GetSizeY();
	FFileHelper::CreateBitmap(*generatedOutputPath, ExtendXWithMSAA, texture2D->GetSizeY(), surfaceData.GetData());
	// FFileHelper::CreateBitmap(*generatedOutputPath, texture2D->GetSizeX(), texture2D->GetSizeY(), surfaceData.GetData());
	UE_LOG(LogTemp, Log, TEXT("Wrote corrected distorted image to path: \"%s\"."), *generatedOutputPath);

	if (!queuedCorrectedDistortedImageResults.IsValid())
		return;

	FCorrectedDistortedImageResults correctedDistortedImageResults;

	correctedDistortedImageResults.pixels = surfaceData;
	correctedDistortedImageResults.width = texture2D->GetSizeX();
	correctedDistortedImageResults.height = texture2D->GetSizeY();

	queuedCorrectedDistortedImageResults->Enqueue(correctedDistortedImageResults);
}

bool ULensSolver::ValidateMediaTexture(const UMediaTexture* inputTexture)
{
	if (inputTexture == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot process null texture."));
		return false;
	}

	if (inputTexture->GetWidth() <= 3 ||
		inputTexture->GetHeight() <= 3)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot process texture, it's to small."));
		return false;
	}

	return true;
}

void ULensSolver::PollLogs()
{
	while (!logQueue.IsEmpty())
	{
		FString msg;
		logQueue.Dequeue(msg);
		UE_LOG(LogTemp, Log, TEXT("%s"), *msg);
	}
}

void ULensSolver::PollCalibrationResults()
{
	bool isQueued = LensSolverWorkDistributor::GetInstance().CalibrationResultIsQueued();
	while (isQueued)
	{
		FCalibrationResult calibrationResult;
		LensSolverWorkDistributor::GetInstance().DequeueCalibrationResult(calibrationResult);

		this->OnReceiveCalibrationResult(calibrationResult);

		isQueued = LensSolverWorkDistributor::GetInstance().CalibrationResultIsQueued();
	}
}

void ULensSolver::PollFinishedJobs()
{
	bool isQueued = queuedFinishedJobs.IsEmpty() == false;
	while (isQueued)
	{
		FJobInfo jobInfo;
		DequeuedFinishedJob(jobInfo);
		UE_LOG(LogTemp, Log, TEXT("Completed job: \"%s\", job will be unregistered."), *jobInfo.jobID);
		this->OnFinishedJob(jobInfo);
		isQueued = queuedFinishedJobs.IsEmpty() == false;
	}
}

void ULensSolver::PollDistortionCorrectionMapGenerationResults()
{
	if (!queuedDistortionCorrectionMapResults.IsValid())
		return;

	bool isQueued = queuedDistortionCorrectionMapResults->IsEmpty() == false;
	while (isQueued)
	{
		FDistortionCorrectionMapGenerationResults distortionCorrectionMapResult;
		if (!queuedDistortionCorrectionMapResults.IsValid())
			return;

		queuedDistortionCorrectionMapResults->Dequeue(distortionCorrectionMapResult);

		UE_LOG(LogTemp, Log, TEXT("Dequeued distortion correction map result from render thread of size: (%d, %d)."), 
			distortionCorrectionMapResult.width, 
			distortionCorrectionMapResult.height);


		UTexture2D* output = nullptr;
		if (!LensSolverUtilities::CreateTexture2D((void*)distortionCorrectionMapResult.distortionCorrectionPixels.GetData(), distortionCorrectionMapResult.width, distortionCorrectionMapResult.height, false, true, output, PF_FloatRGBA))
			return;
		this->OnGeneratedDistortionMap(output);

		isQueued = queuedDistortionCorrectionMapResults->IsEmpty() == false;
	}
}

void ULensSolver::PollCorrectedDistortedImageResults()
{
	if (!queuedCorrectedDistortedImageResults.IsValid())
		return;

	bool isQueued = queuedCorrectedDistortedImageResults->IsEmpty() == false;
	while (isQueued)
	{
		FCorrectedDistortedImageResults correctedDistortedImageResult;
		if (!queuedCorrectedDistortedImageResults.IsValid())
			return;

		queuedCorrectedDistortedImageResults->Dequeue(correctedDistortedImageResult);

		UE_LOG(LogTemp, Log, TEXT("Dequeued corrected distorted image result from render thread of size: (%d, %d)."), 
			correctedDistortedImageResult.width, 
			correctedDistortedImageResult.height);

		UTexture2D* output = nullptr;
		if (!LensSolverUtilities::CreateTexture2D((void*)correctedDistortedImageResult.pixels.GetData(), correctedDistortedImageResult.width, correctedDistortedImageResult.height, true, false, output))
			return;
		this->OnDistortedImageCorrected(output);

		isQueued = queuedCorrectedDistortedImageResults->IsEmpty() == false;
	}
}

void ULensSolver::QueueFinishedJob(FJobInfo jobInfo)
{
	queuedFinishedJobs.Enqueue(jobInfo);
}

bool ULensSolver::FinishedJobIsQueued()
{
	return queuedFinishedJobs.IsEmpty() == false;
}

void ULensSolver::DequeuedFinishedJob(FJobInfo& jobInfo)
{
	queuedFinishedJobs.Dequeue(jobInfo);
}

void ULensSolver::QueueLog(FString msg)
{
	logQueue.Enqueue(msg);
}

void ULensSolver::OneTimeProcessArrayOfTextureFolderZoomPairs(
	TArray<FTextureFolderZoomPair> inputTextures, 
	FOneTimeProcessParameters oneTimeProcessParameters, 
	FJobInfo& ouptutJobInfo)
{
	if (inputTextures.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No input texture folders."));
		return;
	}

	if (LensSolverWorkDistributor::GetInstance().GetFindCornerWorkerCount() <= 0 || LensSolverWorkDistributor::GetInstance().GetCalibrateCount() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No workers available, make sure you start both background \"FindCorner\" & \"Calibrate\" workers."));
		return;
	}

	int useCount = 0, useIndex = 0, offset = 0;
	for (int ti = 0; ti < inputTextures.Num(); ti++)
		useCount += inputTextures[ti].use;

	TArray<TArray<FString>> imageFiles;
	TArray<int> expectedImageCounts;
	TArray<float> zoomLevels;

	imageFiles.SetNum(useCount);
	expectedImageCounts.SetNum(useCount);
	zoomLevels.SetNum(useCount);

	for (int ti = 0; ti < inputTextures.Num(); ti++)
	{
		useIndex = ti - offset;
		TArray<FString> imagesInDirectory;
		TArray<UTexture2D*> textures;

		if (!inputTextures[ti].use)
		{
			offset++;
			continue;
		}

		if (!LensSolverUtilities::GetFilesInFolder(inputTextures[ti].absoluteFolderPath, imageFiles[useIndex]))
			return;

		if (imageFiles[ti].Num() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("No textures in directory: \"%s\", canceled job."), *inputTextures[ti].absoluteFolderPath);
			return;
		}

		expectedImageCounts[useIndex] = imageFiles[ti].Num();
		zoomLevels[useIndex] = inputTextures[ti].zoomLevel;
	}

	LensSolverWorkDistributor::GetInstance().SetCalibrateWorkerParameters(oneTimeProcessParameters.calibrationParameters);
	ouptutJobInfo = LensSolverWorkDistributor::GetInstance().RegisterJob(expectedImageCounts, useCount, OneTime);
	for (int ci = 0; ci < useCount; ci++)
	{
		for (int ii = 0; ii < imageFiles[ci].Num(); ii++)
		{
			FLensSolverTextureFileWorkUnit workUnit;
			workUnit.baseParameters.jobID						= ouptutJobInfo.jobID;
			workUnit.baseParameters.calibrationID				= ouptutJobInfo.calibrationIDs[ci];
			workUnit.baseParameters.zoomLevel					= zoomLevels[ci];
			workUnit.baseParameters.friendlyName				= FPaths::GetBaseFilename(imageFiles[ci][ii]);
			workUnit.textureSearchParameters					= oneTimeProcessParameters.textureSearchParameters;
			workUnit.textureFileParameters.absoluteFilePath		= imageFiles[ci][ii];

			LensSolverWorkDistributor::GetInstance().QueueTextureFileWorkUnit(ouptutJobInfo.jobID, workUnit);
		}
	}
}

void ULensSolver::StartMediaStreamCalibration(
	FStartMediaStreamParameters mediaStreamParameters,
	FJobInfo& ouptutJobInfo)
{
	if (!ValidateMediaTexture(mediaStreamParameters.mediaStreamParameters.mediaTexture))
	{
		UE_LOG(LogTemp, Error, TEXT("The input MediaStreamParameters member \"Media Texture\" invalid!"));
		return;
	}

	if (mediaStreamParameters.mediaStreamParameters.expectedStreamSnapshotCount <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("The input MediaStreamParameters member \"Expected Stream Snapshot Count\" should be a positive number greater than zero!"));
		return;
	}

	if (mediaStreamParameters.mediaStreamParameters.streamSnapshotIntervalFrequencyInSeconds <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("The input MediaStreamParameters member \"Stream Snapshot Interval Frequency In Seconds\" should be a positive number greater than zero!"));
		return;
	}

	if (mediaStreamParameters.mediaStreamParameters.zoomLevel < 0.0f || mediaStreamParameters.mediaStreamParameters.zoomLevel > 1.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("The input MediaStreamParameters member \"Zoom Level\" should be a normalized value between 0 - 1!"));
		return;
	}

	TArray<int> expectedImageCounts;
	expectedImageCounts.Add(mediaStreamParameters.mediaStreamParameters.expectedStreamSnapshotCount);

	ouptutJobInfo = LensSolverWorkDistributor::GetInstance().RegisterJob(expectedImageCounts, 1, OneTime);

	FMediaStreamWorkUnit workUnit;
	workUnit.baseParameters.jobID								= ouptutJobInfo.jobID;
	workUnit.baseParameters.calibrationID						= ouptutJobInfo.calibrationIDs[0];
	workUnit.baseParameters.friendlyName						= "stream";
	workUnit.baseParameters.zoomLevel							= mediaStreamParameters.mediaStreamParameters.zoomLevel;
	workUnit.textureSearchParameters							= mediaStreamParameters.textureSearchParameters;
	workUnit.mediaStreamParameters								= mediaStreamParameters.mediaStreamParameters;
	workUnit.mediaStreamParameters.currentStreamSnapshotCount	= 0;

	LensSolverWorkDistributor::GetInstance().QueueMediaStreamWorkUnit(workUnit);
}

void ULensSolver::GenerateDistortionCorrectionMap(
	const FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams)
{
	if (distortionCorrectionMapGenerationParams.distortionCoefficients.Num() != 5)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot generate distortion correction map, there should an array of 5 float value distortion coefficients in the calibration result DistortionCorrectionMapParameter member."));
		return;
	}

	if (distortionCorrectionMapGenerationParams.outputMapResolution.X <= 3 || distortionCorrectionMapGenerationParams.outputMapResolution.Y <= 3)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot generate distortion correction map, the map resolution DistortionCorrectionMapParameter member is <= 3 pixels on the X or Y axis."));
		return;
	}

	static const FString backupOutputPath = LensSolverUtilities::GenerateGenericOutputPath(FString("DistortionCorrectionMaps/"));
	FString correctionOutputPath = distortionCorrectionMapGenerationParams.correctionOutputPath;
	if (!LensSolverUtilities::ValidatePath(correctionOutputPath, backupOutputPath, FString("DistortionCorrectionMap"), FString("png"), FString("Distortion Correction: ")))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot generate distortion correction map, unable to create folder path: \"%s\"."), *correctionOutputPath);
		return;
	}

	FString inverseCorrectionOutputPath = distortionCorrectionMapGenerationParams.inverseCorrectionOutputPath;
	if (!LensSolverUtilities::ValidatePath(inverseCorrectionOutputPath, backupOutputPath, FString("InverseDistortionCorrectionMap"), FString("png"), FString("Distortion Correction: ")))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot generate inverse distortion correction map, unable to create folder path: \"%s\"."), *inverseCorrectionOutputPath);
		return;
	}

	ULensSolver * lensSolver = this;
	const FDistortionCorrectionMapGenerationParameters temp = distortionCorrectionMapGenerationParams;

	if (!queuedDistortionCorrectionMapResults.IsValid())
		queuedDistortionCorrectionMapResults = MakeShareable(new TQueue<FDistortionCorrectionMapGenerationResults>);

	UE_LOG(LogTemp, Log, TEXT("Queuing render command to generate distortion correction map of size: (%d, %d)."),
		distortionCorrectionMapGenerationParams.outputMapResolution.X,
		distortionCorrectionMapGenerationParams.outputMapResolution.Y);

	ENQUEUE_RENDER_COMMAND(GenerateDistortionCorrectionMap)
	(
		[lensSolver, temp, correctionOutputPath, inverseCorrectionOutputPath](FRHICommandListImmediate& RHICmdList)
		{
			lensSolver->GenerateDistortionCorrectionMapRenderThread(
				RHICmdList,
				temp,
				correctionOutputPath,
				inverseCorrectionOutputPath);
		}
	);
}

void ULensSolver::DistortTextureWithTexture(const FDistortTextureWithTextureParams distortionCorrectionParams)
{
	if (distortionCorrectionParams.distortedTexture == nullptr || distortionCorrectionParams.distortionCorrectionTexture == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot correct distorted texture, one of the texture DistortionCorrectionParameters members is NULL!"));
		return;
	}

	if (distortionCorrectionParams.distortedTexture->GetSizeX() <= 3 || 
		distortionCorrectionParams.distortedTexture->GetSizeY() <= 3 ||
		distortionCorrectionParams.distortionCorrectionTexture->GetSizeX() <= 3 ||
		distortionCorrectionParams.distortionCorrectionTexture->GetSizeY() <= 3)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot correct distorted texture, one of the texture DistortionCorrectionParameters members is to small."));
		return;
	}

	static const FString backupOutputPath = LensSolverUtilities::GenerateGenericOutputPath(FString("CorrectedDistortedImages/"));
	FString targetOutputPath = distortionCorrectionParams.outputPath;

	if (!LensSolverUtilities::ValidatePath(targetOutputPath, backupOutputPath, FString("CorrectedDistortedImage"), FString("bmp"), FString("Distortion Correction: ")))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot generate distortion correction map, unable to create folder path: \"%s\"."), *targetOutputPath);
		return;
	}

	ULensSolver * lensSolver = this;
	const FDistortTextureWithTextureParams tempDistortionCorrectionParams = distortionCorrectionParams;

	if (!queuedCorrectedDistortedImageResults.IsValid())
		queuedCorrectedDistortedImageResults = MakeShareable(new TQueue<FCorrectedDistortedImageResults>);

	UE_LOG(LogTemp, Log, TEXT("Queuing render command to correct distorted image of size: (%d, %d)."),
		distortionCorrectionParams.distortedTexture->GetSizeX(),
		distortionCorrectionParams.distortedTexture->GetSizeY());

	ENQUEUE_RENDER_COMMAND(CorrectionImageDistortion)
	(
		[lensSolver, tempDistortionCorrectionParams, targetOutputPath](FRHICommandListImmediate& RHICmdList)
		{
			lensSolver->UndistortImageRenderThread(
				RHICmdList,
				tempDistortionCorrectionParams,
				targetOutputPath);
		}
	);
}

void ULensSolver::DistortTextureWithTextureFile(FDistortTextureWithTextureFileParams distortionCorrectionParams)
{
	UTexture2D* texture = nullptr;
	if (!LensSolverUtilities::LoadTexture(distortionCorrectionParams.absoluteFilePath, false, true, texture, EPixelFormat::PF_FloatRGBA))
		return;

	FDistortTextureWithTextureParams newParams;
	newParams.distortedTexture = distortionCorrectionParams.distortedTexture;
	newParams.distortionCorrectionTexture = texture;
	newParams.reverseOperation = distortionCorrectionParams.reverseOperation;
	newParams.zoomLevel = distortionCorrectionParams.zoomLevel;
	newParams.outputPath = distortionCorrectionParams.outputPath;

	DistortTextureWithTexture(newParams);
}

void ULensSolver::DistortTextureWithCoefficients(FDistortTextureWithCoefficientsParams distortionCorrectionParams)
{
}

void ULensSolver::StartBackgroundImageProcessors(int findCornersWorkerCount, int calibrateWorkerCount)
{
	LensSolverWorkDistributor::GetInstance().Configure(queueLogOutputDel, queueFinishedJobOutputDel, debug);

	if (queueLogOutputDel->IsBound())
		queueLogOutputDel->Unbind();

	queueLogOutputDel->BindUObject(this, &ULensSolver::QueueLog);
	UE_LOG(LogTemp, Log, TEXT("Binded log queue."));

	if (queueFinishedJobOutputDel->IsBound())
		queueFinishedJobOutputDel->Unbind();

	queueFinishedJobOutputDel->BindUObject(this, &ULensSolver::QueueFinishedJob);
	UE_LOG(LogTemp, Log, TEXT("Binded finished queue."));


	LensSolverWorkDistributor::GetInstance().PrepareFindCornerWorkers(findCornersWorkerCount);
	LensSolverWorkDistributor::GetInstance().PrepareCalibrateWorkers(calibrateWorkerCount);
}

void ULensSolver::StopBackgroundImageprocessors()
{
	LensSolverWorkDistributor::GetInstance().StopBackgroundWorkers();
}

void ULensSolver::Poll()
{
	PollLogs();
	PollCalibrationResults();
	PollFinishedJobs();
	PollDistortionCorrectionMapGenerationResults();
	PollCorrectedDistortedImageResults();

	LensSolverWorkDistributor::GetInstance().PollMediaTextureStreams();
}
