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
}

void ULensSolver::BeginPlay()
{
	Super::BeginPlay();
}

void ULensSolver::EndPlay(const EEndPlayReason::Type EndPlayReason) 
{
	Super::EndPlay(EndPlayReason);
	FireWorkers();
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

bool ULensSolver::ValidateCommonVariables(FIntPoint cornerCount, float inputZoomLevel, float inputSquareSize)
{
	if (cornerCount.X <= 0 || cornerCount.Y <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("X or Y corner count is less than or equal to 0: (%d, %d)."), cornerCount.X, cornerCount.Y);
		return false;
	}

	if (inputZoomLevel < 0 || inputZoomLevel > 1)
	{
		UE_LOG(LogTemp, Error, TEXT("The input zoom level: %f is not normalized between 0 and 1."), inputZoomLevel);
		return false;
	}

	if (inputSquareSize <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("The input square size: %f is less than or equal to zero"), inputSquareSize);
		return false;
	}

	return true;
}

void ULensSolver::BeginDetectPoints(
	FJobInfo inputJobInfo,
	UTexture2D* inputTexture, 
	FFirstPassParameters firstPassParameters,
	TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints)
{
	if (inputTexture == nullptr ||
		inputTexture->GetSizeX() <= 2 ||
		inputTexture->GetSizeY() <= 2)
		return;

	if (!ValidateCommonVariables(firstPassParameters.cornerCount, firstPassParameters.zoomLevel, firstPassParameters.squareSize))
		return;

	if (GetWorkerCount() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot process any textures, you need to execute StartBackgroundTextureProcessors in order to process textures."));
		return;
	}

	FJobInfo jobInfo = inputJobInfo;

	UTexture2D* cachedTextureReference = inputTexture;
	TSharedPtr<TQueue<FSolvedPoints>> queuedSolvedPoints = inputQueuedSolvedPoints;

	FFirstPassParameters tempFirstPassParameters = firstPassParameters;

	UE_LOG(LogTemp, Log, TEXT("Enqueuing calibration image render comand at resolution: (%d, %d)."), firstPassParameters.currentResolution.X, firstPassParameters.currentResolution.Y);

	ULensSolver * lensSolver = this;
	ENQUEUE_RENDER_COMMAND(OneTimeProcessMediaTexture)
	(
		[lensSolver, jobInfo, inputTexture, tempFirstPassParameters, queuedSolvedPoints](FRHICommandListImmediate& RHICmdList)
		{
			lensSolver->DetectPointsRenderThread(
				RHICmdList, 
				jobInfo,
				inputTexture, 
				tempFirstPassParameters,
				queuedSolvedPoints);
		}
	);
}

void ULensSolver::BeginDetectPoints(
	FJobInfo inputJobInfo,
	UMediaTexture* inputMediaTexture, 
	FFirstPassParameters firstPassParameters,
	TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints)
{
	if (inputMediaTexture == nullptr ||
		inputMediaTexture->GetWidth() <= 2 ||
		inputMediaTexture->GetWidth() <= 2)
		return;

	if (!ValidateCommonVariables(firstPassParameters.cornerCount, firstPassParameters.zoomLevel, firstPassParameters.squareSize))
		return;

	if (GetWorkerCount() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot process any textures, you need to execute StartBackgroundTextureProcessors in order to process textures."));
		return;
	}

	FJobInfo jobInfo = inputJobInfo;

	UMediaTexture* cachedMediaTextureReference = inputMediaTexture;

	/*
	float zoomLevel = inputZoomLevel;
	float squareSize = inputSquareSize;

	FIntPoint currentResolution = FIntPoint(inputMediaTexture->GetWidth(), inputMediaTexture->GetHeight());
	bool resize = inputResize;
	FIntPoint resizeResolution = inputResizeResolution;
	*/

	TSharedPtr<TQueue<FSolvedPoints>> queuedSolvedPoints = inputQueuedSolvedPoints;

	ULensSolver * lensSolver = this;
	ENQUEUE_RENDER_COMMAND(OneTimeProcessMediaTexture)
	(
		[lensSolver, 
		jobInfo, 
		cachedMediaTextureReference, 
		firstPassParameters,
		queuedSolvedPoints](FRHICommandListImmediate& RHICmdList)
		{
			lensSolver->DetectPointsRenderThread(
				RHICmdList, 
				jobInfo,
				cachedMediaTextureReference, 
				firstPassParameters,
				queuedSolvedPoints);
		}
	);
}

void ULensSolver::BeginDetectPoints(
	FJobInfo jobInfo,
	FSolveParameters solveParameters,
	TArray<UTexture2D*> inputTextures,
	TArray<float> inputZoomLevels,
	FIntPoint cornerCount,
	float inputSquaresize,
	bool resize,
	FIntPoint resizeResolution,
	bool flipX,
	bool flipY,
	TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints)
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
		FFirstPassParameters firstPassParameters;
		firstPassParameters.solveParameters = solveParameters;
		firstPassParameters.zoomLevel = inputZoomLevels[i];
		firstPassParameters.cornerCount = cornerCount;
		firstPassParameters.squareSize = inputSquaresize;
		firstPassParameters.currentResolution = FIntPoint(inputTextures[i]->GetSizeX(), inputTextures[i]->GetSizeY());
		firstPassParameters.resize = resize;
		firstPassParameters.resizeResolution = resizeResolution;
		firstPassParameters.flipDirection = FIntPoint(flipX ? -1 : 1, flipY ? -1 : 1);
		BeginDetectPoints(jobInfo, inputTextures[i], firstPassParameters, inputQueuedSolvedPoints);
	}
}

