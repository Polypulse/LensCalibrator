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

class FDistortionCorrectionShaderVS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FDistortionCorrectionShaderVS);

public:
	FDistortionCorrectionShaderVS();
	FDistortionCorrectionShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);
	template<typename TShaderRHIParamRef>
	void SetParameters(FRHICommandList& RHICmdList, const TShaderRHIParamRef ShaderRHI, const FGlobalShaderPermutationParameters& ShaderInputData);
	virtual bool Serialize(FArchive& Ar) override;
};

class FDistortionCorrectionShaderPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FDistortionCorrectionShaderPS);

private:
	/*
	FShaderResourceParameter InputTextureParameter;
	FShaderResourceParameter InputTextureSamplerParameter;
	FShaderParameter flipDirectionParameter;
	*/
	FShaderParameter distortionCoefficientsParameter;

public:
	FDistortionCorrectionShaderPS();
	FDistortionCorrectionShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);

	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		TArray<float> inputDistortionCoefficients);

	virtual bool Serialize(FArchive& Ar) override;
};

IMPLEMENT_GLOBAL_SHADER(FDistortionCorrectionShaderVS, "/LensCalibratorShaders/Private/DistortionCorrection.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FDistortionCorrectionShaderPS, "/LensCalibratorShaders/Private/DistortionCorrection.usf", "MainPS", SF_Pixel);
