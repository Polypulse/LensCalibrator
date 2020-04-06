#include "VPTKBlueprintAPI.h"
#pragma push_macro("check")
#undef check
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/types_c.h"
#pragma pop_macro("check")
#include <Runtime\Engine\Classes\Engine\Texture.h>
#include <Runtime\Engine\Classes\Engine\Texture2D.h>
#include "TextureResource.h"
#include "BlitShader.h"
#include <vector.h>

#include "GlobalShader.h"
#include "RHIStaticStates.h"
#include "CommonRenderResources.h"
#include "Engine/RendererSettings.h"
#include "PixelShaderUtils.h"


void UVPTKBlueprintAPI::CalibrateLens(UMediaTexture * mediaTexture)
{
}
