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

bool FDistortionCorrectionShaderVS::Serialize(FArchive& Ar) 
{
	return FGlobalShader::Serialize(Ar);
}

FDistortionCorrectionShaderPS::FDistortionCorrectionShaderPS() {}

FDistortionCorrectionShaderPS::FDistortionCorrectionShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
	/*
	InputTextureParameter.Bind(Initializer.ParameterMap, TEXT("InTexture"));
	InputTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("InTextureSampler"));
	flipDirectionParameter.Bind(Initializer.ParameterMap, TEXT("InFlipDirection"));
	*/
}

bool FDistortionCorrectionShaderPS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

void FDistortionCorrectionShaderPS::SetParameters(
		FRHICommandListImmediate& RHICmdList,
		TArray<float> inputDistortionCoefficients)
{
	/*
	SetTextureParameter(RHICmdList, GetPixelShader(), InputTextureParameter, InputTexture);
	RHICmdList.SetShaderSampler(GetPixelShader(), InputTextureSamplerParameter.GetBaseIndex(), TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());

	SetShaderValue(RHICmdList, GetPixelShader(), flipDirectionParameter, flipDirection);
	*/
	SetShaderValue(RHICmdList, GetPixelShader(), distortionCoefficientsParameter, inputDistortionCoefficients);
}

bool FDistortionCorrectionShaderPS::Serialize(FArchive& Ar) 
{
	bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
	Ar
		<< distortionCoefficientsParameter;

	return bShaderHasOutdatedParameters;
}