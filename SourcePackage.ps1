$editorCmd = [IO.File]::ReadAllText(".\EnginePath.txt")
echo "Editor path: $editorCmd"

$projectRoot = Get-Location
$currentLocation = $projectRoot
$outputPath = "D:\Distribution\Source\"

echo "Packaging LensCalibrator plugin..."
& "$editorCmd\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="$currentLocation/LensCalibrator.uplugin" -TargetPlatforms=Win64 -Package="$outputPath/LensCalibrator" -Rocket

Copy-Item -Recurse -Force -Path .\License.txt -Destination "$outputPath\LensCalibrator\License.txt"
New-Item -Force -ItemType Directory "$outputPath\LensCalibrator\Source\ThirdParty\"
New-Item -Force -ItemType Directory "$outputPath\LensCalibrator\Source\ThirdParty\OpenCV\Include\"
New-Item -Force -ItemType Directory "$outputPath\LensCalibrator\Source\ThirdParty\OpenCV\Binaries\"
New-Item -Force -ItemType Directory "$outputPath\LensCalibrator\Binaries\ThirdParty\Win64\"

Copy-Item -Recurse -Force -Path .\Source\ThirdParty\OpenCV\OpenCV.txt -Destination "$outputPath\LensCalibrator\Source\ThirdParty\OpenCV.txt"
Copy-Item -Recurse -Force -Path .\Source\ThirdParty\OpenCV\Include\ -Destination "$outputPath\LensCalibrator\Source\ThirdParty\OpenCV\Include\"
# Copy-Item -Recurse -Force -Path .\Source\ThirdParty\OpenCV\Binaries\Debug\Dynamic\* -Destination "$outputPath\LensCalibrator\Binaries\Win64\"
# Copy-Item -Recurse -Force -Path .\Source\ThirdParty\OpenCV\Binaries\Release\Dynamic\* -Destination "$outputPath\LensCalibrator\Binaries\Win64\"
Copy-Item -Recurse -Force -Path .\Source\ThirdParty\OpenCV\Binaries\ -Destination "$outputPath\Source\ThirdParty\OpenCV\Binaries\"

# Remove-Item -Recurse -Force -Path "$outputPath\LensCalibrator\Source\LensCalibrator\Private"
# Remove-Item -Recurse -Force -Path "$outputPath\LensCalibrator\Intermediate"
# Remove-Item -Recurse -Force -Path "$outputPath\LensCalibrator\Binaries\Win64\UE4Editor-LensCalibrator.pdb"

pause
