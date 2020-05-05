#include "LensSolverWorkerCalibrate.h"
#include "JsonUtilities.h"

FLensSolverWorkerCalibrate::FLensSolverWorkerCalibrate(
	FLensSolverWorkerParameters inputParameters,
	QueueLatchInputDel* inputSignalLatch, 
	QueueCalibrationResultOutputDel * inputOnSolvePointsDel) : FLensSolverWorker(inputParameters)
{
	inputSignalLatch->BindRaw(this, &FLensSolverWorkerCalibrate::QueueLatch);
	onSolvePointsDel = inputOnSolvePointsDel;
}

FMatrix FLensSolverWorkerCalibrate::GeneratePerspectiveMatrixFromFocalLength(cv::Size& imageSize, cv::Point2d principlePoint, float focalLength)
{
	FMatrix perspectiveMatrix;

	float min = 0.1f;
	float max = 10000.0f;

	float left = min * (-principlePoint.x) / focalLength;
	float right = min * (imageSize.width - principlePoint.x) / focalLength;
	float bottom = min * (principlePoint.y - imageSize.height) / focalLength;
	float top = min * (principlePoint.y) / focalLength;

	float a = -(right + left) / (right - left);
	float b = -(top + bottom) / (top - bottom);

	float c = min / (min - max);
	float d = -max * min / (min - max);

	float nrl = (2 * min) / (right - left);
	float ntb = (2 * min) / (top - bottom);

	perspectiveMatrix = FMatrix(
		FPlane(nrl,		0.0f,	0.0f,	0.0f),
		FPlane(0.0f,	ntb,	0.0f,	0.0f),
		FPlane(a,		b,		c,		1.0f),
		FPlane(0.0f,	0.0f,	d,		0.0f)
	);

	return perspectiveMatrix;
}

void FLensSolverWorkerCalibrate::TransformVectorFromCVToUE4(FVector& v)
{
	float x, y, z;

	x = v.X;
	y = v.Y;
	z = v.Z;

	v.X = z;
	v.Y = x;
	v.Z = -y;
}

FTransform FLensSolverWorkerCalibrate::GenerateTransformFromRAndTVecs(std::vector<cv::Mat>& rvecs, std::vector<cv::Mat>& tvecs)
{
	FTransform outputTransform;
	FMatrix transformMatrix;

	cv::Mat rot;
	FVector forward, right, up, origin;
	FVector zero(0, 0, 0);

	cv::Rodrigues(rvecs[0], rot);

	forward		= FVector(rot.at<double>(0, 2), rot.at<double>(1, 2), rot.at<double>(2, 2));
	right		= FVector(rot.at<double>(0, 0), rot.at<double>(1, 0), rot.at<double>(2, 0));
	up			= FVector(rot.at<double>(0, 1), rot.at<double>(1, 1), rot.at<double>(2, 1));
	origin		= FVector(tvecs[0].at<double>(0, 0), tvecs[0].at<double>(1, 0), tvecs[0].at<double>(2, 0));

	TransformVectorFromCVToUE4(forward);
	TransformVectorFromCVToUE4(right);
	TransformVectorFromCVToUE4(up);
	TransformVectorFromCVToUE4(origin);

	up = -up;

	transformMatrix.SetAxes(
		&forward,
		&right,
		&up,
		&zero
	);

	transformMatrix = transformMatrix.GetTransposed();
	origin = transformMatrix.TransformPosition(origin);
	outputTransform.SetFromMatrix(transformMatrix);
	outputTransform.SetLocation(-origin);

	return outputTransform;
}

