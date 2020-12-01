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

#include "DistortionCorrectionShader.h"
#include "RHIStaticStates.h"

FDistortionCorrectionShaderVS::FDistortionCorrectionShaderVS() {}
FDistortionCorrectionShaderVS::FDistortionCorrectionShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer) {}
bool FDistortionCorrectionShaderVS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return true; }

template<typename TShaderRHIParamRef>
void FDistortionCorrectionShaderVS::SetParameters(FRHICommandList& RHICmdList, const TShaderRHIParamRef ShaderRHI, const FGlobalShaderPermutationParameters& ShaderInputData) {}

FDistortionCorrectionShaderPS::FDistortionCorrectionShaderPS() {}
FDistortionCorrectionShaderPS::FDistortionCorrectionShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
	InputDistortedTextureParameter.Bind(Initializer.ParameterMap, TEXT("InDistortedTexture"));
	InputDistortedTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("InDistortedTextureSampler"));

	InputDistortionCorrectionTextureParameter.Bind(Initializer.ParameterMap, TEXT("InDistortionCorrectionTexture"));
	InputDistortionCorrectionTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("InDistortionCorrectionTextureSampler"));

	reverseParameter.Bind(Initializer.ParameterMap, TEXT("InReverse"));
}

bool FDistortionCorrectionShaderPS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5); }

void FDistortionCorrectionShaderPS::SetParameters(
	FRHICommandListImmediate& RHICmdList,
	FTextureRHIRef InputDistortedTexture,
	FTextureRHIRef InputDistortionCorrectionTexture,
	bool reverse)
{
	SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), InputDistortedTextureParameter, InputDistortedTexture);
	RHICmdList.SetShaderSampler(RHICmdList.GetBoundPixelShader(), InputDistortedTextureSamplerParameter.GetBaseIndex(), TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());

	SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), InputDistortionCorrectionTextureParameter, InputDistortionCorrectionTexture);
	RHICmdList.SetShaderSampler(RHICmdList.GetBoundPixelShader(), InputDistortionCorrectionTextureSamplerParameter.GetBaseIndex(), TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());

	SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), reverseParameter, (reverse ? 1 : 0));
}