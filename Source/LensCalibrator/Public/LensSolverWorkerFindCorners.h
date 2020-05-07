#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "LensSolverWorker.h"

DECLARE_DELEGATE_OneParam(QueueTextureFileWorkUnitInputDel, FLensSolverTextureFileWorkUnit)
DECLARE_DELEGATE_OneParam(QueuePixelArrayWorkUnitInputDel, FLensSolverPixelArrayWorkUnit)
DECLARE_DELEGATE_OneParam(QueueFindCornerResultOutputDel, FLensSolverCalibrateWorkUnit)

class FLensSolverWorkerFindCorners : public FLensSolverWorker
{
public:
	FLensSolverWorkerFindCorners(
		const FLensSolverWorkerParameters & inputParameters,
		QueueTextureFileWorkUnitInputDel* inputQueueTextureFileWorkUnitInputDel,
		QueuePixelArrayWorkUnitInputDel* inputQueuePixelArrayWorkUnitInputDel,
		const QueueFindCornerResultOutputDel* inputQueueFindCornerResultOutputDel);

	~FLensSolverWorkerFindCorners() {}


private:
	mutable int workUnitCount;

	TQueue<FLensSolverPixelArrayWorkUnit> pixelArrayWorkQueue;
	TQueue<FLensSolverTextureFileWorkUnit> textureFileWorkQueue;

	/*
	const QueueTextureFileWorkUnitInputDel* queueTextureFileWorkUnitInputDel;
	const QueuePixelArrayWorkUnitInputDel* queuePixelArrayWorkUnitInputDel;
	*/
	const QueueFindCornerResultOutputDel* queueFindCornerResultOutputDel;

	void WriteMatToFile(cv::Mat image, FString folder, FString fileName);
	void DequeueTextureFileWorkUnit(FLensSolverTextureFileWorkUnit& workUnit);
	void DequeuePixelArrayWorkUnit(FLensSolverPixelArrayWorkUnit & workUnit);
	void QueueTextureFileWorkUnit(FLensSolverTextureFileWorkUnit workUnit);
	void QueuePixelArrayWorkUnit(FLensSolverPixelArrayWorkUnit workUnit);

protected:
	bool GetImageFromFile(const FString & absoluteFilePath, cv::Mat& image, FIntPoint & sourceResolution);
	bool GetImageFromArray(const TArray<FColor> & pixels, const FIntPoint resolution, cv::Mat& image);

	virtual void Tick() override;
	virtual int GetWorkLoad() override;
};
