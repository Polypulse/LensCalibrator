/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "DistortionCorrectionMapGenerationShader.h"
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

FDistortionCorrectionMapGenerationVS::FDistortionCorrectionMapGenerationVS() {}

FDistortionCorrectionMapGenerationVS::FDistortionCorrectionMapGenerationVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
}

bool FDistortionCorrectionMapGenerationVS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	return true;
}

template<typename TShaderRHIParamRef>
void FDistortionCorrectionMapGenerationVS::SetParameters(FRHICommandList& RHICmdList, const TShaderRHIParamRef ShaderRHI, const FGlobalShaderPermutationParameters& ShaderInputData)
{
}

bool FDistortionCorrectionMapGenerationVS::Serialize(FArchive& Ar) 
{
	return FGlobalShader::Serialize(Ar);
}

FDistortionCorrectionMapGenerationPS::FDistortionCorrectionMapGenerationPS() {}

FDistortionCorrectionMapGenerationPS::FDistortionCorrectionMapGenerationPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
	distortionCoefficientsParameter.Bind(Initializer.ParameterMap, TEXT("InDistortionCoefficients"));
	normalizedPrincipalPointParameter.Bind(Initializer.ParameterMap, TEXT("InNormalizedPrincipalPoint"));
}

bool FDistortionCorrectionMapGenerationPS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

void FDistortionCorrectionMapGenerationPS::SetParameters(
		FRHICommandListImmediate& RHICmdList,
		FVector2D normalizedPrincipalPoint,
		TArray<float> inputDistortionCoefficients)
{
	/*
	SetTextureParameter(RHICmdList, GetPixelShader(), InputTextureParameter, InputTexture);
	RHICmdList.SetShaderSampler(GetPixelShader(), InputTextureSamplerParameter.GetBaseIndex(), TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());

	SetShaderValue(RHICmdList, GetPixelShader(), flipDirectionParameter, flipDirection);
	*/
	SetShaderValue(RHICmdList, GetPixelShader(), normalizedPrincipalPointParameter, normalizedPrincipalPoint);
	SetShaderValue(RHICmdList, GetPixelShader(), distortionCoefficientsParameter, inputDistortionCoefficients);
}

bool FDistortionCorrectionMapGenerationPS::Serialize(FArchive& Ar) 
{
	bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
	Ar
		<< distortionCoefficientsParameter
		<< normalizedPrincipalPointParameter;

	return bShaderHasOutdatedParameters;
}