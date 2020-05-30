/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "BlitShader.h"
#include "RHIStaticStates.h"
// #include "SceneRenderTargetParameters.h"
/*
#include "CoreMinimal.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "RenderResource.h"
#include "ShaderParameters.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "SceneRenderTargetParameters.h"
*/

FBlitShaderVS::FBlitShaderVS() {}

FBlitShaderVS::FBlitShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
}

bool FBlitShaderVS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	return true;
}

template<typename TShaderRHIParamRef>
void FBlitShaderVS::SetParameters(FRHICommandList& RHICmdList, const TShaderRHIParamRef ShaderRHI, const FGlobalShaderPermutationParameters& ShaderInputData)
{
}

/*
bool FBlitShaderVS::Serialize(FArchive& Ar) 
{
	return FGlobalShader::Serialize(Ar);
}
*/

FBlitShaderPS::FBlitShaderPS() {}

FBlitShaderPS::FBlitShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
	InputTextureParameter.Bind(Initializer.ParameterMap, TEXT("InTexture"));
	InputTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("InTextureSampler"));
	flipDirectionParameter.Bind(Initializer.ParameterMap, TEXT("InFlipDirection"));
}

bool FBlitShaderPS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	return  IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

void FBlitShaderPS::SetParameters(
	FRHICommandListImmediate& RHICmdList,
	FTextureRHIRef InputTexture,
	FVector2D flipDirection)
{
	SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), InputTextureParameter, InputTexture);
	RHICmdList.SetShaderSampler(RHICmdList.GetBoundPixelShader(), InputTextureSamplerParameter.GetBaseIndex(), TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());

	SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), flipDirectionParameter, flipDirection);
}

/*
bool FBlitShaderPS::Serialize(FArchive& Ar) 
{
	bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
	Ar
		<< InputTextureParameter
		<< InputTextureSamplerParameter
		<< flipDirectionParameter;

	return bShaderHasOutdatedParameters;
}
*/