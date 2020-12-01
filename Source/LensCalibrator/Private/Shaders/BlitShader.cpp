/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */


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