void FLensSolverWorkerCalibrate::Tick()
{
	if (!LatchInqueue())
		return;

	FLatchData latchData;
	DequeueLatch(latchData);

	std::vector<std::vector<cv::Point3f>> objectPoints;
	std::vector<std::vector<cv::Point2f>> corners;

	for (int i = 0; i < latchData.workUnitCount; i++)
	{
		if (!WorkUnitInQueue())
		{
			QueueLog(FString("(ERROR): Latch was received by calibration worker. However, there are not enough work units in the queue!"));
			return;
		}

		TUniquePtr<FLensSolverWorkUnit> workUnit;
		DequeueWorkUnit(workUnit);
		if (!workUnit.IsValid())
		{
			QueueLog(FString("(ERROR): NULL work unit in calibrate worker work unit queue!"));
			return;
		}

		FLensSolverCalibrateWorkUnit* calibrateWorkUnit = static_cast<FLensSolverCalibrateWorkUnit*>(workUnit.Get());
		if (calibrateWorkUnit->corners.size() == 0 || calibrateWorkUnit->objectPoints.size() == 0)
		{
			QueueLog(FString("(ERROR): NULL calibrate worker work unit contained an invalid number of corners or object points!"));
			continue;
		}

		corners.push_back(calibrateWorkUnit->corners);
		objectPoints.push_back(calibrateWorkUnit->objectPoints);
	}

	if (ShouldExit())
		return;

	std::vector<cv::Mat> rvecs, tvecs;
	cv::Mat cameraMatrix = cv::Mat::eye(3, 3, cv::DataType<float>::type);
	cv::Mat distortionCoefficients = cv::Mat::zeros(5, 1, cv::DataType<float>::type);
	cv::Size sourceImageSize(latchData.sourceResolution.X, latchData.sourceResolution.Y);
	cv::Point2d principalPoint = cv::Point2d(0.0, 0.0);
	cv::TermCriteria termCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001f);

	int sourcePixelWidth = latchData.sourceResolution.X;
	int sourcePixelHeight = latchData.sourceResolution.Y;

	float sensorHeight = (latchData.sensorDiagonalMM * sourcePixelWidth) / FMath::Sqrt(sourcePixelWidth * sourcePixelWidth + sourcePixelHeight * sourcePixelHeight);
	float sensorWidth = sensorHeight * (sourcePixelWidth / (float)sourcePixelHeight);

	QueueLog(FString::Printf(TEXT("%sSensor size: (%f, %f) mm, diagonal: (%f) mm."), *workerMessage, sensorWidth, sensorHeight, latchData.sensorDiagonalMM));

	double fovX = 0.0f, fovY = 0.0f, focalLength = 0.0f;
	double aspectRatio = 0.0f;

	FMatrix perspectiveMatrix;

	int flags = 0;
	flags |= latchData.workerParameters.useInitialIntrinsicValues			?	cv::CALIB_USE_INTRINSIC_GUESS : 0;
	flags |= latchData.workerParameters.keepPrincipalPixelPositionFixed		?	cv::CALIB_FIX_PRINCIPAL_POINT : 0;
	flags |= latchData.workerParameters.keepAspectRatioFixed				?	cv::CALIB_FIX_ASPECT_RATIO : 0;
	flags |= latchData.workerParameters.lensHasTangentalDistortion			?	cv::CALIB_ZERO_TANGENT_DIST : 0;
	flags |= latchData.workerParameters.fixRadialDistortionCoefficientK1	?	cv::CALIB_FIX_K1 : 0;
	flags |= latchData.workerParameters.fixRadialDistortionCoefficientK2	?	cv::CALIB_FIX_K2 : 0;
	flags |= latchData.workerParameters.fixRadialDistortionCoefficientK3	?	cv::CALIB_FIX_K3 : 0;
	flags |= latchData.workerParameters.fixRadialDistortionCoefficientK4	?	cv::CALIB_FIX_K4 : 0;
	flags |= latchData.workerParameters.fixRadialDistortionCoefficientK5	?	cv::CALIB_FIX_K5 : 0;
	flags |= latchData.workerParameters.fixRadialDistortionCoefficientK6	?	cv::CALIB_FIX_K6 : 0;

	if (flags & cv::CALIB_USE_INTRINSIC_GUESS)
	{
		cameraMatrix.at<float>(0, 2) = latchData.initialPrincipalPointPixelPosition.X;
		cameraMatrix.at<float>(1, 2) = latchData.initialPrincipalPointPixelPosition.Y;
		QueueLog(FString::Printf(TEXT("Setting initial principal point to: (%f, %f)"), 
			latchData.initialPrincipalPointPixelPosition.X,
			latchData.initialPrincipalPointPixelPosition.Y));
	}

	else if (flags & cv::CALIB_FIX_ASPECT_RATIO)
	{
		cameraMatrix.at<float>(0, 0) = 1.0f / (latchData.sourceResolution.X * 0.5f);
		cameraMatrix.at<float>(1, 1) = 1.0f / (latchData.sourceResolution.Y * 0.5f);
		QueueLog(FString::Printf(TEXT("%sKeeping aspect ratio at: %f"), 
			(latchData.sourceResolution.X / (float)latchData.sourceResolution.Y)));
	}

	double error = cv::calibrateCamera(
		objectPoints,
		corners,
		sourceImageSize,
		cameraMatrix,
		distortionCoefficients,
		rvecs,
		tvecs,
		flags,
		termCriteria);

	if (ShouldExit())
		return;

	cv::calibrationMatrixValues(cameraMatrix, sourceImageSize, sensorWidth, sensorHeight, fovX, fovY, focalLength, principalPoint, aspectRatio);
	FMatrix perspectiveMatrix = GeneratePerspectiveMatrixFromFocalLength(sourceImageSize, principalPoint, focalLength);

	fovX *= 2.0f;
	fovY *= 2.0f;
	focalLength *= 2.0f;

	principalPoint.x = sourcePixelWidth * (principalPoint.x / sensorWidth);
	principalPoint.y = sourcePixelHeight * (principalPoint.y / sensorHeight);

	FString format = FString("Completed camera calibration at zoom level: %f "
		"with solve error: %f "
		"with results: ("
		"\n\tField of View in degrees: (%f, %f)"
		"\n\tSensor width in MM: %f,"
		"\n\tSensor height in MM: %f,"
		"\n\tFocal Length in MM: %f,"
		"\n\tPrincipal Point Pixel: (%f, %f),"
		"\n\tAspect Ratio: %f\n)");

	QueueLog(FString::Printf(*format,
		latchData.zoomLevel,
		error,
		fovX,
		fovY,
		sensorWidth,
		sensorHeight,
		focalLength,
		principalPoint.x, principalPoint.y,
		aspectRatio));

	if (ShouldExit())
		return;

	TArray<float> outputDistortionCoefficients;
	outputDistortionCoefficients.SetNum(5);
	for (int i = 0; i < distortionCoefficients.rows; i++)
		outputDistortionCoefficients[i] = distortionCoefficients.at<float>(i, 0);

	FCalibrationResult solvedPoints;

	solvedPoints.jobInfo = latchData.jobInfo;
	solvedPoints.zoomLevel = latchData.zoomLevel;
	solvedPoints.success = true;
	solvedPoints.fovX = fovX;
	solvedPoints.fovY = fovY;
	solvedPoints.focalLengthMM = focalLength;
	solvedPoints.aspectRatio = aspectRatio;
	solvedPoints.sensorSizeMM = FVector2D(sensorWidth, sensorHeight);
	solvedPoints.principalPixelPoint = FVector2D(principalPoint.x, principalPoint.y);
	solvedPoints.resolution = latchData.sourceResolution;
	solvedPoints.perspectiveMatrix = perspectiveMatrix;
	solvedPoints.distortionCoefficients = outputDistortionCoefficients;

	if (latchData.workerParameters.writeCalibrationResultsToFile)
		WriteSolvedPointsToJSONFile(solvedPoints, latchData.workerParameters.calibrationResultsFolderPath, "result");

	QueueLog(FString("Finished with work unit."));
	QueueSolvedPoints(solvedPoints);
}

