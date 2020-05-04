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
#include "UniquePtr.h"

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

FJobInfo ULensSolver::RegisterJob(int workUnitCount, UJobType jobType)
{
	FJobInfo jobInfo;
	jobInfo.jobID = FGuid::NewGuid().ToString();
	jobInfo.workUnitCount = workUnitCount;
	jobInfo.jobType = jobType;

	FJob job;
	job.jobInfo = jobInfo;
	job.completedWorkUnits = 0;

	jobs.Add(jobInfo.jobID, job);

	UE_LOG(LogTemp, Log, TEXT("Registered new job: \"%s\"."), *jobInfo.jobID);

	return jobInfo;
}

int ULensSolver::GetWorkerCount()
{
	threadLock.Lock();
	int workercount = workers.Num();
	threadLock.Unlock();

	return workercount;
;
}

void ULensSolver::BeginDetectPoints(
	const FJobInfo inputJobInfo,
	const FTextureZoomPair& inputTextureZoomPair,
	FOneTimeProcessParameters oneTimeProcessParameters,
	const bool inputLatch)
{
	if (!ValidateTexture(inputJobInfo, inputTextureZoomPair.texture, 0, FIntPoint(inputTextureZoomPair.texture->GetSizeX(), inputTextureZoomPair.texture->GetSizeY())))
	{
		ReturnErrorSolvedPoints(inputJobInfo);
		return;
	}

	if (!ValidateZoom(inputJobInfo, inputTextureZoomPair.zoomLevel))
	{
		ReturnErrorSolvedPoints(inputJobInfo);
		return;
	}

	if (!ValidateOneTimeProcessParameters(oneTimeProcessParameters))
	{
		ReturnErrorSolvedPoints(inputJobInfo);
		return;
	}

	if (GetWorkerCount() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot process any textures, you need to execute StartBackgroundTextureProcessors in order to process textures."));
		ReturnErrorSolvedPoints(inputJobInfo);
		return;
	}

	oneTimeProcessParameters.currentResolution = FIntPoint(inputTextureZoomPair.texture->GetSizeX(), inputTextureZoomPair.texture->GetSizeY());

	FJobInfo jobInfo = inputJobInfo;
	FTextureZoomPair textureZoomPair = inputTextureZoomPair;
	FOneTimeProcessParameters tempFirstPassParameters = oneTimeProcessParameters;
	bool latch = inputLatch;

	UE_LOG(LogTemp, Log, TEXT("Enqueuing calibration image render comand at resolution: (%d, %d)."), oneTimeProcessParameters.currentResolution.X, oneTimeProcessParameters.currentResolution.Y);

	ULensSolver * lensSolver = this;
	ENQUEUE_RENDER_COMMAND(OneTimeProcessMediaTexture)
	(
		[lensSolver, jobInfo, textureZoomPair, tempFirstPassParameters, latch](FRHICommandListImmediate& RHICmdList)
		{
			lensSolver->DetectPointsRenderThread(
				RHICmdList,
				jobInfo,
				textureZoomPair,
				tempFirstPassParameters,
				1,
				latch);
				
		}
	);
}

/*
void ULensSolver::BeginDetectPoints(
	const FJobInfo inputJobInfo,
	const UMediaTexture* inputMediaTexture,
	const float inputZoomLevel,
	FOneTimeProcessParameters oneTimeProcessParameters)
{
	if (!ValidateMediaTexture(inputJobInfo, inputMediaTexture))
	{
		ReturnErrorSolvedPoints(inputJobInfo);
		return;
	}

	if (!ValidateZoom(inputJobInfo, inputZoomLevel))
	{
		ReturnErrorSolvedPoints(inputJobInfo);
		return;
	}

	if (!ValidateOneTimeProcessParameters(oneTimeProcessParameters))
	{
		ReturnErrorSolvedPoints(inputJobInfo);
		return;
	}

	if (GetWorkerCount() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot process any textures, you need to execute StartBackgroundTextureProcessors in order to process textures."));
		ReturnErrorSolvedPoints(inputJobInfo);
		return;
	}

	oneTimeProcessParameters.currentResolution = FIntPoint(inputMediaTexture->GetWidth(), inputMediaTexture->GetHeight());

	FJobInfo jobInfo = inputJobInfo;

	const UMediaTexture* cachedMediaTextureReference = inputMediaTexture;
	float zoomLevel = inputZoomLevel;
	FOneTimeProcessParameters tempFirstPassParameters = oneTimeProcessParameters;

	ULensSolver * lensSolver = this;
	ENQUEUE_RENDER_COMMAND(OneTimeProcessMediaTexture)
	(
		[lensSolver, 
		jobInfo, 
		cachedMediaTextureReference, 
		zoomLevel,
		tempFirstPassParameters] (FRHICommandListImmediate& RHICmdList)
		{
			lensSolver->DetectPointsRenderThread(
				RHICmdList,
				jobInfo,
				cachedMediaTextureReference,
				zoomLevel,
				tempFirstPassParameters);
		}
	);
}
*/