void ULensSolver::BeginDetectPoints(
	FJobInfo jobInfo,
	FSolveParameters solveParameters,
	TArray<UMediaTexture*> inputTextures, 
	TArray<float> inputZoomLevels, 
	FIntPoint cornerCount, 
	float inputSquaresize, 
	bool resize,
	FIntPoint resizeResolution,
	bool flipX,
	bool flipY,
	TSharedPtr<TQueue<FSolvedPoints>> inputQueuedSolvedPoints)
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
		FFirstPassParameters firstPassParameters;
		firstPassParameters.solveParameters = solveParameters;
		firstPassParameters.zoomLevel = inputZoomLevels[i];
		firstPassParameters.cornerCount = cornerCount;
		firstPassParameters.squareSize = inputSquaresize;
		firstPassParameters.currentResolution = FIntPoint(inputTextures[i]->GetWidth(), inputTextures[i]->GetHeight());
		firstPassParameters.resize = resize;
		firstPassParameters.resizeResolution = resizeResolution;
		firstPassParameters.flipDirection = FIntPoint(flipX ? -1 : 1, flipY ? -1 : 1);
		BeginDetectPoints(jobInfo, inputTextures[i], firstPassParameters, inputQueuedSolvedPoints);
	}
}

void ULensSolver::DetectPointsRenderThread(
	FRHICommandListImmediate& RHICmdList, 
	FJobInfo jobInfo,
	UTexture* texture, 
	FFirstPassParameters firstPassParameters,
	TSharedPtr<TQueue<FSolvedPoints>> queuedSolvedPoints)