bool FLensSolverWorkerCalibrate::WorkUnitInQueue() 
{
	return false;
}

void FLensSolverWorkerCalibrate::WriteSolvedPointsToJSONFile(const FCalibrationResult& solvePoints, FString folder, const FString fileName)
{
	if (!LensSolverUtilities::ValidateFolder(folder, calibrationVisualizationOutputPath, workerMessage))
		return;

	FString outputPath = LensSolverUtilities::GenerateIndexedFilePath(folder, fileName, "json");

	TSharedPtr<FJsonObject> obj = MakeShareable(new FJsonObject);
	TSharedPtr<FJsonObject> result = MakeShareable(new FJsonObject);
	result->SetStringField("jobid", solvePoints.jobInfo.jobID);
	result->SetNumberField("zoomlevel", solvePoints.zoomLevel);
	result->SetNumberField("width", solvePoints.resolution.X);
	result->SetNumberField("height", solvePoints.resolution.Y);
	result->SetNumberField("fovx", solvePoints.fovX);
	result->SetNumberField("fovy", solvePoints.fovY);
	result->SetNumberField("focallength", solvePoints.focalLengthMM);
	result->SetNumberField("aspectratio", solvePoints.aspectRatio);

	TSharedPtr<FJsonObject> sensorSizeObj = MakeShareable(new FJsonObject);
	sensorSizeObj->SetNumberField("X", solvePoints.sensorSizeMM.X);
	sensorSizeObj->SetNumberField("y", solvePoints.sensorSizeMM.Y);
	result->SetObjectField("sensorsizemm", sensorSizeObj);

	TSharedPtr<FJsonObject> principalObj = MakeShareable(new FJsonObject);
	principalObj->SetNumberField("X", solvePoints.principalPixelPoint.X);
	principalObj->SetNumberField("y", solvePoints.principalPixelPoint.Y);
	result->SetObjectField("principalpixelpoint", principalObj);

	TSharedPtr<FJsonObject> resolutionObj = MakeShareable(new FJsonObject);
	resolutionObj->SetNumberField("X", solvePoints.resolution.X);
	resolutionObj->SetNumberField("y", solvePoints.resolution.Y);
	result->SetObjectField("resolution", resolutionObj);

	TArray<TSharedPtr<FJsonValue>> matVals;

	for (int i = 0; i < 16; i++)
		matVals.Add(MakeShareable(new FJsonValueNumber(solvePoints.perspectiveMatrix.M[i / 4][i % 4])));

	result->SetArrayField("perspectivematrix", matVals);

	TArray<TSharedPtr<FJsonValue>> distortionCoefficients;
	for (int i = 0; i < solvePoints.distortionCoefficients.Num(); i++)
		distortionCoefficients.Add(MakeShareable(new FJsonValueNumber(static_cast<float>(solvePoints.distortionCoefficients[i]))));

	result->SetArrayField("distortioncoefficients", distortionCoefficients);
	obj->SetObjectField("result", result);

	FString outputJson(TEXT("{}"));
	TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&outputJson);
	FJsonSerializer::Serialize(obj.ToSharedRef(), writer);

	if (!FFileHelper::SaveStringToFile(outputJson, *outputPath))
	{
		UE_LOG(LogTemp, Error, TEXT("%sUnable to write calibration result to path: \"%s\", check your permissions."), *workerMessage, *outputPath);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("%sCalibration result written to file at path: \"%s\"."), *workerMessage, *outputPath);
}

void FLensSolverWorkerCalibrate::QueueSolvedPointsError(FJobInfo jobInfo, float zoomLevel)
{
	TArray<FVector2D> emptyPoints;

	FCalibrationResult solvedPoints;
	solvedPoints.jobInfo = jobInfo;
	solvedPoints.zoomLevel = zoomLevel;
	solvedPoints.success = false;

	if (!onSolvePointsDel->IsBound())
		return;
	onSolvePointsDel->Execute(solvedPoints);
}

void FLensSolverWorkerCalibrate::QueueSolvedPoints(FCalibrationResult solvedPoints)
{
	if (!onSolvePointsDel->IsBound())
		return;
	onSolvePointsDel->Execute(solvedPoints);
}

void FLensSolverWorkerCalibrate::QueueLatch(const FLatchData latchData)
{
	latchQueue.Enqueue(latchData);
}

void FLensSolverWorkerCalibrate::DequeueLatch(FLatchData& latchData)
{
	latchQueue.Dequeue(latchData);
}

bool FLensSolverWorkerCalibrate::LatchInqueue()
{
	return latchQueue.IsEmpty() == false;
}
