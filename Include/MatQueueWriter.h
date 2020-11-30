/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include <queue>
#include <string>
#include <mutex>

#pragma push_macro("check")
#undef check
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/types_c.h"
#pragma pop_macro("check")

struct MatQueueContainer
{
	std::string folderPath;
	cv::Mat mat;
};

class MatQueueWriter
{
private:
	std::queue<MatQueueContainer> matQueue;
	std::mutex queueMutex;

public:
	MatQueueWriter() {}
	MatQueueWriter(MatQueueWriter const&) = delete;
	void operator=(MatQueueWriter const&) = delete;

	__declspec(dllexport) void QueueMat(
		const std::string & outputPath,
		cv::Mat inputMat,
		const bool debug);

	__declspec(dllexport) void Poll(const bool debug);

private:
	// void ValidateFilePath(std::string & folderPath, std::string& fileName, std::string& outputPath);
};

extern "C" __declspec(dllexport) MatQueueWriter & GetMatQueueWriter();