void ULensSolver::RandomSortTArray(TArray<UTexture2D*>& arr)
{
	if (arr.Num() == 0)
		return;
	int32 lastIndex = arr.Num() - 1;
	for (int32 i = 0; i <= lastIndex; ++i)
	{
		int32 index = FMath::RandRange(i, lastIndex);
		if (i != index)
			arr.Swap(i, index);
	}
}

void ULensSolver::BeginDetectPoints(
	const FJobInfo jobInfo,
	const TArray<FTextureZoomPair> & textureZoomPairs,
	FOneTimeProcessParameters oneTimeProcessParameters)
{
	if (textureZoomPairs.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No textures to process."))
		ReturnErrorSolvedPoints(jobInfo);
		return;
	}

	for (int i = 0; i < textureZoomPairs.Num(); i++)
		BeginDetectPoints(
			jobInfo,
			textureZoomPairs[i],
			oneTimeProcessParameters,
			true);
}

void ULensSolver::BeginDetectPoints(
	const FJobInfo inputJobInfo,
	FTextureArrayZoomPair& inputTextures,
	FOneTimeProcessParameters inputOneTimeProcessParameters)
{
	if (!ValidateZoom(inputJobInfo, inputTextures.zoomLevel))
	{
		ReturnErrorSolvedPoints(inputJobInfo);
		return;
	}

	if (!ValidateOneTimeProcessParameters(inputOneTimeProcessParameters))
	{
		ReturnErrorSolvedPoints(inputJobInfo);
		return;
	}

	if (GetWorkerCount() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot process any textures, you need to execute StartBackgroundTextureProcessors in order to process textures."));
		ReturnErrorSolvedPoints(inputJobInfo);
		return;
	}

	if (inputTextures.textures.Num() == 0 || inputTextures.textures[0] == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("No input textures."));
		ReturnErrorSolvedPoints(inputJobInfo);
		return;
	}

	FIntPoint targetResolution = FIntPoint(inputTextures.textures[0]->GetSizeX(), inputTextures.textures[0]->GetSizeY());
	for (int i = 0; i < inputTextures.textures.Num(); i++)
	{
		if (!ValidateTexture(inputJobInfo, inputTextures.textures[i], i, targetResolution))
		{
			ReturnErrorSolvedPoints(inputJobInfo);
			return;
		}
	}

	RandomSortTArray(inputTextures.textures);

	for (int i = 0; i < inputTextures.textures.Num(); i++)
	{
		inputOneTimeProcessParameters.currentResolution = FIntPoint(inputTextures.textures[i]->GetSizeX(), inputTextures.textures[i]->GetSizeY());
		FJobInfo jobInfo = inputJobInfo;
		FTextureZoomPair textureZoomPair = 
		{
			inputTextures.textures[i],
			inputTextures.zoomLevel
		};

		FOneTimeProcessParameters tempFirstPassParameters = inputOneTimeProcessParameters;
		const int latchImageCount = inputTextures.textures.Num();
		const bool latch = i == inputTextures.textures.Num() - 1;

		UE_LOG(LogTemp, Log, TEXT("Enqueuing calibration image render comand at resolution: (%d, %d)."), inputOneTimeProcessParameters.currentResolution.X, inputOneTimeProcessParameters.currentResolution.Y);

		ULensSolver * lensSolver = this;
		ENQUEUE_RENDER_COMMAND(OneTimeProcessMediaTexture)
		(
			[lensSolver, jobInfo, textureZoomPair, tempFirstPassParameters, latchImageCount, latch](FRHICommandListImmediate& RHICmdList)
			{
				lensSolver->DetectPointsRenderThread(
					RHICmdList,
					jobInfo,
					textureZoomPair,
					tempFirstPassParameters,
					latchImageCount,
					latch);
			}
		);
	}
}

