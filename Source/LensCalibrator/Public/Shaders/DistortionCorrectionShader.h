/*
 * Copyright (C) 2020 - LensCalibrator contributors, see Contributors.txt at the root of the project.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
class FDistortionCorrectionShaderVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDistortionCorrectionShaderVS, Global);

public:
	FDistortionCorrectionShaderVS();
	FDistortionCorrectionShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);
	template<typename TShaderRHIParamRef>
	void SetParameters(FRHICommandList& RHICmdList, const TShaderRHIParamRef ShaderRHI, const FGlobalShaderPermutationParameters& ShaderInputData);
};

/* The purpose of this shader is to add/remove lens distortion from an
image using a texture containing UV coordinates to resample the input. However
this only performs distortion add/removal via a single texture at a single zoom 
level. Currently, full lens distortion add/removal across zoom levels occurs in
a material editor shader so it can be exposed to the user. */
class FDistortionCorrectionShaderPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDistortionCorrectionShaderPS, Global);

private:
	/* Input camera feed. */
	LAYOUT_FIELD(FShaderResourceParameter, InputDistortedTextureParameter);
	/* Input camera feed parameters. */
	LAYOUT_FIELD(FShaderResourceParameter, InputDistortedTextureSamplerParameter);

	/* Input distortion correction texture. */
	LAYOUT_FIELD(FShaderResourceParameter, InputDistortionCorrectionTextureParameter);

	/* Input distortion correction texture parameters. */
	LAYOUT_FIELD(FShaderResourceParameter, InputDistortionCorrectionTextureSamplerParameter);

	/* Invert distortion correction. */
	LAYOUT_FIELD(FShaderParameter, reverseParameter);

public:
	FDistortionCorrectionShaderPS();
	FDistortionCorrectionShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);

	/* Set shader parameter values. */
	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		FTextureRHIRef InputDistortedTexture,
		FTextureRHIRef InputDistortionCorrectionTexture,
		bool reverse);
};

/* Paths to vertex/pixel shader files. */
IMPLEMENT_GLOBAL_SHADER(FDistortionCorrectionShaderVS, "/LensCalibratorShaders/Private/DistortionCorrection.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FDistortionCorrectionShaderPS, "/LensCalibratorShaders/Private/DistortionCorrection.usf", "MainPS", SF_Pixel);
