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
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "ShaderParameters.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"

/* This is a basic vertex shader that just renders a full screen quad. */
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

/* The purpose of this shader is to generate a distortion or
inverse distortion map from kth distortion coefficients
determined during the lens calibration process. */
class FDistortionCorrectionMapGenerationPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDistortionCorrectionMapGenerationPS, Global);

private:
	/* Center of the lens. */
	LAYOUT_FIELD(FShaderParameter, normalizedPrincipalPointParameter);

	/* Generate distortion removal/add toggle. */
	LAYOUT_FIELD(FShaderParameter, generateInverseMapParameter);

	/* Kth distortion coefficients, see: https://en.wikipedia.org/wiki/Distortion_(optics) */
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
	/* Apply shader parameters. */
	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		const TShaderRHIParamRef ShaderRHI,
		FVector2D normalizedPrincipalPointParameter,
		float k1,
		float k2,
		float p1,
		float p2,
		float k3,
		bool generateInverseMap);
};

/* Paths to vertex/pixel shader files. */
IMPLEMENT_GLOBAL_SHADER(FDistortionCorrectionMapGenerationVS, "/LensCalibratorShaders/Private/DistortionCorrectionMapGeneration.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FDistortionCorrectionMapGenerationPS, "/LensCalibratorShaders/Private/DistortionCorrectionMapGeneration.usf", "MainPS", SF_Pixel);
