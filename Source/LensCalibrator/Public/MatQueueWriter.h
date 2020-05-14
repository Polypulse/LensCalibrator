#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine.h"

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
	FString outputPath;
	cv::Mat mat;
};

class MatQueueWriter
{
public:
	static MatQueueWriter & Get()
	{
		static MatQueueWriter instance;
		return instance;
	}

private:
	TQueue<MatQueueContainer> matQueue;
	MatQueueWriter() {}

public:
	MatQueueWriter(MatQueueWriter const&) = delete;
	void operator=(MatQueueWriter const&) = delete;

	void QueueMat(FString outputPath, cv::Mat inputMat);
	void Poll();
};