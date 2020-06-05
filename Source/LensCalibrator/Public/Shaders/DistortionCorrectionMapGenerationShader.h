/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "RenderResource.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
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
	LAYOUT_FIELD(FShaderParameter, normalizedPrincipalPointParameter);
	LAYOUT_FIELD(FShaderParameter, generateInverseMapParameter);
	LAYOUT_FIELD(FShaderParameter, k1Parameter);
	LAYOUT_FIELD(FShaderParameter, k2Parameter);
	LAYOUT_FIELD(FShaderParameter, p1Parameter);
	LAYOUT_FIELD(FShaderParameter, p2Parameter);
	LAYOUT_FIELD(FShaderParameter, k3Parameter);

public:
	FDistortionCorrectionMapGenerationPS();
	FDistortionCorrectionMapGenerationPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);

	template<typename TShaderRHIParamRef>
	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		const TShaderRHIParamRef ShaderRHI,
		FVector2D normalizedPrincipalPointParameter,
		const float k1,
		const float k2,
		const float p1,
		const float p2,
		const float k3,
		bool generateInverseMap);
};

IMPLEMENT_GLOBAL_SHADER(FDistortionCorrectionMapGenerationVS, "/LensCalibratorShaders/Private/DistortionCorrectionMapGeneration.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FDistortionCorrectionMapGenerationPS, "/LensCalibratorShaders/Private/DistortionCorrectionMapGeneration.usf", "MainPS", SF_Pixel);
