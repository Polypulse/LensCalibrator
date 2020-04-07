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
	for (int i = 0; i < 8; i++)
	{
		FWorkerInterfaceContainer workerInterfaceContainer;

		workerInterfaceContainer.worker = new FAutoDeleteAsyncTask<FLensSolverWorker>(
			&workerInterfaceContainer.isClosingDel,
			&workerInterfaceContainer.getWorkLoadDel, 
			&workerInterfaceContainer.queueWorkUnitDel,
			onSolvePointsDel);

		workerInterfaceContainer.worker->StartBackgroundTask();
		workers.Add(workerInterfaceContainer);
	}
}

void ULensSolver::EndPlay(const EEndPlayReason::Type EndPlayReason) 
{
	Super::EndPlay(EndPlayReason);
	FireWorkers();
}

void ULensSolver::BeginDetectPoints(UMediaTexture* inputMediaTexture, float inputZoomLevel, TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints)
{
	if (inputMediaTexture == nullptr ||
		inputMediaTexture->GetWidth() <= 2 ||
		inputMediaTexture->GetWidth() <= 2)
		return;

	UMediaTexture* cachedMediaTextureReference = inputMediaTexture;
	TSharedPtr<TQueue<FSolvedPoints>> queuedSolvedPoints = inputQueuedSolvedPoints;
	float zoomLevel = inputZoomLevel;

	ULensSolver * lensSolver = this;

	ENQUEUE_RENDER_COMMAND(ProcessMediaTexture)
	(
		[lensSolver, cachedMediaTextureReference, queuedSolvedPoints, zoomLevel](FRHICommandListImmediate& RHICmdList)
		{
			lensSolver->DetectPointsRenderThread(RHICmdList, cachedMediaTextureReference, queuedSolvedPoints, zoomLevel);
		}
	);
}

void ULensSolver::DetectPointsRenderThread(FRHICommandListImmediate& RHICmdList, UMediaTexture* mediaTexture, TSharedPtr<TQueue<FSolvedPoints>> queuedSolvedPoints, float zoomLevel)
{
	int width = mediaTexture->GetWidth();
	int height = mediaTexture->GetHeight();

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
	RHICmdList.BeginRenderPass(RPInfo, TEXT("CopyMediaTexturePass"));
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
		FTextureRHIRef texture = mediaTexture->TextureReference.TextureReferenceRHI;
		PixelShader->SetParameters(RHICmdList, texture);

		FPixelShaderUtils::DrawFullscreenQuad(RHICmdList, 1);
	}

	RHICmdList.EndRenderPass();

	FRHITexture2D * texture = renderTexture->GetTexture2D();
	TArray<FColor> Bitmap;

	FReadSurfaceDataFlags ReadDataFlags;
	ReadDataFlags.SetLinearToGamma(false);
	ReadDataFlags.SetOutputStencil(false);
	ReadDataFlags.SetMip(0);

	UE_LOG(LogTemp, Log, TEXT("Reading pixels from rect: (%d, %d, %d, %d)."), 0, 0, width, height);
	RHICmdList.ReadSurfaceData(texture, FIntRect(0, 0, width, height), Bitmap, ReadDataFlags);

	// threadLock.Lock();

	workers.Sort([](const FWorkerInterfaceContainer& workerA, const FWorkerInterfaceContainer& workerB) {
		return workerA.getWorkLoadDel.Execute() > workerB.getWorkLoadDel.Execute();
	});

	workers[0].queueWorkUnitDel.Execute(Bitmap, width, height, zoomLevel);

	// threadLock.Unlock();
}

bool ULensSolver::ValidateMediaInputs(UMediaPlayer* mediaPlayer, UMediaTexture* mediaTexture, FString url)
{
	return
		mediaTexture != nullptr &&
		mediaPlayer != nullptr &&
		!url.IsEmpty();
}

void ULensSolver::ProcessMediaTexture(UMediaTexture* inputMediaTexture, float normalizedZoomValue)
{
	if (!queuedSolvedPointsPtr.IsValid())
		queuedSolvedPointsPtr = TSharedPtr<TQueue<FSolvedPoints>>(&queuedSolvedPoints);
	BeginDetectPoints(inputMediaTexture, normalizedZoomValue, queuedSolvedPointsPtr);
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

	bool isQueued = queuedSolvedPoints.IsEmpty() == false;
	bool outputIsQueued = isQueued;
	this->SolvedPointsQueued(outputIsQueued);

	while (isQueued)
	{
		FSolvedPoints solvedPoints;
		if (!queuedSolvedPoints.Dequeue(solvedPoints))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to dequeue solved points."));
			return;
		}

		this->DequeueSolvedPoints(solvedPoints);
		isQueued = queuedSolvedPoints.IsEmpty() == false;
	}
}

void ULensSolver::OnSolvedPoints(FSolvedPoints solvedPoints)
{
	queuedSolvedPoints.Enqueue(solvedPoints);
}
