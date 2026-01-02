Camera ISP & Calibration System
Organization: Pixolish System
Project Title: ISP Camera Calibration
Version: 1.0.0

Overview
A comprehensive, optimized C++ application for Image Signal Processing (ISP) and camera calibration with Qt GUI framework. Supports USB/UVC camera streaming on both Linux and Windows platforms.

Features
🎯 Core Functionality
Multi-platform Camera Support: Windows (DirectShow) and Linux (V4L2)

Real-time ISP Pipeline: Full image processing chain from raw to display

Camera Calibration: Intrinsic/extrinsic parameter estimation

Undistortion: Lens distortion correction

📷 Camera Features
USB/UVC camera support

Multi-camera detection and selection

Resolution and FPS control

Camera property adjustment (exposure, gain, white balance)

Auto/manual camera backend selection

🖥️ Image Processing Pipeline
Demosaicing: Bilinear, VNG, and AHD algorithms

White Balance: Auto and manual RGB gain control

Color Correction: 3x3 color matrix transformation

Gamma Correction: Customizable gamma curves

Tone Mapping: Exposure, contrast, brightness controls

Noise Reduction: Fast non-local means denoising

Sharpening: Unsharp masking with adjustable strength

Lens Correction: Distortion and tangential correction

🔧 Calibration Tools
Chessboard pattern detection

Intrinsic camera parameter estimation

Distortion coefficient calculation

Reprojection error analysis

Save/load calibration profiles

Real-time undistorted preview

🎨 GUI Features
Qt6-based modern interface

Real-time camera preview

ISP parameter adjustment panels

Calibration frame capture and management

Multi-tab interface for different functions

Responsive design with icon support

System Requirements
Minimum Requirements
CPU: x86_64, 2+ cores

RAM: 4GB

Storage: 2GB free space

OS: Windows 10/11 or Linux (kernel 4.15+)

Camera: USB 2.0 UVC-compliant camera

Recommended Requirements
CPU: 4+ cores, Intel i5/Ryzen 5 or better

RAM: 8GB+

Storage: SSD with 10GB free space

GPU: Dedicated graphics for OpenCV acceleration

Camera: USB 3.0 camera for higher resolutions

Quick Start
Linux Installation
bash
# Clone the repository
git clone https://github.com/pixolish-system/isp-camera-calibration.git
cd isp-camera-calibration

# Install dependencies
chmod +x install_linux_deps.sh
./install_linux_deps.sh

# Build the application
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Run the application
./CameraCalibrationISP
Windows Installation
Install Visual Studio 2022 with C++ support

Install Qt 6.6+ via Qt Online Installer

Install OpenCV 4.8+

Use CMake GUI to configure and generate project files

Build in Visual Studio

Dependencies
Linux
bash
# Ubuntu/Debian
sudo apt install build-essential cmake qt6-base-dev \
    qt6-multimedia-dev libopencv-dev libv4l-dev libtbb-dev

# Fedora/RHEL
sudo dnf install gcc-c++ cmake qt6-qtbase-devel \
    qt6-qtmultimedia-devel opencv-devel v4l-utils-devel tbb-devel

# Arch Linux
sudo pacman -S base-devel cmake qt6-base qt6-multimedia \
    opencv v4l-utils tbb
Windows
Visual Studio 2022

Qt 6.6+

OpenCV 4.8+

Windows SDK

Usage Guide
1. Camera Setup
Connect your USB camera

Click "Refresh" to detect available cameras

Select camera and desired resolution

Click "Start" to begin streaming

2. ISP Adjustment
Navigate to "ISP Settings" tab

Adjust white balance (auto or manual)

Modify exposure, contrast, and brightness

Enable/disable denoising and sharpening

Adjust gamma correction as needed

3. Camera Calibration
Print a chessboard pattern (9x6 recommended)

Switch to "Calibration" mode

Capture multiple frames (15-20 recommended) from different angles

Click "Calibrate" to compute camera parameters

Save calibration for future use

4. Undistorted View
Load a saved calibration

Switch to "Undistort" mode

View real-time lens-corrected output

