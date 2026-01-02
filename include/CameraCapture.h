#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <functional>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <sys/mman.h>

#ifdef _WIN32
#include <windows.h>
#include <dshow.h>
#pragma comment(lib, "strmiids.lib")
#else
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#endif

class CameraCapture {
public:
    enum CaptureBackend {
        AUTO = 0,
        V4L2,
        DSHOW,
        OPENCV
    };

    struct CameraInfo {
        int id;
        std::string name;
        std::vector<std::pair<int, int>> resolutions;
    };

    CameraCapture();
    ~CameraCapture();

    bool initialize(int camera_id = 0, 
                   int width = 640, 
                   int height = 480, 
                   int fps = 30,
                   CaptureBackend backend = AUTO);
    
    void shutdown();
    
    bool captureFrame(cv::Mat& frame);
    
    bool setResolution(int width, int height);
    bool setFPS(int fps);
    bool setExposure(int exposure);
    bool setGain(int gain);
    bool setWhiteBalance(int red, int green, int blue);
    
    std::vector<CameraInfo> listAvailableCameras();
    
    bool isInitialized() const { return initialized_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    int getFPS() const { return fps_; }

private:
#ifdef _WIN32
    bool initDirectShow();
    void cleanupDirectShow();
    IGraphBuilder* graph_builder_ = nullptr;
    IMediaControl* media_control_ = nullptr;
    IMediaEvent* media_event_ = nullptr;
    ISampleGrabber* sample_grabber_ = nullptr;
    IBaseFilter* capture_filter_ = nullptr;
    AM_MEDIA_TYPE media_type_;
    HANDLE frame_event_ = nullptr;
#else
    bool initV4L2();
    void cleanupV4L2();
    int v4l2_fd_ = -1;
    void* v4l2_buffer_ = nullptr;
    size_t v4l2_buffer_size_ = 0;
#endif

    bool initOpenCV();
    void cleanupOpenCV();
    
    cv::VideoCapture* opencv_cap_ = nullptr;
    
    CaptureBackend backend_ = AUTO;
    bool initialized_ = false;
    std::atomic<bool> running_{false};
    
    int camera_id_ = 0;
    int width_ = 640;
    int height_ = 480;
    int fps_ = 30;
    
    cv::Mat current_frame_;
    std::mutex frame_mutex_;
};
