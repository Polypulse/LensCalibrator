/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once

#include "CoreMinimal.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "RenderResource.h"
#include "ShaderParameters.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "SceneRenderTargetParameters.h"

class FBlitShaderVS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FBlitShaderVS);

public:
	FBlitShaderVS() {}

	FBlitShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
	{
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	template<typename TShaderRHIParamRef>
	void SetParameters(FRHICommandList& RHICmdList, const TShaderRHIParamRef ShaderRHI, const FGlobalShaderPermutationParameters& ShaderInputData)
	{
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		return FGlobalShader::Serialize(Ar);
	}
};

class FBlitShaderPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FBlitShaderPS);

private:
	FShaderResourceParameter InputTextureParameter;
	FShaderResourceParameter InputTextureSamplerParameter;
	FShaderParameter flipDirectionParameter;

public:
	FBlitShaderPS() {}

	FBlitShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
	{
		InputTextureParameter.Bind(Initializer.ParameterMap, TEXT("InTexture"));
		InputTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("InTextureSampler"));
		flipDirectionParameter.Bind(Initializer.ParameterMap, TEXT("InFlipDirection"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return  IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		FTextureRHIRef InputTexture,
		FVector2D flipDirection)
	{
		SetTextureParameter(RHICmdList, GetPixelShader(), InputTextureParameter, InputTexture);
		RHICmdList.SetShaderSampler(GetPixelShader(), InputTextureSamplerParameter.GetBaseIndex(), TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());

		SetShaderValue(RHICmdList, GetPixelShader(), flipDirectionParameter, flipDirection);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar
			<< InputTextureParameter
			<< InputTextureSamplerParameter
			<< flipDirectionParameter;

		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_GLOBAL_SHADER(FBlitShaderVS, "/LensCalibratorShaders/Private/Blit.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FBlitShaderPS, "/LensCalibratorShaders/Private/Blit.usf", "MainPS", SF_Pixel);
