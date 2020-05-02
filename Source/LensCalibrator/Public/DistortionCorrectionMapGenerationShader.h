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
	DECLARE_GLOBAL_SHADER(FDistortionCorrectionMapGenerationVS);

public:
	FDistortionCorrectionMapGenerationVS();
	FDistortionCorrectionMapGenerationVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);
	template<typename TShaderRHIParamRef>
	void SetParameters(FRHICommandList& RHICmdList, const TShaderRHIParamRef ShaderRHI, const FGlobalShaderPermutationParameters& ShaderInputData);
	virtual bool Serialize(FArchive& Ar) override;
};

class FDistortionCorrectionMapGenerationPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FDistortionCorrectionMapGenerationPS);

private:
	/*
	FShaderResourceParameter InputTextureParameter;
	FShaderResourceParameter InputTextureSamplerParameter;
	FShaderParameter flipDirectionParameter;
	*/
	FShaderParameter distortionCoefficientsParameter;
	FShaderParameter normalizedPrincipalPointParameter;

public:
	FDistortionCorrectionMapGenerationPS();
	FDistortionCorrectionMapGenerationPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);

	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		FVector2D normalizedPrincipalPointParameter,
		TArray<float> inputDistortionCoefficients);

	virtual bool Serialize(FArchive& Ar) override;
};

IMPLEMENT_GLOBAL_SHADER(FDistortionCorrectionMapGenerationVS, "/LensCalibratorShaders/Private/DistortionCorrectionMapGeneration.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FDistortionCorrectionMapGenerationPS, "/LensCalibratorShaders/Private/DistortionCorrectionMapGeneration.usf", "MainPS", SF_Pixel);
