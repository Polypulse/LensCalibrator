#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "LensSolverWorker.h"

class FLensSolverWorkerFindCorners : public FLensSolverWorker
{
public:
	DECLARE_DELEGATE_OneParam(QueueTextureFileWorkUnitInputDel, FLensSolverTextureFileWorkUnit)
	DECLARE_DELEGATE_OneParam(QueuePixelArrayWorkUnitInputDel, FLensSolverPixelArrayWorkUnit)
	DECLARE_DELEGATE_OneParam(QueueFindCornerResultOutputDel, FLensSolverCalibrateWorkUnit)
	FLensSolverWorkerFindCorners(
		FLensSolverWorkerParameters inputParameters,
		const QueueTextureFileWorkUnitInputDel * inputQueueTextureFileWorkUnitInputDel,
		const QueuePixelArrayWorkUnitInputDel * inputQueuePixelArrayWorkUnitInputDel,
		const QueueFindCornerResultOutputDel * inputQueueFindCornerResultOutputDel
	);

	~FLensSolverWorkerFindCorners() {}


private:
	mutable int workUnitCount;

	TQueue<FLensSolverPixelArrayWorkUnit> pixelArrayWorkQueue;
	TQueue<FLensSolverTextureFileWorkUnit> textureFileWorkQueue;

	const QueueTextureFileWorkUnitInputDel* queueTextureFileWorkUnitInputDel;
	const QueuePixelArrayWorkUnitInputDel* queuePixelArrayWorkUnitInputDel;
	const QueueFindCornerResultOutputDel* queueFindCornerResultOutputDel;

	void WriteMatToFile(cv::Mat image, FString folder, FString fileName);
	void DequeueTextureFileWorkUnit(FLensSolverTextureFileWorkUnit& workUnit);
	void DequeuePixelArrayWorkUnit(FLensSolverPixelArrayWorkUnit & workUnit);
	void QueueTextureFileWorkUnit(FLensSolverTextureFileWorkUnit workUnit);
	void QueuePixelArrayWorkUnit(FLensSolverPixelArrayWorkUnit workUnit);

protected:
	virtual void Tick() override;
	bool GetImageFromFile(const FString & absoluteFilePath, cv::Mat& image, FIntPoint & sourceResolution);
	bool GetImageFromArray(const TArray<FColor> & pixels, const FIntPoint resolution, cv::Mat& image);

	virtual int GetWorkLoad() override;
};
