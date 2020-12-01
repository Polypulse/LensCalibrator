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
	k1Parameter.Bind(Initializer.ParameterMap, TEXT("k1"));
	k2Parameter.Bind(Initializer.ParameterMap, TEXT("k2"));
	p1Parameter.Bind(Initializer.ParameterMap, TEXT("p1"));
	p2Parameter.Bind(Initializer.ParameterMap, TEXT("p2"));
	k3Parameter.Bind(Initializer.ParameterMap, TEXT("k3"));
	generateInverseMapParameter.Bind(Initializer.ParameterMap, TEXT("InGenerateInverseMap"));
	normalizedPrincipalPointParameter.Bind(Initializer.ParameterMap, TEXT("InNormalizedPrincipalPoint"));
}

bool FDistortionCorrectionMapGenerationPS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5); }

template<typename TShaderRHIParamRef>
void FDistortionCorrectionMapGenerationPS::SetParameters(
	FRHICommandListImmediate& RHICmdList,
	const TShaderRHIParamRef ShaderRHI,
	FVector2D normalizedPrincipalPoint,
	float k1,
	float k2,
	float p1,
	float p2,
	float k3,
	bool generateInverseMap)
{
	SetShaderValue(RHICmdList, ShaderRHI, k1Parameter, k1);
	SetShaderValue(RHICmdList, ShaderRHI, k2Parameter, k2);
	SetShaderValue(RHICmdList, ShaderRHI, p1Parameter, p1);
	SetShaderValue(RHICmdList, ShaderRHI, p2Parameter, p2);
	SetShaderValue(RHICmdList, ShaderRHI, k3Parameter, k3);

	SetShaderValue(RHICmdList, ShaderRHI, generateInverseMapParameter, generateInverseMap ? 1 : 0);
	SetShaderValue(RHICmdList, ShaderRHI, normalizedPrincipalPointParameter, normalizedPrincipalPoint);
}