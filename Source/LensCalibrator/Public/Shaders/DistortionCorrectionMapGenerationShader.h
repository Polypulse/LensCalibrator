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

class FDistortionCorrectionMapGenerationVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDistortionCorrectionMapGenerationVS, Global);

public:
	FDistortionCorrectionMapGenerationVS();
	FDistortionCorrectionMapGenerationVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);
	template<typename TShaderRHIParamRef>
	void SetParameters(FRHICommandList& RHICmdList, const TShaderRHIParamRef ShaderRHI, const FGlobalShaderPermutationParameters& ShaderInputData);
};

class FDistortionCorrectionMapGenerationPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDistortionCorrectionMapGenerationPS, Global);

private:
	LAYOUT_FIELD(FShaderParameter, distortionCoefficientsParameter);
	LAYOUT_FIELD(FShaderParameter, normalizedPrincipalPointParameter);
	LAYOUT_FIELD(FShaderParameter, generateInverseMapParameter);

public:
	FDistortionCorrectionMapGenerationPS();
	FDistortionCorrectionMapGenerationPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);

	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		const FVector2D normalizedPrincipalPointParameter,
		const TArray<float> inputDistortionCoefficients,
		const bool generateInverseMap);
};

IMPLEMENT_GLOBAL_SHADER(FDistortionCorrectionMapGenerationVS, "/LensCalibratorShaders/Private/DistortionCorrectionMapGeneration.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FDistortionCorrectionMapGenerationPS, "/LensCalibratorShaders/Private/DistortionCorrectionMapGeneration.usf", "MainPS", SF_Pixel);
