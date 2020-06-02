/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "DistortionCorrectionMapGenerationShader.h"
#include "RHIStaticStates.h"

FDistortionCorrectionMapGenerationVS::FDistortionCorrectionMapGenerationVS() {}
FDistortionCorrectionMapGenerationVS::FDistortionCorrectionMapGenerationVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer) {}
bool FDistortionCorrectionMapGenerationVS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return true; }

template<typename TShaderRHIParamRef>
void FDistortionCorrectionMapGenerationVS::SetParameters(FRHICommandList& RHICmdList, const TShaderRHIParamRef ShaderRHI, const FGlobalShaderPermutationParameters& ShaderInputData) {}

FDistortionCorrectionMapGenerationPS::FDistortionCorrectionMapGenerationPS() {}

FDistortionCorrectionMapGenerationPS::FDistortionCorrectionMapGenerationPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
	distortionCoefficientsParameter.Bind(Initializer.ParameterMap, TEXT("InDistortionCoefficients"));
	normalizedPrincipalPointParameter.Bind(Initializer.ParameterMap, TEXT("InNormalizedPrincipalPoint"));
	generateInverseMapParameter.Bind(Initializer.ParameterMap, TEXT("InGenerateInverseMap"));
}

bool FDistortionCorrectionMapGenerationPS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5); }

void FDistortionCorrectionMapGenerationPS::SetParameters(
	FRHICommandListImmediate& RHICmdList,
	const FVector2D normalizedPrincipalPoint,
	const TArray<float> inputDistortionCoefficients,
	const bool generateInverseMap)
{
	SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), normalizedPrincipalPointParameter, normalizedPrincipalPoint);
	SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), distortionCoefficientsParameter, inputDistortionCoefficients);
	SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), generateInverseMapParameter, generateInverseMap ? 1 : 0);
}