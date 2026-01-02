#!/bin/bash

# Install dependencies for Camera Calibration ISP
echo "Installing dependencies for Camera Calibration ISP..."

# Detect Linux distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    echo "Cannot detect Linux distribution"
    exit 1
fi

echo "Detected OS: $OS"

# Ubuntu/Debian
if [ "$OS" = "ubuntu" ] || [ "$OS" = "debian" ]; then
    echo "Installing packages for Ubuntu/Debian..."
    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        cmake \
        git \
        qt6-base-dev \
        qt6-multimedia-dev \
        qt6-multimediawidgets-dev \
        libopencv-dev \
        libv4l-dev \
        libtbb-dev \
        pkg-config
    
    # Check if Qt6 Multimedia is installed
    if ! dpkg -l | grep -q "qt6-multimedia-dev"; then
        echo "Warning: Qt6 Multimedia development packages not found"
        echo "Attempting to install from Qt repositories..."
        
        # Add Qt6 PPA for Ubuntu 22.04+
        sudo add-apt-repository -y ppa:beineri/opt-qt-6.5.0-jammy
        sudo apt-get update
        sudo apt-get install -y qt6-multimedia-dev
    fi

# Fedora
elif [ "$OS" = "fedora" ]; then
    echo "Installing packages for Fedora..."
    sudo dnf install -y \
        gcc-c++ \
        cmake \
        git \
        qt6-qtbase-devel \
        qt6-qtmultimedia-devel \
        opencv-devel \
        v4l-utils-devel \
        tbb-devel

# Arch Linux
elif [ "$OS" = "arch" ] || [ "$OS" = "manjaro" ]; then
    echo "Installing packages for Arch Linux..."
    sudo pacman -S --needed \
        base-devel \
        cmake \
        git \
        qt6-base \
        qt6-multimedia \
        opencv \
        v4l-utils \
        tbb

# OpenSUSE
elif [ "$OS" = "opensuse-tumbleweed" ] || [ "$OS" = "opensuse-leap" ]; then
    echo "Installing packages for OpenSUSE..."
    sudo zypper install -y \
        gcc-c++ \
        cmake \
        git \
        libqt6-qtbase-devel \
        libqt6-qtmultimedia-devel \
        opencv-devel \
        v4l-utils-devel \
        tbb-devel

else
    echo "Unsupported OS: $OS"
    echo "Please install the following packages manually:"
    echo "- cmake"
    echo "- gcc/g++"
    echo "- Qt6 Core and Multimedia"
    echo "- OpenCV"
    echo "- v4l-utils"
    exit 1
fi

echo "Dependencies installed successfully!"
echo ""
echo "To build the project:"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  make -j$(nproc)"
echo ""
echo "To run the application:"
echo "  ./CameraCalibrationISP"