void ULensSolver::BeginDetectPoints(
	const FJobInfo jobInfo,
	TArray<FTextureArrayZoomPair>& inputTextures,
	FOneTimeProcessParameters oneTimeProcessParameters)
{
	if (inputTextures.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No textures to process."))
		ReturnErrorSolvedPoints(jobInfo);
		return;
	}

	bool useAny = false;
	for (int i = 0; i < inputTextures.Num(); i++)
	{
		if (inputTextures[i].use)
		{
			useAny = true;
			break;
		}
	}

	if (!useAny)
	{
		UE_LOG(LogTemp, Error, TEXT("No textures to process, check your inputs and make sure you have the \"use\" flag checked."))
		ReturnErrorSolvedPoints(jobInfo);
		return;
	}

	for (int i = 0; i < inputTextures.Num(); i++)
	{
		if (inputTextures[i].use)
		{
			BeginDetectPoints(
				jobInfo,
				inputTextures[i],
				oneTimeProcessParameters);
		}
	}
}

/*
void ULensSolver::BeginDetectPoints(
	const FJobInfo jobInfo,
	TArray<UMediaTexture*> inputTextures, 
	TArray<float> inputZoomLevels, 
	FOneTimeProcessParameters oneTimeProcessParameters)
{
	if (inputTextures.Num() == 0 || inputZoomLevels.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No textures to process."))
		ReturnErrorSolvedPoints(jobInfo);
		return;
	}

	if (inputTextures.Num() != inputZoomLevels.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("The texture and zoom level count does not match!"));
		ReturnErrorSolvedPoints(jobInfo);
		return;
	}

	for (int i = 0; i < inputTextures.Num(); i++)
		BeginDetectPoints(jobInfo, inputTextures[i], inputZoomLevels[i], oneTimeProcessParameters);
}
*/

void ULensSolver::DetectPointsRenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FJobInfo jobInfo,
	const FTextureZoomPair textureZoomPair,
	FOneTimeProcessParameters oneTimeProcessParameters,
	const int latchImageCount,
	const bool latch)

{
	int width = oneTimeProcessParameters.resize ? oneTimeProcessParameters.currentResolution.X * oneTimeProcessParameters.resizePercentage : oneTimeProcessParameters.currentResolution.X;
	int height = oneTimeProcessParameters.resize ? oneTimeProcessParameters.currentResolution.Y * oneTimeProcessParameters.resizePercentage : oneTimeProcessParameters.currentResolution.Y;
	if (!blitRenderTextureAllocated)
	{
		FRHIResourceCreateInfo createInfo;
		FTexture2DRHIRef dummyTexRef;
		RHICreateTargetableShaderResource2D(
			width,
			height,
			EPixelFormat::PF_B8G8R8A8,
			1,
			TexCreate_None,
			TexCreate_RenderTargetable,
			false,
			createInfo,
			blitRenderTexture,
			dummyTexRef);

		blitRenderTextureAllocated = true;
	}

	FRHIRenderPassInfo RPInfo(blitRenderTexture, ERenderTargetActions::Clear_DontStore);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("PresentAndCopyMediaTexture"));
	{
		const ERHIFeatureLevel::Type RenderFeatureLevel = GMaxRHIFeatureLevel;
		const auto GlobalShaderMap = GetGlobalShaderMap(RenderFeatureLevel);

		TShaderMapRef<FBlitShaderVS> VertexShader(GlobalShaderMap);
		TShaderMapRef<FBlitShaderPS> PixelShader(GlobalShaderMap);

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
		PixelShader->SetParameters(RHICmdList, textureZoomPair.texture->TextureReference.TextureReferenceRHI.GetReference(), FVector2D(oneTimeProcessParameters.flipX ? -1.0f : 1.0f, oneTimeProcessParameters.flipY ? 1.0f : -1.0f));

		FPixelShaderUtils::DrawFullscreenQuad(RHICmdList, 1);
	}

	RHICmdList.EndRenderPass();

	FRHITexture2D * texture2D = blitRenderTexture->GetTexture2D();
	TArray<FColor> surfaceData;

	FReadSurfaceDataFlags ReadDataFlags;
	ReadDataFlags.SetLinearToGamma(false);
	ReadDataFlags.SetOutputStencil(false);
	ReadDataFlags.SetMip(0);

	// UE_LOG(LogTemp, Log, TEXT("Reading pixels from rect: (%d, %d, %d, %d)."), 0, 0, width, height);
	RHICmdList.ReadSurfaceData(texture2D, FIntRect(0, 0, width, height), surfaceData, ReadDataFlags);

	/*
	uint32 ExtendXWithMSAA = surfaceData.Num() / texture2D->GetSizeY();
	FString outputPath = FString("D:/output.bmp");
	FFileHelper::CreateBitmap(*outputPath, ExtendXWithMSAA, texture2D->GetSizeY(), surfaceData.GetData());
	*/

	threadLock.Lock();
	if (workers.Num() == 0)
	{
		threadLock.Unlock();
		return;
	}

	FString textureName;
	textureZoomPair.texture->GetName(textureName);

	FLensSolverWorkUnit workerUnit;
	workerUnit.unitName = textureName;
	workerUnit.pixels = surfaceData;

	if (nextWorkerIndex < 0 || nextWorkerIndex > workers.Num() - 1)
		nextWorkerIndex = 0;

	workers[nextWorkerIndex].queueWorkUnitDel.Execute(workerUnit);

	if (latch)
	{
		FLatchData latchData =
		{
			jobInfo,
			oneTimeProcessParameters.workerParameters,
			latchImageCount,
			textureZoomPair.zoomLevel,
			oneTimeProcessParameters.currentResolution,
			oneTimeProcessParameters.resize,
			oneTimeProcessParameters.resizePercentage,
			oneTimeProcessParameters.cornerCount,
			oneTimeProcessParameters.squareSizeMM,
			oneTimeProcessParameters.sensorDiagonalSizeMM,
			oneTimeProcessParameters.initialPrincipalPointPixelPosition
		};

		UE_LOG(LogTemp, Log, TEXT("Latching worker."))
		workers[nextWorkerIndex].signalLatch.Execute(latchData);

		/*
		workers.Sort([](const FWorkerInterfaceContainer& workerA, const FWorkerInterfaceContainer& workerB) {
			return workerA.getWorkLoadDel.Execute() > workerB.getWorkLoadDel.Execute();
		});
		*/

		nextWorkerIndex++;
		if (nextWorkerIndex > workers.Num() - 1)
			nextWorkerIndex = 0;
	}

	threadLock.Unlock();
}

