/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once

#include <vector>

#pragma push_macro("check")
#undef check
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/types_c.h"
#pragma pop_macro("check")

struct FResizeParameters
{
	int sourceX, sourceY;
	int resizeX, resizeY;
	int nativeX, nativeY;

	FResizeParameters()
	{
		sourceX = 0; sourceY = 0;
		resizeX = 0;  resizeY = 0;
		nativeX = 0;  nativeY = 0;
	}
};

struct FChessboardSearchParameters
{
	int nativeFullResolutionX, nativeFullResolutionY;
	float resizePercentage;
	bool resize;
	bool flipX, flipY;
	bool exhaustiveSearch;
	float checkerBoardSquareSizeMM;
	int checkerBoardCornerCountX, checkerBoardCornerCountY;

	bool writePreCornerDetectionTextureToFile;
	char preCornerDetectionTextureOutputPath[260];

	bool writeCornerVisualizationTextureToFile;
	char cornerVisualizationTextureOutputPath[260];

	FChessboardSearchParameters()
	{
		nativeFullResolutionX = 0; nativeFullResolutionY = 0;
		resizePercentage = 0.5f;
		resize = true;
		flipX = false; flipY = false;
		exhaustiveSearch = false;
		checkerBoardSquareSizeMM = 12.7f;
		checkerBoardCornerCountX = 0; checkerBoardCornerCountY = 0;

		writePreCornerDetectionTextureToFile = false;
		memset(preCornerDetectionTextureOutputPath, 0, sizeof(char) * 260);

		writeCornerVisualizationTextureToFile = false;
		memset(cornerVisualizationTextureOutputPath, 0, sizeof(char) * 260);
	}
};

struct FCalibrateLensParameters
{
	float sensorDiagonalSizeMM;
	float initialPrincipalPointNativePixelPositionX, initialPrincipalPointNativePixelPositionY;
	bool useInitialIntrinsicValues;
	bool keepPrincipalPixelPositionFixed;
	bool keepAspectRatioFixed;
	bool lensHasTangentalDistortion;
	bool fixRadialDistortionCoefficientK1;
	bool fixRadialDistortionCoefficientK2;
	bool fixRadialDistortionCoefficientK3;
	bool fixRadialDistortionCoefficientK4;
	bool fixRadialDistortionCoefficientK5;
	bool fixRadialDistortionCoefficientK6;
	bool useRationalModel;

	FCalibrateLensParameters()
	{
		sensorDiagonalSizeMM = 9.960784f;
		initialPrincipalPointNativePixelPositionX = 0.0f;
		initialPrincipalPointNativePixelPositionY = 0.0f;

		useInitialIntrinsicValues = false;
		keepPrincipalPixelPositionFixed = false;
		keepAspectRatioFixed = true;
		lensHasTangentalDistortion = false;
		fixRadialDistortionCoefficientK1 = false;
		fixRadialDistortionCoefficientK2 = false;
		fixRadialDistortionCoefficientK3 = false;
		fixRadialDistortionCoefficientK4 = false;
		fixRadialDistortionCoefficientK5 = false;
		fixRadialDistortionCoefficientK6 = false;
		useRationalModel = false;
	}
};

struct FCalibrateLensOutput
{
	float error;
	float fovX;
	float fovY;
	float focalLengthMM;
	float aspectRatio;
	float sensorSizeMMX, sensorSizeMMY;
	float principalPixelPointX, principalPixelPointY;
	int resolutionX, resolutionY;
	float k1, k2, p1, p2, k3, k4, k5, k6;

	FCalibrateLensOutput()
	{
		error = 0.0f;
		fovX = 0.0f;
		fovY = 0.0f;
		focalLengthMM = 0.0f;
		aspectRatio = 0.0f;
		sensorSizeMMX = 0.0f;
		sensorSizeMMY = 0.0f;
		principalPixelPointX = 0.0f;
		principalPixelPointY = 0.0f;
		resolutionX = 0;
		resolutionY = 0;
		k1 = 0; k2 = 0; p1 = 0; p2 = 0; k3 = 0;
		k4 = 0; k5 = 0; k6 = 0;
	}
};

class OpenCVWrapper
{
public:
	OpenCVWrapper() {}
	OpenCVWrapper(OpenCVWrapper const&) = delete;
	void operator=(OpenCVWrapper const&) = delete;

	__declspec(dllexport) bool ProcessImageFromFile (
		FResizeParameters & resizeParameters,
		const FChessboardSearchParameters & textureSearchParameters,
		const char absoluteFilePath[260], 
		float * data,
		const bool debug);

	__declspec(dllexport) bool ProcessImageFromPixels (
		const FResizeParameters & resizeParameters,
		const FChessboardSearchParameters & textureSearchParameters,
		const uint8_t * pixels,
		const int stride,
		const int width,
		const int height, 
		float * data,
		const bool debug);

	__declspec(dllexport) bool CalibrateLens (
		const FResizeParameters & resizeParameters,
		const FCalibrateLensParameters & calibrationParameters,
		const float * cornersData,
		const float chessboardSquareSizeMM,
		const int cornerCountX,
		const int cornerCountY,
		const int imageCount,
		FCalibrateLensOutput & output,
		const bool debug);

private:
	bool GetImageFromArray(
		const uint8_t * pixels,
		const int stride,
		const int width,
		const int height,
		cv::Mat& image,
		const bool debug);

	bool GetImageFromFile(
		const std::string & absoluteFilePath,
		cv::Mat& image,
		int & sourceWidth,
		int & sourceHeight,
		const bool debug);

	void WriteMatToFile(
		cv::Mat image,
		const std::string & outputPath,
		const bool debug);

	bool ProcessImage(
		const FResizeParameters & resizeParameters,
		const FChessboardSearchParameters & textureSearchParameters,
		cv::Mat image,
		float * data,
		const bool debug);
};

extern "C" __declspec(dllexport) OpenCVWrapper & GetOpenCVWrapper();