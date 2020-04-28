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
	const UTexture2D* inputTexture,
	const float inputZoomLevel,
	FOneTimeProcessParameters oneTimeProcessParameters)
{
	if (!ValidateTexture(inputJobInfo, inputTexture))
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

	oneTimeProcessParameters.currentResolution = FIntPoint(inputTexture->GetSizeX(), inputTexture->GetSizeY());

	FJobInfo jobInfo = inputJobInfo;

	const UTexture2D* cachedTextureReference = inputTexture;
	float zoomLevel = inputZoomLevel;
	FOneTimeProcessParameters tempFirstPassParameters = oneTimeProcessParameters;

	UE_LOG(LogTemp, Log, TEXT("Enqueuing calibration image render comand at resolution: (%d, %d)."), oneTimeProcessParameters.currentResolution.X, oneTimeProcessParameters.currentResolution.Y);

	ULensSolver * lensSolver = this;
	ENQUEUE_RENDER_COMMAND(OneTimeProcessMediaTexture)
	(
		[lensSolver, jobInfo, inputTexture, zoomLevel, tempFirstPassParameters](FRHICommandListImmediate& RHICmdList)
		{
			lensSolver->DetectPointsRenderThread(
				RHICmdList,
				jobInfo,
				inputTexture,
				zoomLevel,
				tempFirstPassParameters);
				
		}
	);
}

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

void ULensSolver::BeginDetectPoints(
	const FJobInfo jobInfo,
	TArray<UTexture2D*> inputTextures,
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

void ULensSolver::DetectPointsRenderThread(
	FRHICommandListImmediate& RHICmdList, 
	const FJobInfo jobInfo,
	const UTexture* texture, 
	const float normalizedZoomLevel,
	FOneTimeProcessParameters oneTimeProcessParameters)

{
	int width = oneTimeProcessParameters.resize ? oneTimeProcessParameters.resizeResolution.X : oneTimeProcessParameters.currentResolution.X;
	int height = oneTimeProcessParameters.resize ? oneTimeProcessParameters.resizeResolution.Y : oneTimeProcessParameters.currentResolution.Y;
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
		PixelShader->SetParameters(RHICmdList, texture->TextureReference.TextureReferenceRHI.GetReference(), FVector2D(oneTimeProcessParameters.flipX ? -1.0f : 1.0f, oneTimeProcessParameters.flipY ? -1.0f : 1.0f));

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

	FString textureName;
	texture->GetName(textureName);

	FLensSolverWorkUnit workerUnit;
	workerUnit.unitName = textureName;
	workerUnit.jobInfo = jobInfo;
	workerUnit.workerParameters = oneTimeProcessParameters.workerParameters;
	workerUnit.width = width;
	workerUnit.height = height;
	workerUnit.cornerCount = oneTimeProcessParameters.cornerCount;
	workerUnit.zoomLevel = normalizedZoomLevel;
	workerUnit.squareSize = oneTimeProcessParameters.squareSize;
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
	FCalibrationResult solvedPoints,
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

bool ULensSolver::ValidateZoom(const FJobInfo& jobInfo, const float zoomValue)
{
	if (zoomValue < 0.0f || zoomValue > 1.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("Input zoom value: %d is outside of expected normalized range between 0 and 1."), zoomValue);
		return false;
	}
	return true;
}

bool ULensSolver::ValidateTexture(const FJobInfo & jobInfo, const UTexture2D* inputTexture)
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

	if (oneTimeProcessParameters.squareSize <= 0.0f)
	{
		outputMessage = FString::Printf(TEXT("%s\n\tSquare Size - Must be positive decimal value that represents the size of the a single checkerboard square in millimeters."), *validationHeader);
		valid = false;
	}

	if (oneTimeProcessParameters.resize &&
		(oneTimeProcessParameters.resizeResolution.X <= 0 ||
			oneTimeProcessParameters.resizeResolution.Y <= 0))
	{
		outputMessage = FString::Printf(TEXT("%s\n\tResize Resolution - Must be positive integer width & height values to resize the image to."), *validationHeader);
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
	solvedPoints.focalLength = 0;
	solvedPoints.fovX = 0;
	solvedPoints.fovY = 0;
	solvedPoints.aspectRatio = 0;
	solvedPoints.perspectiveMatrix = FMatrix::Identity;
	solvedPoints.height = 0;
	solvedPoints.width = 0;
	solvedPoints.zoomLevel = 0;

	this->DequeueSolvedPoints(solvedPoints);
}

bool ULensSolver::ValidateMediaInputs(UMediaPlayer* mediaPlayer, UMediaTexture* mediaTexture, FString url)
{
	return
		mediaTexture != nullptr &&
		mediaPlayer != nullptr &&
		!url.IsEmpty();
}

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

void ULensSolver::OneTimeProcessTexture2D(
		UTexture2D* inputTexture, 
		float normalizedZoomValue, 
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo)
{
	if (!queuedSolvedPointsPtr.IsValid())
		queuedSolvedPointsPtr = MakeShareable(new TQueue<FCalibrationResult>);

	ouptutJobInfo = RegisterJob(1, UJobType::OneTime);
	oneTimeProcessParameters.currentResolution = FIntPoint(inputTexture->GetSizeX(), inputTexture->GetSizeY());
	BeginDetectPoints(ouptutJobInfo, inputTexture, normalizedZoomValue, oneTimeProcessParameters);
}

void ULensSolver::OneTimeProcessTexture2DArray(
		TArray<UTexture2D*> inputTextures, 
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

void ULensSolver::PollSolvedPoints()
{
	/*
	if (this == nullptr)
		return;
	*/

	if (!queuedSolvedPointsPtr.IsValid())
		return;

	bool isQueued = queuedSolvedPointsPtr->IsEmpty() == false;
	bool outputIsQueued = isQueued;

	FCalibrationResult lastSolvedPoints;
	bool dequeued = false;

	while (isQueued)
	{
		if (!queuedSolvedPointsPtr.IsValid())
			return;

		if (!queuedSolvedPointsPtr->Dequeue(lastSolvedPoints))
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

void ULensSolver::OnSolvedPoints(FCalibrationResult solvedPoints)
{
	if (!queuedSolvedPointsPtr.IsValid())
		return;

	queuedSolvedPointsPtr->Enqueue(solvedPoints);
}