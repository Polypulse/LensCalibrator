#include "LensSolver.h"

#include <vector.h>

#include <Runtime\Engine\Classes\Engine\Texture.h>
#include <Runtime\Engine\Classes\Engine\Texture2D.h>
#include "TextureResource.h"

#include "CoreTypes.h"
#include "GlobalShader.h"
#include "RHIStaticStates.h"
#include "Engine/RendererSettings.h"
#include "PixelShaderUtils.h"
// #include "GameViewportClient.h"
#include "SceneViewport.h"
#include "FileHelper.h"
// #include "SceneRenderTargets.h"

#include "BlitShader.h"

void ULensSolver::FireWorkers()
{
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

	queuedSolvedPoints.Empty();
}

void ULensSolver::BeginPlay()
{
	Super::BeginPlay();

	onSolvePointsDel.BindUObject(this, &ULensSolver::OnSolvedPoints);
	for (int i = 0; i < 12; i++)
	{
		FWorkerInterfaceContainer workerInterfaceContainer;

		UE_LOG(LogTemp, Log, TEXT("Starting lens solver worker: %d"), i);

		workerInterfaceContainer.worker = new FAutoDeleteAsyncTask<FLensSolverWorker>(
			&workerInterfaceContainer.isClosingDel,
			&workerInterfaceContainer.getWorkLoadDel, 
			&workerInterfaceContainer.queueWorkUnitDel,
			onSolvePointsDel,
			i);

		workerInterfaceContainer.worker->StartBackgroundTask();
		workers.Add(workerInterfaceContainer);
	}

	visualizationTexture = nullptr;
}

void ULensSolver::EndPlay(const EEndPlayReason::Type EndPlayReason) 
{
	Super::EndPlay(EndPlayReason);
	FireWorkers();
}

void ULensSolver::BeginDetectPoints(UTexture2D* inputTexture, float inputZoomLevel, FIntPoint cornerCount, TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints)
{
	if (inputTexture == nullptr ||
		inputTexture->GetSizeX() <= 2 ||
		inputTexture->GetSizeY() <= 2)
		return;

	UTexture2D* cachedTextureReference = inputTexture;
	TSharedPtr<TQueue<FSolvedPoints>> queuedSolvedPoints = inputQueuedSolvedPoints;
	float zoomLevel = inputZoomLevel;
	int width = inputTexture->GetSizeX();
	int height = inputTexture->GetSizeY();

	ULensSolver * lensSolver = this;
	ENQUEUE_RENDER_COMMAND(ProcessMediaTexture)
	(
		[lensSolver, inputTexture, width, height, zoomLevel, cornerCount, queuedSolvedPoints](FRHICommandListImmediate& RHICmdList)
		{
			lensSolver->DetectPointsRenderThread(RHICmdList, inputTexture, width, height, zoomLevel, cornerCount, queuedSolvedPoints);
		}
	);

	// GEngine->GameViewport->Viewport->Draw();
}

void ULensSolver::BeginDetectPoints(UMediaTexture* inputMediaTexture, float inputZoomLevel, FIntPoint cornerCount, TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints)
{
	if (inputMediaTexture == nullptr ||
		inputMediaTexture->GetWidth() <= 2 ||
		inputMediaTexture->GetWidth() <= 2)
		return;

	UMediaTexture* cachedMediaTextureReference = inputMediaTexture;
	TSharedPtr<TQueue<FSolvedPoints>> queuedSolvedPoints = inputQueuedSolvedPoints;
	float zoomLevel = inputZoomLevel;
	int width = inputMediaTexture->GetWidth();
	int height = inputMediaTexture->GetHeight();

	ULensSolver * lensSolver = this;
	ENQUEUE_RENDER_COMMAND(ProcessMediaTexture)
	(
		[lensSolver, cachedMediaTextureReference, width, height, zoomLevel, cornerCount, queuedSolvedPoints](FRHICommandListImmediate& RHICmdList)
		{
			lensSolver->DetectPointsRenderThread(RHICmdList, cachedMediaTextureReference, width, height, zoomLevel, cornerCount, queuedSolvedPoints);
		}
	);

	// GEngine->GameViewport->Viewport->Draw();
}