void ULensSolver::GenerateDistortionCorrectionMapRenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FDistortionCorrectionMapGenerationParameters distortionCorrectionMapGenerationParams,
	const FString generatedOutputPath)
{
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

	RHICmdList.BeginRenderPass(RPInfo, TEXT("GenerateDistortionCorrectionMapPass"));
	{
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		RHICmdList.SetViewport(0, 0, 0.0f, width, height, 1.0f);
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
		PixelShader->SetParameters(RHICmdList, normalizedPrincipalPoint, distortionCorrectionMapGenerationParams.distortionCoefficients, true);
		FPixelShaderUtils::DrawFullscreenQuad(RHICmdList, 1);
	}
	RHICmdList.EndRenderPass();

	FIntRect rect = FIntRect(0, 0, distortionCorrectionMapGenerationParams.outputMapResolution.X, distortionCorrectionMapGenerationParams.outputMapResolution.Y);

	FRHITexture2D * texture2D = distortionCorrectionRenderTexture->GetTexture2D();
	TUniquePtr<TImagePixelData<FFloat16Color>> pixelData = MakeUnique<TImagePixelData<FFloat16Color>>(rect.Size());

	RHICmdList.ReadSurfaceFloatData(texture2D, rect, pixelData->Pixels, (ECubeFace)0, 0, 0);
	check(pixelData->IsDataWellFormed());

	TArray<FFloat16Color> pixelDataCopy = pixelData->Pixels;
	if (!LensSolverUtilities::WriteTexture16(generatedOutputPath, width, height, MoveTemp(pixelData)))
		return

	// uint32 ExtendXWithMSAA = surfaceData.Num() / texture2D->GetSizeY();
	// FFileHelper::CreateBitmap(*generatedOutputPath, ExtendXWithMSAA, texture2D->GetSizeY(), surfaceData.GetData());
	// FFileHelper::CreateBitmap(*generatedOutputPath, texture2D->GetSizeX(), texture2D->GetSizeY(), surfaceData.GetData());
	UE_LOG(LogTemp, Log, TEXT("Wrote distortion correction map to path: \"%s\"."), *generatedOutputPath);

	if (!queuedDistortionCorrectionMapResults.IsValid())
		return;

	FDistortionCorrectionMapGenerationResults distortionCorrectionMapGenerationResults;

	distortionCorrectionMapGenerationResults.pixels = pixelDataCopy;
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

/*
void ULensSolver::VisualizeCalibration(
	FRHICommandListImmediate& RHICmdList, 
	FSceneViewport* sceneViewport, 
	UTexture2D * texture, 
	FCalibrationResult solvedPoints,
	bool flipX,
	bool flipY)
{
	if (texture == nullptr)
		return;

	FTextureRHIRef visualizationTextureRHIRef = texture->Resource->TextureRHI;

	FTexture2DRHIRef viewportTexture2DRHIRef = sceneViewport->GetRenderTargetTexture();
	int width = viewportTexture2DRHIRef->GetSizeX();
	int height = viewportTexture2DRHIRef->GetSizeY();

	FRHIRenderPassInfo RPInfo(viewportTexture2DRHIRef, ERenderTargetActions::Clear_Store);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("VisualizationCalibrationPass"));
	{
		const ERHIFeatureLevel::Type RenderFeatureLevel = GMaxRHIFeatureLevel;
		const auto GlobalShaderMap = GetGlobalShaderMap(RenderFeatureLevel);

		TShaderMapRef<FBlitShaderVS> VertexShader(GlobalShaderMap);
		TShaderMapRef<FBlitShaderPS> PixelShader(GlobalShaderMap);

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
		PixelShader->SetParameters(RHICmdList, visualizationTextureRHIRef, FVector2D(flipX ? -1 : 1, flipY ? -1 : 1));

		FPixelShaderUtils::DrawFullscreenQuad(RHICmdList, 1);
	}

	// sceneViewport->EndRenderFrame(RHICmdList, true, false);
	RHICmdList.EndRenderPass();
	sceneViewport->Draw(true);

	// solvedPoints.texture->ReleaseResource();
	// texture->ConditionalBeginDestroy();
}
*/

bool ULensSolver::ValidateZoom(const FJobInfo& jobInfo, const float zoomValue)
{
	if (zoomValue < 0.0f || zoomValue > 1.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("Input zoom value: %d is outside of expected normalized range between 0 and 1."), zoomValue);
		return false;
	}
	return true;
}

