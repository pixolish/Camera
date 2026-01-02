cmake_minimum_required(VERSION 3.16)
project(CameraCalibrationISP VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Qt6 Core first
find_package(Qt6 COMPONENTS Core REQUIRED)

# Try to find Multimedia, but make it optional
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 QUIET COMPONENTS Multimedia MultimediaWidgets)

# Find OpenCV
find_package(OpenCV REQUIRED)

# If OpenCV components not found, try to find them separately
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found. Please install OpenCV development packages.")
endif()

find_package(Threads REQUIRED)

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
    
    # Find DirectShow if available
    find_path(DSHOW_INCLUDE_DIR dshow.h)
    if(DSHOW_INCLUDE_DIR)
        add_definitions(-DHAVE_DSHOW)
        include_directories(${DSHOW_INCLUDE_DIR})
    endif()
    
elseif(UNIX AND NOT APPLE)
    # Linux-specific settings
    # Check for V4L2 headers
    include(CheckIncludeFile)
    check_include_file(linux/videodev2.h HAVE_V4L2_HEADER)
    
    if(HAVE_V4L2_HEADER)
        message(STATUS "Found V4L2 headers")
        add_definitions(-DHAVE_V4L2)
        
        # Check for libv4l2 library
        find_library(V4L2_LIBRARY NAMES v4l2)
        if(V4L2_LIBRARY)
            message(STATUS "Found V4L2 library: ${V4L2_LIBRARY}")
        else()
            message(STATUS "V4L2 library not found, using system calls")
        endif()
    else()
        message(WARNING "V4L2 headers not found. Camera support may be limited.")
    endif()
    
    # Additional Linux libraries
    set(EXTRA_LIBS ${EXTRA_LIBS} pthread)
endif()

# Add project source files
set(SOURCES
    src/CameraCapture.cpp
    src/ISPPipeline.cpp
    src/CalibrationEngine.cpp
    src/ProcessingThread.cpp
    src/MainWindow.cpp
    main.cpp
)

set(HEADERS
    include/CameraCapture.h
    include/ISPPipeline.h
    include/CalibrationEngine.h
    include/ProcessingThread.h
    include/MainWindow.h
)

# Add resources
set(RESOURCES resources/icons.qrc)

# Create executable
add_executable(CameraCalibrationISP ${SOURCES} ${HEADERS} ${RESOURCES})

# Link libraries
target_link_libraries(CameraCalibrationISP
    Qt6::Core
    Qt6::Widgets
    ${OpenCV_LIBS}
    Threads::Threads
    ${EXTRA_LIBS}
)

# Conditionally link multimedia components
if(Qt6Multimedia_FOUND)
    target_link_libraries(CameraCalibrationISP Qt6::Multimedia)
endif()

if(Qt6MultimediaWidgets_FOUND)
    target_link_libraries(CameraCalibrationISP Qt6::MultimediaWidgets)
endif()

# Link V4L2 if found
if(V4L2_LIBRARY)
    target_link_libraries(CameraCalibrationISP ${V4L2_LIBRARY})
endif()

# Include directories
target_include_directories(CameraCalibrationISP PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Qt6Core_INCLUDE_DIRS}
    ${Qt6Widgets_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

# Add definitions for optional features
if(Qt6Multimedia_FOUND)
    target_compile_definitions(CameraCalibrationISP PRIVATE HAVE_QT_MULTIMEDIA)
endif()

if(HAVE_V4L2_HEADER)
    target_compile_definitions(CameraCalibrationISP PRIVATE HAVE_V4L2)
endif()

# Set properties for Windows
if(WIN32)
    set_target_properties(CameraCalibrationISP PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Install target (optional)
install(TARGETS CameraCalibrationISP
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)