void ULensSolver::BeginDetectPoints(TArray<UTexture2D*> inputTextures, TArray<float> inputZoomLevels, FIntPoint cornerCount, TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints)
{
	if (inputTextures.Num() == 0 || inputZoomLevels.Num() == 0)
		return;

	if (inputTextures.Num() != inputZoomLevels.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("The texture and zoom level count does not match!"));
		return;
	}

	TSharedPtr<TQueue<FSolvedPoints>> queuedSolvedPoints = inputQueuedSolvedPoints;
	for (int i = 0; i < inputTextures.Num(); i++)
	{
		if (inputTextures[i] == nullptr ||
			inputTextures[i]->GetSizeX() <= 2 ||
			inputTextures[i]->GetSizeY() <= 2)
		{
			UE_LOG(LogTemp, Error, TEXT("The input texture at index: %d is null!"), i);
			return;
		}

		UTexture2D* cachedTextureReference = inputTextures[i];
		float zoomLevel = inputZoomLevels[i];
		int width = inputTextures[i]->GetSizeX();
		int height = inputTextures[i]->GetSizeY();

		ULensSolver * lensSolver = this;
		ENQUEUE_RENDER_COMMAND(ProcessMediaTexture)
		(
			[lensSolver, cachedTextureReference, width, height, zoomLevel, cornerCount, queuedSolvedPoints](FRHICommandListImmediate& RHICmdList)
			{
				lensSolver->DetectPointsRenderThread(RHICmdList, cachedTextureReference, width, height, zoomLevel, cornerCount, queuedSolvedPoints);
			}
		);
	}

	// GEngine->GameViewport->Viewport->Draw();
}

void ULensSolver::DetectPointsRenderThread(FRHICommandListImmediate& RHICmdList, UTexture* texture, int width, int height, float zoomLevel, FIntPoint cornerCount,TSharedPtr<TQueue<FSolvedPoints>> queuedSolvedPoints)
{
	if (!allocated)
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
			renderTexture,
			dummyTexRef);

		allocated = true;
	}

	FRHIRenderPassInfo RPInfo(renderTexture, ERenderTargetActions::Clear_Store);
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
		PixelShader->SetParameters(RHICmdList, texture->TextureReference.TextureReferenceRHI.GetReference());

		FPixelShaderUtils::DrawFullscreenQuad(RHICmdList, 1);
	}

	RHICmdList.EndRenderPass();

	FRHITexture2D * texture2D = renderTexture->GetTexture2D();
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

	if (workers.Num() == 0)
		return;

	workers.Sort([](const FWorkerInterfaceContainer& workerA, const FWorkerInterfaceContainer& workerB) {
		return workerA.getWorkLoadDel.Execute() < workerB.getWorkLoadDel.Execute();
	});

	FLensSolverWorkUnit workerUnit;
	workerUnit.width = width;
	workerUnit.height = height;
	workerUnit.cornerCount = cornerCount;
	workerUnit.zoomLevel = zoomLevel;
	workerUnit.pixels = surfaceData;

	workers[0].queueWorkUnitDel.Execute(workerUnit);
}

UTexture2D * ULensSolver::CreateTexture2D(TArray<FColor> * rawData, int width, int height)
{
	if (visualizationTexture == nullptr || visualizationTexture->GetSizeX() != width || visualizationTexture->GetSizeY() != height)
	{
		visualizationTexture = UTexture2D::CreateTransient(width, height, EPixelFormat::PF_B8G8R8A8);
		if (visualizationTexture == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("Unable to create transient texture"));
			return nullptr;
		}

		visualizationTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		visualizationTexture->PlatformData->Mips[0].SizeX = width;
		visualizationTexture->PlatformData->Mips[0].SizeY = height;
		visualizationTexture->PlatformData->Mips[0].BulkData.Realloc(rawData->Num());
		visualizationTexture->PlatformData->Mips[0].BulkData.Unlock();
	}

	uint8 * textureData = (uint8*)visualizationTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);

	if (textureData == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("BulkData.Lock returned nullptr!"));
		return nullptr;
	}
	
	FMemory::Memcpy(textureData, rawData->GetData(), rawData->Num());
	visualizationTexture->PlatformData->Mips[0].BulkData.Unlock();

	// texture->Resource = texture->CreateResource();
	visualizationTexture->UpdateResource();
	visualizationTexture->RefreshSamplerStates();

	return visualizationTexture;
}