bool ULensSolver::ValidateTexture(const FJobInfo & jobInfo, const UTexture2D* inputTexture, const int textureIndex, const FIntPoint targetResolution)
{
	if (inputTexture == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot process null texture."));
		ReturnErrorSolvedPoints(jobInfo);
		return false;
	}

	if (inputTexture->GetSizeX() <= 3 ||
		inputTexture->GetSizeY() <= 3)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot process texture, it's to small."));
		ReturnErrorSolvedPoints(jobInfo);
		return false;
	}

	if (inputTexture->GetSizeX() != targetResolution.X || inputTexture->GetSizeY() != targetResolution.Y)
	{
		UE_LOG(LogTemp, Error, TEXT("The texture at index: %d has a different resolution: (%d, %d) from the resolution: (%d, %d) of the first image in the job."),
			textureIndex,
			inputTexture->GetSizeX(),
			inputTexture->GetSizeY(),
			targetResolution.X,
			targetResolution.Y);

		ReturnErrorSolvedPoints(jobInfo);
		return false;
	}

	return true;
}

bool ULensSolver::ValidateMediaTexture(const FJobInfo& jobInfo, const UMediaTexture* inputTexture)
{
	if (inputTexture == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot process null texture."));
		ReturnErrorSolvedPoints(jobInfo);
		return false;
	}

	if (inputTexture->GetWidth() <= 3 ||
		inputTexture->GetHeight() <= 3)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot process texture, it's to small."));
		ReturnErrorSolvedPoints(jobInfo);
		return false;
	}

	return true;
}

bool ULensSolver::ValidateOneTimeProcessParameters(const FOneTimeProcessParameters& oneTimeProcessParameters)
{
	static FString validationHeader("Incorrect OneTimeProcessParameter member values:");
	FString outputMessage;

	bool valid = true;
	if (oneTimeProcessParameters.cornerCount.X <= 0 || oneTimeProcessParameters.cornerCount.Y <= 0)
	{
		outputMessage = FString::Printf(TEXT("%s\n\tCorner Count - Must be positive X & Y integer that represents the number of inside corners across width an height of a checkerboard."), *validationHeader);
		valid = false;
	}

	if (oneTimeProcessParameters.squareSizeMM <= 0.0f)
	{
		outputMessage = FString::Printf(TEXT("%s\n\tSquare Size - Must be positive decimal value that represents the size of the a single checkerboard square in millimeters."), *validationHeader);
		valid = false;
	}

	if (oneTimeProcessParameters.resize && oneTimeProcessParameters.resizePercentage <= 0.0f)
	{
		outputMessage = FString::Printf(TEXT("%s\n\tResize Resolution - Must be positive decimal number to resize the image to."), *validationHeader);
		valid = false;
	}

	if (!valid)
		UE_LOG(LogTemp, Error, TEXT("%s"), *outputMessage);

	return valid;
}

