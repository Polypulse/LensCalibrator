# Lens Calibrator - Unreal Engine Plugin
The purpose of this plugin is to provide an easy method of solving for the physical lens characteristics of a camera. These characteristics include:
- Lens Calibration from MediaPlayer, MediaBundle and MediaTexture.
- Lens Calibration from Folders of images.
- Lens Distortion Correction with interpolation between camera zoom levels in composure.
- Fast setup and and asynchronous calibration procedure.
- Runtime and Editor Blueprint support.

[Discord Server](https://discord.gg/QfyreCkC) - for asking questions.

[Documentation](https://docs.google.com/document/d/1HNYupII7wy-lZkVENv9LS7f7xqjnMhFPVWmATGc8Xoc/edit?usp=sharing) - Maybe this will eventually be refactored into the readme.

## Trailer
[![Lens Calibrator - Unreal Engine - Trailer](https://img.youtube.com/vi/HfIi4gkH9Es/0.jpg)](https://www.youtube.com/watch?v=HfIi4gkH9Es)

## Full Walkthrough (~40 min)
If documentation is preferred that can be found [here](https://docs.google.com/document/d/1HNYupII7wy-lZkVENv9LS7f7xqjnMhFPVWmATGc8Xoc/edit?usp=sharing).

[![Lens Calibrator - Full Walkthrough](https://img.youtube.com/vi/CBvmpoI9hAs/0.jpg)](https://www.youtube.com/watch?v=CBvmpoI9hAs)

## Supported Unreal Engine Versions
- 4.26
- 4.24
- 4.25
- 4.26

# Plugin Installation
## Easy Mode
The easiest way to install the plugin is to grab one of the [release binaries](https://github.com/Polypulse/LensCalibrator/releases), then extracting and copying it into the *Plugins* folder in the root of your UE4 project. You can also recursively clone the [LensCalibratorProject](https://github.com/Polypulse/LensCalibratorProject) repository which is UE4 project for testing.

## Source Code Mode
1. Navigate to the UE4 project that you want to install the plugin into.
2. Make a *Plugins* folder if it does not already exist in the root of your UE4 project.
3. Open up a terminal and ```cd``` into the Plugin folder you created.
4. Clone the repository via: ```git clone --recursive git@github.com:Polypulse/LensCalibrator.git```.
5. After cloning the repository, open an explorer window at the repo path.
6. Hold shift, right click in the window and select *Open Powershell window here*.

![Powershell Open](./Resources/openpowershell.png)

7. Execute ```./PullDependencies.ps1``` or download the [OpenCVWrapper](https://github.com/Polypulse/OpenCVWrapper/releases) dependency and extract OpenCVWrapper.zip into ```LensCalibratorProject/Plugins/lenscalibrator/Source/ThirdParty/OpenCVWrapper/```
8. Then open your UE4 project.

## Documentation
Currently the plugin's documentation is located in this [document](https://docs.google.com/document/d/1HNYupII7wy-lZkVENv9LS7f7xqjnMhFPVWmATGc8Xoc/edit?usp=sharing). However, in the future this may be refatored into the readme file.

Link: [https://docs.google.com/document/d/1HNYupII7wy-lZkVENv9LS7f7xqjnMhFPVWmATGc8Xoc/edit?usp=sharing](https://docs.google.com/document/d/1HNYupII7wy-lZkVENv9LS7f7xqjnMhFPVWmATGc8Xoc/edit?usp=sharing)

## Architecture

### Overview Communication Pipe
In order to avoid DLL boundary data curroption between UE4 objects and standard library objects, the OpenCVWrapper acts as an interface to OpenCV and converts any data going to and from.

![Communication Pipe](./Resources/dllboundary.png)
