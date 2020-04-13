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

public:
	FBlitShaderPS() {}

	FBlitShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
	{
		InputTextureParameter.Bind(Initializer.ParameterMap, TEXT("InTexture"));
		InputTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("InTextureSampler"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return  IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		FTextureRHIRef InputTexture)
	{
		SetTextureParameter(RHICmdList, GetPixelShader(), InputTextureParameter, InputTexture);
		RHICmdList.SetShaderSampler(GetPixelShader(), InputTextureSamplerParameter.GetBaseIndex(), TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar
			<< InputTextureParameter
			<< InputTextureSamplerParameter;

		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_GLOBAL_SHADER(FBlitShaderVS, "/VPTKShaders/Private/Blit.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FBlitShaderPS, "/VPTKShaders/Private/Blit.usf", "MainPS", SF_Pixel);
