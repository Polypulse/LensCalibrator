/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "LensSolverWorker.h"

DECLARE_DELEGATE_OneParam(QueueTextureFileWorkUnitInputDel, FLensSolverTextureFileWorkUnit)
DECLARE_DELEGATE_OneParam(QueuePixelArrayWorkUnitInputDel, FLensSolverPixelArrayWorkUnit)
DECLARE_DELEGATE_OneParam(QueueFindCornerResultOutputDel, FLensSolverCalibrationPointsWorkUnit)

class FLensSolverWorkerFindCorners : public FLensSolverWorker
{
public:
	FLensSolverWorkerFindCorners(
		const FLensSolverWorkerParameters & inputParameters,
		QueueTextureFileWorkUnitInputDel* inputQueueTextureFileWorkUnitInputDel,
		QueuePixelArrayWorkUnitInputDel* inputQueuePixelArrayWorkUnitInputDel,
		const QueueFindCornerResultOutputDel* inputQueueFindCornerResultOutputDel);

	~FLensSolverWorkerFindCorners()
	{
		/*
		queueTextureFileWorkUnitInputDel->Unbind();
		queuePixelArrayWorkUnitInputDel->Unbind();

		queueTextureFileWorkUnitInputDel = nullptr;
		queuePixelArrayWorkUnitInputDel = nullptr;
		/*
		if (queueTextureFileWorkUnitInputDel != nullptr && queueTextureFileWorkUnitInputDel->IsBound())
		{
			queueTextureFileWorkUnitInputDel->Unbind();
			queueTextureFileWorkUnitInputDel = nullptr;
		}

		if (queuePixelArrayWorkUnitInputDel != nullptr && queuePixelArrayWorkUnitInputDel->IsBound())
		{
			queuePixelArrayWorkUnitInputDel->Unbind();
			queuePixelArrayWorkUnitInputDel = nullptr;
		}
		*/
	};

private:
	mutable int workUnitCount;

	TQueue<FLensSolverPixelArrayWorkUnit> pixelArrayWorkQueue;
	TQueue<FLensSolverTextureFileWorkUnit> textureFileWorkQueue;

	/*
	const QueueTextureFileWorkUnitInputDel* queueTextureFileWorkUnitInputDel;
	const QueuePixelArrayWorkUnitInputDel* queuePixelArrayWorkUnitInputDel;
	*/
	QueueTextureFileWorkUnitInputDel* queueTextureFileWorkUnitInputDel;
	QueuePixelArrayWorkUnitInputDel* queuePixelArrayWorkUnitInputDel;
	const QueueFindCornerResultOutputDel* queueFindCornerResultOutputDel;

	void WriteMatToFile(cv::Mat image, FString folder, FString fileName);
	void DequeueTextureFileWorkUnit(FLensSolverTextureFileWorkUnit& workUnit);
	void DequeuePixelArrayWorkUnit(FLensSolverPixelArrayWorkUnit & workUnit);
	void QueueTextureFileWorkUnit(FLensSolverTextureFileWorkUnit workUnit);
	void QueuePixelArrayWorkUnit(FLensSolverPixelArrayWorkUnit workUnit);

	void QueueCalibrationPointsWorkUnit(const FLensSolverCalibrationPointsWorkUnit & calibrationPointsWorkUnit);
	void QueueEmptyCalibrationPointsWorkUnit(const FBaseParameters & baseParameters, const FResizeParameters & resizeParameters);

protected:
	bool GetImageFromFile(const FString & absoluteFilePath, cv::Mat& image, FIntPoint & sourceResolution);
	bool GetImageFromArray(const TArray<FColor> & pixels, const FIntPoint resolution, cv::Mat& image);

	virtual void Tick() override;
	virtual int GetWorkLoad() override;
};
