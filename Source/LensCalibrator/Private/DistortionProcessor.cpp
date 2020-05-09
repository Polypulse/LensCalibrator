#include "DistortionProcessor.h"

#include "TextureResource.h"
#include "CoreTypes.h"
#include "GlobalShader.h"
#include "RHIStaticStates.h"
#include "Engine/RendererSettings.h"
#include "PixelShaderUtils.h"
#include "Engine.h"
#include "ImagePixelData.h"
#include "LensSolverUtilities.h"

#include "DistortionCorrectionMapGenerationShader.h"
#include "DistortionCorrectionShader.h"

void UDistortionProcessor::GenerateDistortionCorrectionMapRenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams,
	const FString correctionFilePath,
	const FString inverseCorrectionFilePath)
{
	FIntRect rect = FIntRect(0, 0, distortionCorrectionMapGenerationParams.outputMapResolution.X, distortionCorrectionMapGenerationParams.outputMapResolution.Y);
	int width = distortionCorrectionMapGenerationParams.outputMapResolution.X;
	int height = distortionCorrectionMapGenerationParams.outputMapResolution.Y;

	FTexture2DRHIRef distortionCorrectionRenderTexture;
	FRHIResourceCreateInfo createInfo;
	FTexture2DRHIRef dummyTexRef;

	RHICreateTargetableShaderResource2D(
		width,
		height,
		EPixelFormat::PF_FloatRGBA,
		1,
		TexCreate_Transient,
		TexCreate_RenderTargetable,
		false,
		createInfo,
		distortionCorrectionRenderTexture,
		dummyTexRef);

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
	{

	}

	UE_LOG(LogTemp, Log, TEXT("Wrote inverse distortion correction map to path: \"%s\"."), *inverseCorrectionFilePath);

	FDistortionCorrectionMapGenerationResults distortionCorrectionMapGenerationResults;
	distortionCorrectionMapGenerationResults.id = distortionCorrectionMapGenerationParams.id;
	distortionCorrectionMapGenerationResults.distortionCorrectionPixels = distortionCorrectionPixels;
	distortionCorrectionMapGenerationResults.inverseDistortionCorrectionPixels = inverseDistortionCorrectionPixels;
	distortionCorrectionMapGenerationResults.width = texture2D->GetSizeX();
	distortionCorrectionMapGenerationResults.height = texture2D->GetSizeY();

	queuedDistortionCorrectionMapResults.Enqueue(distortionCorrectionMapGenerationResults);
}

void UDistortionProcessor::UndistortImageRenderThread(
	FRHICommandListImmediate& RHICmdList, 
	const FDistortTextureWithTextureParams distortionCorrectionParams, 
	const FString generatedOutputPath)
{
	int width = distortionCorrectionParams.distortedTexture->GetSizeX();
	int height = distortionCorrectionParams.distortedTexture->GetSizeY();

	FTexture2DRHIRef correctDistortedTextureRenderTexture;
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

	FCorrectedDistortedImageResults correctedDistortedImageResults;
	correctedDistortedImageResults.id = distortionCorrectionParams.id;
	correctedDistortedImageResults.pixels = surfaceData;
	correctedDistortedImageResults.width = texture2D->GetSizeX();
	correctedDistortedImageResults.height = texture2D->GetSizeY();

	queuedCorrectedDistortedImageResults.Enqueue(correctedDistortedImageResults);
}

void UDistortionProcessor::PollDistortionCorrectionMapGenerationResults()
{
	bool isQueued = queuedDistortionCorrectionMapResults.IsEmpty() == false;
	while (isQueued)
	{
		FDistortionCorrectionMapGenerationResults result;
		queuedDistortionCorrectionMapResults.Dequeue(result);

		UE_LOG(LogTemp, Log, TEXT("(INFO): Dequeued distortion result of id: \"%s\"."), 
			*result.id);

		DistortionJob* job = cachedEvents.Find(result.id);
		if (job != nullptr)
		{
			UTexture2D* map = nullptr;
			if (LensSolverUtilities::CreateTexture2D(result.distortionCorrectionPixels.GetData(), result.width, result.height, false, true, map, EPixelFormat::PF_FloatRGBA))
			{
				if (job->eventReceiver.GetObject()->IsValidLowLevel())
					job->eventReceiver->Execute_OnGeneratedDistortionMap(job->eventReceiver.GetObject(), map);
			}

			else
				UE_LOG(LogTemp, Error, TEXT("Unable to create texture for distortion correction map."));
		}

		else
			UE_LOG(LogTemp, Error, TEXT("(INFO): No cached event interface for distortion job id: \"%s\"."), 
				*result.id);

		cachedEvents.Remove(result.id);
		isQueued = queuedDistortionCorrectionMapResults.IsEmpty() == false;
	}
}

