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

class FBlitShaderVS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FBlitShaderVS);

public:
	FBlitShaderVS();
	FBlitShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);
	template<typename TShaderRHIParamRef>
	void SetParameters(FRHICommandList& RHICmdList, const TShaderRHIParamRef ShaderRHI, const FGlobalShaderPermutationParameters& ShaderInputData);
	// virtual bool Serialize(FArchive& Ar) override;
};

class FBlitShaderPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FBlitShaderPS);

private:
	FShaderResourceParameter InputTextureParameter;
	FShaderResourceParameter InputTextureSamplerParameter;
	FShaderParameter flipDirectionParameter;

public:
	FBlitShaderPS();
	FBlitShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);

	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		FTextureRHIRef InputTexture,
		FVector2D flipDirection);

	// virtual bool Serialize(FArchive& Ar) override;
};

IMPLEMENT_GLOBAL_SHADER(FBlitShaderVS, "/LensCalibratorShaders/Private/Blit.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FBlitShaderPS, "/LensCalibratorShaders/Private/Blit.usf", "MainPS", SF_Pixel);
