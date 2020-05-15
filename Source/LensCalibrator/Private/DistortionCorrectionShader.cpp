/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "DistortionCorrectionShader.h"
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

FDistortionCorrectionShaderVS::FDistortionCorrectionShaderVS() {}

FDistortionCorrectionShaderVS::FDistortionCorrectionShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
}

bool FDistortionCorrectionShaderVS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	return true;
}

template<typename TShaderRHIParamRef>
void FDistortionCorrectionShaderVS::SetParameters(FRHICommandList& RHICmdList, const TShaderRHIParamRef ShaderRHI, const FGlobalShaderPermutationParameters& ShaderInputData)
{
}

/*
bool FDistortionCorrectionShaderVS::Serialize(FArchive& Ar) 
{
	return FGlobalShader::Serialize(Ar);
}
*/

FDistortionCorrectionShaderPS::FDistortionCorrectionShaderPS() {}

FDistortionCorrectionShaderPS::FDistortionCorrectionShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
	InputDistortedTextureParameter.Bind(Initializer.ParameterMap, TEXT("InDistortedTexture"));
	InputDistortedTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("InDistortedTextureSampler"));

	InputDistortionCorrectionTextureParameter.Bind(Initializer.ParameterMap, TEXT("InDistortionCorrectionTexture"));
	InputDistortionCorrectionTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("InDistortionCorrectionTextureSampler"));

	reverseParameter.Bind(Initializer.ParameterMap, TEXT("InReverse"));
}

bool FDistortionCorrectionShaderPS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	return  IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

void FDistortionCorrectionShaderPS::SetParameters(
	FRHICommandListImmediate& RHICmdList,
	FTextureRHIRef InputDistortedTexture,
	FTextureRHIRef InputDistortionCorrectionTexture,
	bool reverse)
{
	SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), InputDistortedTextureParameter, InputDistortedTexture);
	RHICmdList.SetShaderSampler(RHICmdList.GetBoundPixelShader(), InputDistortedTextureSamplerParameter.GetBaseIndex(), TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());

	SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), InputDistortionCorrectionTextureParameter, InputDistortionCorrectionTexture);
	RHICmdList.SetShaderSampler(RHICmdList.GetBoundPixelShader(), InputDistortionCorrectionTextureSamplerParameter.GetBaseIndex(), TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());

	SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), reverseParameter, (reverse ? 1 : 0));
}

/*
bool FDistortionCorrectionShaderPS::Serialize(FArchive& Ar) 
{
	bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
	Ar
		<< InputDistortedTextureParameter
		<< InputDistortedTextureSamplerParameter
		<< InputDistortionCorrectionTextureParameter
		<< InputDistortionCorrectionTextureSamplerParameter
		<< reverseParameter;

	return bShaderHasOutdatedParameters;
}
*/