Project Structure
text
isp-camera-calibration/
├── CMakeLists.txt              # Build configuration
├── main.cpp                    # Application entry point
├── include/                    # Header files
│   ├── CameraCapture.h        # Camera abstraction layer
│   ├── ISPPipeline.h          # ISP processing pipeline
│   ├── CalibrationEngine.h    # Camera calibration logic
│   ├── ProcessingThread.h     # Background processing
│   └── MainWindow.h          # GUI interface
├── src/                       # Source files
│   ├── CameraCapture.cpp
│   ├── ISPPipeline.cpp
│   ├── CalibrationEngine.cpp
│   ├── ProcessingThread.cpp
│   └── MainWindow.cpp
├── resources/                 # Application resources
│   ├── icons.qrc             # Qt resource file
│   └── icons/               # SVG/PNG icons
├── dependencies/             # Platform-specific dependencies
│   ├── linux/
│   │   └── README.md
│   └── windows/
│       └── README.md
└── README.md                # This file
Architecture
Camera Abstraction Layer
Platform-specific implementations (V4L2, DirectShow, OpenCV)

Unified interface for camera control

Automatic backend fallback

ISP Pipeline
Modular processing stages

Real-time parameter adjustment

Hardware acceleration where available

Calibration Engine
Chessboard detection using OpenCV

Zhang's calibration method

Comprehensive error analysis

GUI Framework
Qt6 for cross-platform compatibility

Multi-threaded processing

Responsive user interface

API Documentation
CameraCapture Class
cpp
// Initialize camera
bool initialize(int camera_id, int width, int height, int fps);

// Capture frame
bool captureFrame(cv::Mat& frame);

// Camera control
bool setResolution(int width, int height);
bool setExposure(int exposure);
bool setWhiteBalance(int red, int green, int blue);

// Camera enumeration
std::vector<CameraInfo> listAvailableCameras();
ISPPipeline Class
cpp
// Process image
void processRGB(const cv::Mat& input, cv::Mat& output);

// Parameter management
void setParameters(const ISPParameters& params);
ISPParameters& getParameters();

// Calibration utilities
void calibrateWhiteBalance(const cv::Mat& gray_image);
void loadColorMatrix(const std::string& filename);
CalibrationEngine Class
cpp
// Add calibration image
bool addCalibrationImage(const cv::Mat& image, cv::Size pattern_size, float square_size);

// Perform calibration
bool calibrate(CalibrationFlags flags = CalibrationFlags());

// Save/load calibration
bool saveCalibration(const std::string& filename);
bool loadCalibration(const std::string& filename);

// Undistort image
void undistortImage(const cv::Mat& input, cv::Mat& output);
Building from Source
Linux
bash
# Clone repository
git clone https://github.com/pixolish-system/isp-camera-calibration.git
cd isp-camera-calibration

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
make -j$(nproc)

# Install (optional)
sudo make install
Windows
powershell
# Using CMake GUI
1. Set source directory to project root
2. Set build directory to 'build'
3. Configure with Visual Studio 2022
4. Generate project files
5. Open solution in Visual Studio
6. Build Release configuration

# Or using command line
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
Performance Optimization
Compiler Optimizations
bash
# Enable maximum optimizations
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native" \
      ..
OpenCV Optimizations
Enable TBB for multi-threading

Use IPP (Intel Performance Primitives) if available

Enable NEON/AVX instructions for ARM/x86

Memory Management
Reuse buffers where possible

Pre-allocate memory for frequent operations

Use move semantics for large data transfers

Troubleshooting
Common Issues
Camera Not Detected
bash
# Check device permissions
ls -la /dev/video*

# Add user to video group
sudo usermod -a -G video $USER
# Log out and log back in

# Test with v4l2-ctl
v4l2-ctl --list-devices
Qt6 Multimedia Not Found
bash
# Install Qt6 Multimedia development package
sudo apt install qt6-multimedia-dev  # Ubuntu
sudo dnf install qt6-qtmultimedia-devel  # Fedora
OpenCV Version Issues
bash
# Check OpenCV version
pkg-config --modversion opencv4

# Reinstall if necessary
sudo apt install libopencv-dev  # Ubuntu
Build Errors
bash
# Clean build directory
rm -rf build
mkdir build && cd build

# Verbose build output
cmake -DCMAKE_VERBOSE_MAKEFILE=ON ..
make VERBOSE=1
Debug Mode
bash
# Build with debug symbols
mkdir debug && cd debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Run with debug output
./CameraCalibrationISP --debug
Contributing
We welcome contributions! Please follow these steps:

Fork the repository

Create a feature branch

Make your changes

Add tests if applicable

Submit a pull request

Development Guidelines
Follow C++17 standards

Use clang-format for code formatting

Add documentation for new features

Update README.md as needed

License
This project is licensed under the MIT License - see the LICENSE file for details.

Support
For support, please:

Check the GitHub Issues

Review the troubleshooting section

Create a new issue with detailed information

Acknowledgments
OpenCV community for computer vision libraries

Qt Company for the GUI framework

Contributors and testers

Pixolish System - Transforming pixels into perfection

For more information, visit our GitHub repository