{
	int width = firstPassParameters.resize ? firstPassParameters.resizeResolution.X : firstPassParameters.currentResolution.X;
	int height = firstPassParameters.resize ? firstPassParameters.resizeResolution.Y : firstPassParameters.currentResolution.Y;
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
		PixelShader->SetParameters(RHICmdList, texture->TextureReference.TextureReferenceRHI.GetReference(), FVector2D(firstPassParameters.flipDirection.X, firstPassParameters.flipDirection.Y));

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

	threadLock.Lock();
	if (workers.Num() == 0)
		return;

	workers.Sort([](const FWorkerInterfaceContainer& workerA, const FWorkerInterfaceContainer& workerB) {
		return workerA.getWorkLoadDel.Execute() > workerB.getWorkLoadDel.Execute();
	});

	FLensSolverWorkUnit workerUnit;
	workerUnit.jobInfo = jobInfo;
	workerUnit.solveParameters = firstPassParameters.solveParameters;
	workerUnit.width = width;
	workerUnit.height = height;
	workerUnit.cornerCount = firstPassParameters.cornerCount;
	workerUnit.zoomLevel = firstPassParameters.zoomLevel;
	workerUnit.squareSize = firstPassParameters.squareSize;
	workerUnit.pixels = surfaceData;

	workers[0].queueWorkUnitDel.Execute(workerUnit);
	threadLock.Unlock();
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


void ULensSolver::VisualizeCalibration(
	FRHICommandListImmediate& RHICmdList, 
	FSceneViewport* sceneViewport, 
	UTexture2D * visualizationTexture, 
	FSolvedPoints solvedPoints,
	bool flipX,
	bool flipY)
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
		PixelShader->SetParameters(RHICmdList, visualizationTextureRHIRef, FVector2D(flipX ? -1 : 1, flipY ? -1 : 1));

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

FJobInfo ULensSolver::OneTimeProcessMediaTexture(
		FSolveParameters inputSolveParameters,
		UMediaTexture* inputMediaTexture,
		float normalizedZoomValue,
		FIntPoint cornerCount,
		float squareSize,
		bool resize,
		FIntPoint resizeResolution,
		bool flipX,
		bool flipY)
{
	if (!queuedSolvedPointsPtr.IsValid())
		queuedSolvedPointsPtr = TSharedPtr<TQueue<FSolvedPoints>>(&queuedSolvedPoints);

	FJobInfo jobInfo = RegisterJob(1, UJobType::OneTime);
	FFirstPassParameters firstPassParameters;
	firstPassParameters.solveParameters = inputSolveParameters;
	firstPassParameters.zoomLevel = normalizedZoomValue;
	firstPassParameters.cornerCount = cornerCount;
	firstPassParameters.squareSize = squareSize;
	firstPassParameters.currentResolution = FIntPoint(inputMediaTexture->GetWidth(), inputMediaTexture->GetHeight());
	firstPassParameters.resize = resize;
	firstPassParameters.resizeResolution = resizeResolution;
	firstPassParameters.flipDirection = FIntPoint(flipX ? -1 : 1, flipY ? -1 : 1);
	BeginDetectPoints(jobInfo, inputMediaTexture, firstPassParameters, queuedSolvedPointsPtr);
	return jobInfo;
}

FJobInfo ULensSolver::OneTimeProcessTexture2D(
		FSolveParameters inputSolveParameters,
		UTexture2D* inputTexture, 
		float normalizedZoomValue, 
		FIntPoint cornerCount, 
		float squareSize,
		bool resize,
		FIntPoint resizeResolution,
		bool flipX,
		bool flipY)
{
	if (!queuedSolvedPointsPtr.IsValid())
		queuedSolvedPointsPtr = TSharedPtr<TQueue<FSolvedPoints>>(&queuedSolvedPoints);

	FJobInfo jobInfo = RegisterJob(1, UJobType::OneTime);
	FFirstPassParameters firstPassParameters;
	firstPassParameters.solveParameters = inputSolveParameters;
	firstPassParameters.zoomLevel = normalizedZoomValue;
	firstPassParameters.cornerCount = cornerCount;
	firstPassParameters.squareSize = squareSize;
	firstPassParameters.currentResolution = FIntPoint(inputTexture->GetSizeX(), inputTexture->GetSizeY());
	firstPassParameters.resize = resize;
	firstPassParameters.resizeResolution = resizeResolution;
	firstPassParameters.flipDirection = FIntPoint(flipX ? -1 : 1, flipY ? -1 : 1);
	BeginDetectPoints(jobInfo, inputTexture, firstPassParameters, queuedSolvedPointsPtr);
	return jobInfo;
}

FJobInfo ULensSolver::OneTimeProcessTexture2DArray(
		FSolveParameters inputSolveParameters,
		TArray<UTexture2D*> inputTextures, 
		TArray<float> normalizedZoomValues, 
		FIntPoint cornerCount, 
		float squareSize,
		bool resize,
		FIntPoint resizeResolution,
		bool flipX,
		bool flipY)
{
	if (!queuedSolvedPointsPtr.IsValid())
		queuedSolvedPointsPtr = TSharedPtr<TQueue<FSolvedPoints>>(&queuedSolvedPoints);

	FJobInfo jobInfo = RegisterJob(inputTextures.Num(), UJobType::OneTime);
	BeginDetectPoints(
		jobInfo, 
		inputSolveParameters,
		inputTextures, 
		normalizedZoomValues, 
		cornerCount, 
		squareSize, 
		resize,
		resizeResolution, 
		flipX,
		flipY,
		queuedSolvedPointsPtr);
	return jobInfo;
}

FJobInfo ULensSolver::OneTimeProcessMediaTextureArray(
		FSolveParameters inputSolveParameters,
		TArray<UMediaTexture*> inputTextures, 
		TArray<float> normalizedZoomValues, 
		FIntPoint cornerCount, 
		float squareSize,
		bool resize,
		FIntPoint resizeResolution,
		bool flipX,
		bool flipY)
{
	if (!queuedSolvedPointsPtr.IsValid())
		queuedSolvedPointsPtr = TSharedPtr<TQueue<FSolvedPoints>>(&queuedSolvedPoints);

	FJobInfo jobInfo = RegisterJob(inputTextures.Num(), UJobType::OneTime);
	BeginDetectPoints(
		jobInfo,
		inputSolveParameters,
		inputTextures,
		normalizedZoomValues,
		cornerCount,
		squareSize,
		resize,
		resizeResolution,
		flipX,
		flipY,
		queuedSolvedPointsPtr);
	return jobInfo;
}

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
			onSolvePointsDel,
			i);

		workerInterfaceContainer.worker->StartBackgroundTask();
		workers.Add(workerInterfaceContainer);
	}

	visualizationTexture = nullptr;
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

	queuedSolvedPoints.Empty();
	threadLock.Unlock();
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
	if (this == nullptr)
		return;
	*/

	bool isQueued = queuedSolvedPoints.IsEmpty() == false;
	bool outputIsQueued = isQueued;

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

		if (!jobs.Contains(lastSolvedPoints.jobInfo.jobID))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Deqeued work unit result for job: \"%s\" that has not been registered."), *lastSolvedPoints.jobInfo.jobID);
			return;
		}

		/*
		if (this == nullptr)
			return;
		*/

		this->DequeueSolvedPoints(lastSolvedPoints);
		isQueued = queuedSolvedPoints.IsEmpty() == false;

		FJob *job = jobs.Find(lastSolvedPoints.jobInfo.jobID);
		job->completedWorkUnits++;

		if (job->completedWorkUnits >= lastSolvedPoints.jobInfo.workUnitCount)
		{
			UE_LOG(LogTemp, Log, TEXT("Completed job: \"%s\", job will be unregistered."), *lastSolvedPoints.jobInfo.jobID);
			this->FinishedJob(lastSolvedPoints.jobInfo);
			jobs.Remove(lastSolvedPoints.jobInfo.jobID);
		}
		
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