void UDistortionProcessor::PollCorrectedDistortedImageResults()
{
	bool isQueued = queuedCorrectedDistortedImageResults.IsEmpty() == false;
	while (isQueued)
	{
		FCorrectedDistortedImageResults result;
		queuedCorrectedDistortedImageResults.Dequeue(result);

		UE_LOG(LogTemp, Log, TEXT("(INFO): Dequeued distortion result of id: \"%s\"."), 
			*result.id);

		DistortionJob* job = cachedEvents.Find(result.id);
		if (job != nullptr)
		{
			UTexture2D* map = nullptr;
			if (LensSolverUtilities::CreateTexture2D(result.pixels.GetData(), result.width, result.height, true, false, map))
			{
				if (job->eventReceiver.GetObject()->IsValidLowLevel())
					job->eventReceiver->Execute_OnDistortedImageCorrected(job->eventReceiver.GetObject(), map);
			}

			else
				UE_LOG(LogTemp, Error, TEXT("Unable to create texture for corrected distorted image."));
		}

		else
			UE_LOG(LogTemp, Error, TEXT("(INFO): No cached event interface for distortion job id: \"%s\"."), 
				*result.id);

		cachedEvents.Remove(result.id);
		isQueued = queuedCorrectedDistortedImageResults.IsEmpty() == false;
	}
}

void UDistortionProcessor::DistortTextureWithTexture(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FDistortTextureWithTextureParams distortionCorrectionParams)
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

	FString guid = FGuid::NewGuid().ToString();

	DistortionJob job;
	job.eventReceiver = eventReceiver;
	job.id = guid;

	distortionCorrectionParams.id = guid;
	cachedEvents.Add(guid, job);

	UDistortionProcessor * distortionProcessor = this;
	const FDistortTextureWithTextureParams tempDistortionCorrectionParams = distortionCorrectionParams;

	UE_LOG(LogTemp, Log, TEXT("Queuing render command to correct distorted image of size: (%d, %d)."),
		distortionCorrectionParams.distortedTexture->GetSizeX(),
		distortionCorrectionParams.distortedTexture->GetSizeY());

	ENQUEUE_RENDER_COMMAND(CorrectionImageDistortion)
	(
		[distortionProcessor, tempDistortionCorrectionParams, targetOutputPath](FRHICommandListImmediate& RHICmdList)
		{
			distortionProcessor->UndistortImageRenderThread(
				RHICmdList,
				tempDistortionCorrectionParams,
				targetOutputPath);
		}
	);
}

void UDistortionProcessor::DistortTextureWithTextureFile(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FDistortTextureWithTextureFileParams distortionCorrectionParams)
{
	UTexture2D* texture = nullptr;
	if (!LensSolverUtilities::LoadTexture16(distortionCorrectionParams.absoluteFilePath, texture))
		return;

	FDistortTextureWithTextureParams newParams;
	newParams.distortedTexture = distortionCorrectionParams.distortedTexture;
	newParams.distortionCorrectionTexture = texture;
	newParams.reverseOperation = distortionCorrectionParams.reverseOperation;
	newParams.zoomLevel = distortionCorrectionParams.zoomLevel;
	newParams.outputPath = distortionCorrectionParams.outputPath;

	DistortTextureWithTexture(eventReceiver, newParams);
}

void UDistortionProcessor::DistortTextureWithCoefficients(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FDistortTextureWithCoefficientsParams distortionCorrectionParams)
{
}

void UDistortionProcessor::Poll()
{
	PollDistortionCorrectionMapGenerationResults();
	PollCorrectedDistortedImageResults();
}

void UDistortionProcessor::GenerateDistortionCorrectionMap(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver,
	FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams)
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

	FString guid = FGuid::NewGuid().ToString();

	DistortionJob job;
	job.eventReceiver = eventReceiver;
	job.id = guid;

	cachedEvents.Add(guid, job);
	distortionCorrectionMapGenerationParams.id = guid;

	UDistortionProcessor * distortionProcessor = this;
	const FDistortionCorrectionMapGenerationParameters temp = distortionCorrectionMapGenerationParams;

	UE_LOG(LogTemp, Log, TEXT("Queuing render command to generate distortion correction map of size: (%d, %d)."),
		distortionCorrectionMapGenerationParams.outputMapResolution.X,
		distortionCorrectionMapGenerationParams.outputMapResolution.Y);

	ENQUEUE_RENDER_COMMAND(GenerateDistortionCorrectionMap)
	(
		[distortionProcessor, temp, correctionOutputPath, inverseCorrectionOutputPath](FRHICommandListImmediate& RHICmdList)
		{
			distortionProcessor->GenerateDistortionCorrectionMapRenderThread(
				RHICmdList,
				temp,
				correctionOutputPath,
				inverseCorrectionOutputPath);
		}
	);
}

