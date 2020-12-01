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


#include "BlitShader.h"
#include "RHIStaticStates.h"

FBlitShaderVS::FBlitShaderVS() {}
FBlitShaderVS::FBlitShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer) {}
bool FBlitShaderVS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return true; }

template<typename TShaderRHIParamRef>
void FBlitShaderVS::SetParameters(FRHICommandList& RHICmdList, const TShaderRHIParamRef ShaderRHI, const FGlobalShaderPermutationParameters& ShaderInputData) {}

FBlitShaderPS::FBlitShaderPS() {}
FBlitShaderPS::FBlitShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
	InputTextureParameter.Bind(Initializer.ParameterMap, TEXT("InTexture"));
	InputTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("InTextureSampler"));
	flipDirectionParameter.Bind(Initializer.ParameterMap, TEXT("InFlipDirection"));
}

bool FBlitShaderPS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5); }

void FBlitShaderPS::SetParameters(
	FRHICommandListImmediate& RHICmdList,
	FTextureRHIRef InputTexture,
	FVector2D flipDirection)
{
	SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), InputTextureParameter, InputTexture);
	RHICmdList.SetShaderSampler(RHICmdList.GetBoundPixelShader(), InputTextureSamplerParameter.GetBaseIndex(), TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());

	SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), flipDirectionParameter, flipDirection);
}