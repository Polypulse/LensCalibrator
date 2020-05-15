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
	DECLARE_SHADER_TYPE(FDistortionCorrectionShaderVS, Global);

public:
	FDistortionCorrectionShaderVS();
	FDistortionCorrectionShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);
	template<typename TShaderRHIParamRef>
	void SetParameters(FRHICommandList& RHICmdList, const TShaderRHIParamRef ShaderRHI, const FGlobalShaderPermutationParameters& ShaderInputData);
	// virtual bool Serialize(FArchive& Ar) override;
};

class FDistortionCorrectionShaderPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDistortionCorrectionShaderPS, Global);

private:
	LAYOUT_FIELD(FShaderResourceParameter, InputDistortedTextureParameter);
	LAYOUT_FIELD(FShaderResourceParameter, InputDistortedTextureSamplerParameter);
	LAYOUT_FIELD(FShaderResourceParameter, InputDistortionCorrectionTextureParameter);
	LAYOUT_FIELD(FShaderResourceParameter, InputDistortionCorrectionTextureSamplerParameter);
	LAYOUT_FIELD(FShaderParameter, reverseParameter);

public:
	FDistortionCorrectionShaderPS();
	FDistortionCorrectionShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);

	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		FTextureRHIRef InputDistortedTexture,
		FTextureRHIRef InputDistortionCorrectionTexture,
		bool reverse);

	// virtual bool Serialize(FArchive& Ar) override;
};

IMPLEMENT_GLOBAL_SHADER(FDistortionCorrectionShaderVS, "/LensCalibratorShaders/Private/DistortionCorrection.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FDistortionCorrectionShaderPS, "/LensCalibratorShaders/Private/DistortionCorrection.usf", "MainPS", SF_Pixel);
