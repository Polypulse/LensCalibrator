#include "LensSolver.h"

#include <vector.h>

#include <Runtime\Engine\Classes\Engine\Texture.h>
#include <Runtime\Engine\Classes\Engine\Texture2D.h>
#include "TextureResource.h"

#include "GlobalShader.h"
#include "RHIStaticStates.h"
#include "CommonRenderResources.h"
#include "Engine/RendererSettings.h"
#include "PixelShaderUtils.h"

#include "BlitShader.h"

void ULensSolver::BeginDetectPoints(UMediaTexture* inputMediaTexture, float inputZoomLevel, TQueue<FSolvedPoints> * inputQueuedSolvedPoints)
{
	UMediaTexture* cachedMediaTextureReference = inputMediaTexture;
	if (cachedMediaTextureReference->GetWidth() <= 2 ||
		cachedMediaTextureReference->GetWidth() <= 2)
	{
		inputQueuedSolvedPoints->Enqueue(FSolvedPoints({TArray<FVector2D>(), inputZoomLevel, false}));
		return;
	}

	float zoomLevel = inputZoomLevel;
	TQueue<FSolvedPoints> * queuedSolvedPoints = inputQueuedSolvedPoints;

	ENQUEUE_RENDER_COMMAND(ProcessMediaTexture)
	(
		[cachedMediaTextureReference, zoomLevel, queuedSolvedPoints](FRHICommandListImmediate& RHICmdList)
		{
			static FTexture2DRHIRef renderTexture;
			static bool allocated = false;

			int width = cachedMediaTextureReference->GetWidth();
			int height = cachedMediaTextureReference->GetHeight();

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
			RHICmdList.BeginRenderPass(RPInfo, TEXT("VSRBender"));
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
				FTextureRHIRef texture = cachedMediaTextureReference->TextureReference.TextureReferenceRHI;
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

			// FString outputPath("D:\\Test.bmp");
			// FFileHelper::CreateBitmap(*outputPath, ExtendXWithMSAA, texture->GetSizeY(), Bitmap.GetData());
			// UE_LOG(LogTemp, Log, TEXT("Wrote test bitmap with: %d pixels to file."), Bitmap.Num());

			static cv::Mat image(width, height, cv::DataType<unsigned char>::type);
			if (image.cols != width || image.rows != height)
				image = cv::Mat(width, height, cv::DataType<unsigned char>::type);

			for (int i = 0; i < width * height; i++)
				image.at<unsigned char>(cv::Point(i / width, i % width)) = Bitmap[i].R;

			cv::imwrite("D:\\output.jpg", image);

			/*
			cv::Mat gray(width, height, CV_8U);
			cv::cvtColor(image, gray, CV_BGR2GRAY);
			*/

			std::vector<cv::Point2f> corners;
			cv::Size patternSize(9, 6);
			bool patternFound = false;

			try
			{
				patternFound = cv::findChessboardCorners(image, patternSize, corners);
			}

			catch (std::exception e)
			{
				UE_LOG(LogTemp, Log, TEXT("OpenCV exception occurred: %s"), e.what());
				queuedSolvedPoints->Enqueue(FSolvedPoints({TArray<FVector2D>(), zoomLevel, false}));
				return;
			}

			if (!patternFound)
			{
				UE_LOG(LogTemp, Warning, TEXT("No pattern in view."));
				queuedSolvedPoints->Enqueue(FSolvedPoints({TArray<FVector2D>(), zoomLevel, false}));
				return;
			}

			// UE_LOG(LogTemp, Log, TEXT("Chessboard detected."));
			static TArray<FVector2D> pointsCache;
			if (pointsCache.Num() != corners.size())
				pointsCache.SetNum(corners.size());

			for (int i = 0; i < pointsCache.Num(); i++)
				pointsCache[i] = FVector2D(corners[i].x, corners[i].y);

			queuedSolvedPoints->Enqueue(FSolvedPoints({pointsCache, zoomLevel, true}));
		}
	);
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
	BeginDetectPoints(inputMediaTexture, normalizedZoomValue, &queuedSolvedPoints);
}
