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

/* This is a basic vertex shader that just renders a full screen quad. */
class FBlitShaderVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FBlitShaderVS, Global);

public:
	FBlitShaderVS();
	FBlitShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);
	template<typename TShaderRHIParamRef>
	void SetParameters(FRHICommandList& RHICmdList, const TShaderRHIParamRef ShaderRHI, const FGlobalShaderPermutationParameters& ShaderInputData);
};

/* The purpose of this shader is to copy, resize and prepare the input 
calibration pattern image for for processing and input into OpenCV. */
class FBlitShaderPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FBlitShaderPS, Global);

private:
	/* Shader input parameters. */
	LAYOUT_FIELD(FShaderResourceParameter, InputTextureParameter);
	LAYOUT_FIELD(FShaderResourceParameter, InputTextureSamplerParameter);
	LAYOUT_FIELD(FShaderParameter, flipDirectionParameter);

public:
	FBlitShaderPS();
	FBlitShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);

	/* Apply shader parameter values. */
	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		FTextureRHIRef InputTexture,
		FVector2D flipDirection);

};

/* Paths to vertex/pixel shader. */
IMPLEMENT_GLOBAL_SHADER(FBlitShaderVS, "/LensCalibratorShaders/Private/Blit.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FBlitShaderPS, "/LensCalibratorShaders/Private/Blit.usf", "MainPS", SF_Pixel);
