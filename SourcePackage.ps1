$editorCmd = [IO.File]::ReadAllText(".\EnginePath.txt")
echo "Editor path: $editorCmd"

$projectRoot = Get-Location
$currentLocation = $projectRoot
$outputPath = "D:\Distribution\Source\"

echo "Packaging LensCalibrator plugin..."
& "$editorCmd\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="$currentLocation/LensCalibrator.uplugin" -TargetPlatforms=Win64 -Package="$outputPath/LensCalibrator" -Rocket -VS2019

Copy-Item -Recurse -Force -Path .\License.txt -Destination "$outputPath\LensCalibrator\License.txt"

New-Item -Force -ItemType Directory "$outputPath\LensCalibrator\Source\ThirdParty\"
New-Item -Force -ItemType Directory "$outputPath\LensCalibrator\Config\"

Copy-Item -Recurse -Force -Path .\Source\ThirdParty\OpenCVWrapper\OpenCV\LICENSE -Destination "$outputPath\LensCalibrator\Source\ThirdParty\OpenCVWrapper\OpenCV\LICENSE"
Copy-Item -Recurse -Force -Path .\Config\FilterPlugin.ini -Destination "$outputPath\LensCalibrator\Config\FilterPlugin.ini"

pause