void ULensSolver::VisualizeCalibration(FRHICommandListImmediate& RHICmdList, FSceneViewport* sceneViewport, UTexture2D * visualizationTexture, FSolvedPoints solvedPoints)
{
	if (visualizationTexture == nullptr)
		return;

	FTextureRHIRef visualizationTextureRHIRef = visualizationTexture->Resource->TextureRHI;

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
		// PixelShader->SetParameters(RHICmdList, visualizationTextureRHIRef);

		FPixelShaderUtils::DrawFullscreenQuad(RHICmdList, 1);
	}

	// sceneViewport->EndRenderFrame(RHICmdList, true, false);
	RHICmdList.EndRenderPass();
	sceneViewport->Draw(true);

	// solvedPoints.visualizationTexture->ReleaseResource();
	// visualizationTexture->ConditionalBeginDestroy();
}

bool ULensSolver::ValidateMediaInputs(UMediaPlayer* mediaPlayer, UMediaTexture* mediaTexture, FString url)
{
	return
		mediaTexture != nullptr &&
		mediaPlayer != nullptr &&
		!url.IsEmpty();
}

void ULensSolver::ProcessMediaTexture(UMediaTexture* inputMediaTexture, float normalizedZoomValue, FIntPoint cornerCount)
{
	if (!queuedSolvedPointsPtr.IsValid())
		queuedSolvedPointsPtr = TSharedPtr<TQueue<FSolvedPoints>>(&queuedSolvedPoints);
	BeginDetectPoints(inputMediaTexture, normalizedZoomValue, cornerCount, queuedSolvedPointsPtr);
}

void ULensSolver::ProcessTexture2D(UTexture2D* inputTexture, float normalizedZoomValue, FIntPoint cornerCount)
{
	if (!queuedSolvedPointsPtr.IsValid())
		queuedSolvedPointsPtr = TSharedPtr<TQueue<FSolvedPoints>>(&queuedSolvedPoints);
	BeginDetectPoints(inputTexture, normalizedZoomValue, cornerCount, queuedSolvedPointsPtr);
}

void ULensSolver::ProcessTexture2DArray(TArray<UTexture2D*> inputTextures, TArray<float> normalizedZoomValues, FIntPoint cornerCount)
{
}

/*
void ULensSolver::SolvedPointsQueued_Implemenation (bool& isQueued)
{
}

void ULensSolver::DequeueSolvedPoints_Implemenation (FSolvedPoints& solvedPoints)
{
}
*/

void ULensSolver::PollSolvedPoints()
{
	/*
	threadLock.Lock();

	for (int i = 0; i < workers.Num(); i++)
	{
	workers[i].
	}

	threadLock.Unlock();
	*/

	if (this == nullptr)
		return;

	bool isQueued = queuedSolvedPoints.IsEmpty() == false;
	bool outputIsQueued = isQueued;
	this->SolvedPointsQueued(outputIsQueued);

	FSolvedPoints lastSolvedPoints;
	bool dequeued = false;

	while (isQueued)
	{
		if (!queuedSolvedPoints.Dequeue(lastSolvedPoints))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to dequeue solved points."));
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("Dequeued solved points."));
		dequeued = true;

		if (this == nullptr)
			return;

		this->DequeueSolvedPoints(lastSolvedPoints);
		isQueued = queuedSolvedPoints.IsEmpty() == false;
	}

	if (dequeued)
	{
		/*
		if (GEngine != nullptr && GEngine->GameViewport != nullptr)
		{
			FSceneViewport * sceneViewport = GEngine->GameViewport->GetGameViewport();
			GEngine->GameViewport->bDisableWorldRendering = 1;
			sceneViewport->SetGameRenderingEnabled(false);

			UTexture2D * visualizationTexture = CreateTexture2D(&lastSolvedPoints.visualizationData, lastSolvedPoints.width, lastSolvedPoints.height);
			lastSolvedPoints.visualizationData.Empty();

			if (sceneViewport != nullptr)
			{
				ULensSolver * lensSolver = this;
				ENQUEUE_RENDER_COMMAND(VisualizeCalibration)
				(
					[lensSolver, sceneViewport, visualizationTexture, lastSolvedPoints](FRHICommandListImmediate& RHICmdList)
					{
						lensSolver->VisualizeCalibration(RHICmdList, sceneViewport, visualizationTexture, lastSolvedPoints);
					}
				);
			}
		}
		*/
	}
}

void ULensSolver::OnSolvedPoints(FSolvedPoints solvedPoints)
{
	queuedSolvedPoints.Enqueue(solvedPoints);
}