void ULensSolver::ReturnErrorSolvedPoints(FJobInfo jobInfo)
{
	FCalibrationResult solvedPoints;
	solvedPoints.jobInfo = jobInfo;
	solvedPoints.success = false;
	solvedPoints.focalLengthMM = 0;
	solvedPoints.fovX = 0;
	solvedPoints.fovY = 0;
	solvedPoints.aspectRatio = 0;
	solvedPoints.perspectiveMatrix = FMatrix::Identity;
	solvedPoints.resolution = FIntPoint(0, 0);
	solvedPoints.sensorSizeMM = FVector2D(0.0f, 0.0f);
	solvedPoints.zoomLevel = 0;

	this->DequeueSolvedPoints(solvedPoints);
}

void ULensSolver::PollCalibrationResults()
{
	if (!queuedSolvedPointsPtr.IsValid())
		return;

	bool isQueued = queuedSolvedPointsPtr->IsEmpty() == false;
	while (isQueued)
	{
		FCalibrationResult lastSolvedPoints;
		if (!queuedSolvedPointsPtr.IsValid())
			return;

		if (!queuedSolvedPointsPtr->Peek(lastSolvedPoints))
			return;

		if (!jobs.Contains(lastSolvedPoints.jobInfo.jobID))
		{
			UE_LOG(LogTemp, Warning, TEXT("Deqeued work unit result for job: \"%s\" that has not been registered."), *lastSolvedPoints.jobInfo.jobID);
			return;
		}

		queuedSolvedPointsPtr->Pop();

		UE_LOG(LogTemp, Log, TEXT("Dequeued solved points."));

		/*
		if (!jobs.Contains(lastSolvedPoints.jobInfo.jobID))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Deqeued work unit result for job: \"%s\" that has not been registered."), *lastSolvedPoints.jobInfo.jobID);
			return;
		}
		*/

		/*
		if (this == nullptr)
			return;
		*/

		this->DequeueSolvedPoints(lastSolvedPoints);
		isQueued = queuedSolvedPointsPtr->IsEmpty() == false;

		FJob *job = jobs.Find(lastSolvedPoints.jobInfo.jobID);
		job->completedWorkUnits++;

		if (job->completedWorkUnits >= lastSolvedPoints.jobInfo.workUnitCount)
		{
			UE_LOG(LogTemp, Log, TEXT("Completed job: \"%s\", job will be unregistered."), *lastSolvedPoints.jobInfo.jobID);
			this->FinishedJob(lastSolvedPoints.jobInfo);
			jobs.Remove(lastSolvedPoints.jobInfo.jobID);
		}
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
		if (!LensSolverUtilities::CreateTexture2D((void*)distortionCorrectionMapResult.pixels.GetData(), distortionCorrectionMapResult.width, distortionCorrectionMapResult.height, false, true, output, PF_FloatRGBA))
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

bool ULensSolver::ValidateMediaInputs(UMediaPlayer* mediaPlayer, UMediaTexture* mediaTexture, FString url)
{
	return
		mediaTexture != nullptr &&
		mediaPlayer != nullptr &&
		!url.IsEmpty();
}

/*
void ULensSolver::OneTimeProcessMediaTexture(
		UMediaTexture* inputMediaTexture,
		float normalizedZoomValue,
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo)
{
	if (!queuedSolvedPointsPtr.IsValid())
		queuedSolvedPointsPtr = MakeShareable(new TQueue<FCalibrationResult>);

	ouptutJobInfo = RegisterJob(1, UJobType::OneTime);
	oneTimeProcessParameters.currentResolution = FIntPoint(inputMediaTexture->GetWidth(), inputMediaTexture->GetHeight());
	BeginDetectPoints(ouptutJobInfo, inputMediaTexture, normalizedZoomValue, oneTimeProcessParameters);
}
*/

void ULensSolver::OneTimeProcessTextureZoomPair(
		FTextureZoomPair textureZoomPair,
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo)
{
	if (!queuedSolvedPointsPtr.IsValid())
		queuedSolvedPointsPtr = MakeShareable(new TQueue<FCalibrationResult>);

	ouptutJobInfo = RegisterJob(1, UJobType::OneTime);
	oneTimeProcessParameters.currentResolution = FIntPoint(textureZoomPair.texture->GetSizeX(), textureZoomPair.texture->GetSizeY());
	BeginDetectPoints(
		ouptutJobInfo, 
		textureZoomPair, 
		oneTimeProcessParameters,
		true);
}

void ULensSolver::OneTimeProcessArrayOfTextureZoomPairs(
	TArray<FTextureZoomPair> textureZoomPairArray,
	FOneTimeProcessParameters oneTimeProcessParameters,
	FJobInfo & ouptutJobInfo)
{
	if (!queuedSolvedPointsPtr.IsValid())
		queuedSolvedPointsPtr = MakeShareable(new TQueue<FCalibrationResult>);

	ouptutJobInfo = RegisterJob(textureZoomPairArray.Num(), UJobType::OneTime);
	BeginDetectPoints(
		ouptutJobInfo,
		textureZoomPairArray,
		oneTimeProcessParameters);
}

void ULensSolver::OneTimeProcessTextureArrayZoomPair(
	FTextureArrayZoomPair inputTextures,
	FOneTimeProcessParameters oneTimeProcessParameters,
	FJobInfo& ouptutJobInfo)
{
	if (!queuedSolvedPointsPtr.IsValid())
		queuedSolvedPointsPtr = MakeShareable(new TQueue<FCalibrationResult>);

	ouptutJobInfo = RegisterJob(1, UJobType::OneTime);
	BeginDetectPoints(
		ouptutJobInfo,
		inputTextures,
		oneTimeProcessParameters
	);
}

void ULensSolver::OneTimeProcessArrayOfTextureArrayZoomPairs(
		TArray<FTextureArrayZoomPair> inputTextures, 
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo)
{
	if (!queuedSolvedPointsPtr.IsValid())
		queuedSolvedPointsPtr = MakeShareable(new TQueue<FCalibrationResult>);

	int useCount = 0;
	for (int i = 0; i < inputTextures.Num(); i++)
		if (inputTextures[i].use)
			useCount++;

	if (useCount == 0)
		return;

	ouptutJobInfo = RegisterJob(useCount, UJobType::OneTime);
	BeginDetectPoints(
		ouptutJobInfo,
		inputTextures,
		oneTimeProcessParameters
	);
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

	TArray<FTextureArrayZoomPair> textureArrayZoomPairs;
	for (int ti = 0; ti < inputTextures.Num(); ti++)
	{
		TArray<FString> imagesInDirectory;
		TArray<UTexture2D*> textures;

		if (!inputTextures[ti].use)
			continue;

		if (!LensSolverUtilities::GetFilesInFolder(inputTextures[ti].absoluteFolderPath, imagesInDirectory))
			return;

		if (imagesInDirectory.Num() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("No textures in directory: \"%s\", canceled job."), *inputTextures[ti].absoluteFolderPath);
			return;
		}

		for (int i = 0; i < imagesInDirectory.Num(); i++)
		{
			UTexture2D* loadedTexture = nullptr;
			if (!LensSolverUtilities::LoadTexture(imagesInDirectory[i], true, false, loadedTexture))
			{
				UE_LOG(LogTemp, Warning, TEXT("Unable load image: \"%s\", moving on to the next image."), *imagesInDirectory[i]);
				continue;
			}

			textures.Add(loadedTexture);
		}

		FTextureArrayZoomPair textureArrayZoomPair;
		textureArrayZoomPair.textures = textures;
		textureArrayZoomPair.zoomLevel = inputTextures[ti].zoomLevel;
		textureArrayZoomPair.use = true;

		textureArrayZoomPairs.Add(textureArrayZoomPair);
	}

	OneTimeProcessArrayOfTextureArrayZoomPairs(textureArrayZoomPairs, oneTimeProcessParameters, ouptutJobInfo);
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
	FString targetOutputPath = distortionCorrectionMapGenerationParams.outputPath;

	if (!LensSolverUtilities::ValidatePath(targetOutputPath, backupOutputPath, FString("DistortionCorrectionMap"), FString("png"), FString("Distortion Correction: ")))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot generate distortion correction map, unable to create folder path: \"%s\"."), *targetOutputPath);
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
		[lensSolver, temp, targetOutputPath](FRHICommandListImmediate& RHICmdList)
		{
			lensSolver->GenerateDistortionCorrectionMapRenderThread(
				RHICmdList,
				temp,
				targetOutputPath);
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

/*
void ULensSolver::OneTimeProcessMediaTextureArray(
		TArray<UMediaTexture*> inputTextures, 
		TArray<float> normalizedZoomValues, 
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo)
{
	if (!queuedSolvedPointsPtr.IsValid())
		queuedSolvedPointsPtr = MakeShareable(new TQueue<FCalibrationResult>);

	ouptutJobInfo = RegisterJob(inputTextures.Num(), UJobType::OneTime);
	BeginDetectPoints(
		ouptutJobInfo,
		inputTextures,
		normalizedZoomValues,
		oneTimeProcessParameters);
}
*/

void ULensSolver::StartBackgroundImageProcessors(int workerCount)
{
	if (workerCount <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Start Background Image Processors was called with 0 requested workers."));
		return;
	}

	threadLock.Lock();
	onSolvePointsDel.BindUObject(this, &ULensSolver::OnSolvedPoints);
	for (int i = 0; i < workerCount; i++)
	{
		FWorkerInterfaceContainer workerInterfaceContainer;

		UE_LOG(LogTemp, Log, TEXT("Starting lens solver worker: %d"), i);

		workerInterfaceContainer.worker = new FAutoDeleteAsyncTask<FLensSolverWorker>(
			&workerInterfaceContainer.isClosingDel,
			&workerInterfaceContainer.getWorkLoadDel, 
			&workerInterfaceContainer.queueWorkUnitDel,
			&workerInterfaceContainer.signalLatch,
			onSolvePointsDel,
			i);

		workerInterfaceContainer.worker->StartBackgroundTask();
		workers.Add(workerInterfaceContainer);
	}

	threadLock.Unlock();
}

void ULensSolver::StopBackgroundImageprocessors()
{
	threadLock.Lock();
	if (workers.Num() > 0)
	{
		for (int i = 0; i < workers.Num(); i++)
		{
			if (workers[i].isClosingDel.IsBound())
			{
				workers[i].isClosingDel.Execute();
				workers[i].isClosingDel.Unbind();
			}

			if (workers[i].getWorkLoadDel.IsBound())
				workers[i].getWorkLoadDel.Unbind();

			if (workers[i].queueWorkUnitDel.IsBound())
				workers[i].queueWorkUnitDel.Unbind();
		}

		workers.Empty();
	}

	if (queuedSolvedPointsPtr.IsValid())
		queuedSolvedPointsPtr->Empty();
	threadLock.Unlock();
}

/*
void ULensSolver::SolvedPointsQueued_Implemenation (bool& isQueued)
{
}

void ULensSolver::DequeueSolvedPoints_Implemenation (FCalibrationResult& solvedPoints)
{
}
*/

void ULensSolver::Poll()
{
	PollCalibrationResults();
	PollDistortionCorrectionMapGenerationResults();
	PollCorrectedDistortedImageResults();

	/*
	if (this == nullptr)
		return;
	*/
	/*
	if (dequeued)
	{
		if (GEngine != nullptr && GEngine->GameViewport != nullptr)
		{
			FSceneViewport * sceneViewport = GEngine->GameViewport->GetGameViewport();
			GEngine->GameViewport->bDisableWorldRendering = 1;
			sceneViewport->SetGameRenderingEnabled(false);

			UTexture2D * texture = CreateTexture2D(&lastSolvedPoints.visualizationData, lastSolvedPoints.width, lastSolvedPoints.height);
			lastSolvedPoints.visualizationData.Empty();

			if (sceneViewport != nullptr)
			{
				ULensSolver * lensSolver = this;
				ENQUEUE_RENDER_COMMAND(VisualizeCalibration)
				(
					[lensSolver, sceneViewport, texture, lastSolvedPoints](FRHICommandListImmediate& RHICmdList)
					{
						lensSolver->VisualizeCalibration(RHICmdList, sceneViewport, texture, lastSolvedPoints);
					}
				);
			}
		}
	}
	*/
}

void ULensSolver::OnSolvedPoints(FCalibrationResult solvedPoints)
{
	if (!queuedSolvedPointsPtr.IsValid())
		return;

	queuedSolvedPointsPtr->Enqueue(solvedPoints);
}
