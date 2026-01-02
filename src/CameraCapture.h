#include "CameraCapture.h"
#include <iostream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <comdef.h>
#include <atlbase.h>
#include <qedit.h>
#pragma comment(lib, "quartz.lib")

class SampleGrabberCallback : public ISampleGrabberCB {
public:
    SampleGrabberCallback(CameraCapture* parent) : parent_(parent) {}
    
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) {
        if (riid == IID_IUnknown || riid == IID_ISampleGrabberCB) {
            *ppv = static_cast<ISampleGrabberCB*>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    
    STDMETHODIMP_(ULONG) AddRef() { return 1; }
    STDMETHODIMP_(ULONG) Release() { return 1; }
    
    STDMETHODIMP SampleCB(double time, IMediaSample* sample) {
        BYTE* buffer = nullptr;
        sample->GetPointer(&buffer);
        
        if (buffer && parent_) {
            std::lock_guard<std::mutex> lock(parent_->frame_mutex_);
            parent_->current_frame_ = cv::Mat(parent_->height_, parent_->width_, 
                                            CV_8UC3, buffer).clone();
        }
        return S_OK;
    }
    
    STDMETHODIMP BufferCB(double time, BYTE* buffer, long length) {
        if (buffer && parent_ && length > 0) {
            std::lock_guard<std::mutex> lock(parent_->frame_mutex_);
            parent_->current_frame_ = cv::Mat(parent_->height_, parent_->width_, 
                                            CV_8UC3, buffer).clone();
        }
        return S_OK;
    }
    
private:
    CameraCapture* parent_;
};
#endif

CameraCapture::CameraCapture() {
#ifdef _WIN32
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif
}

CameraCapture::~CameraCapture() {
    shutdown();
#ifdef _WIN32
    CoUninitialize();
#endif
}

bool CameraCapture::initialize(int camera_id, int width, int height, 
                              int fps, CaptureBackend backend) {
    shutdown();
    
    camera_id_ = camera_id;
    width_ = width;
    height_ = height;
    fps_ = fps;
    backend_ = backend;
    
    bool success = false;
    
    if (backend == AUTO) {
        // Try different backends in order of preference
#ifdef _WIN32
        success = initDirectShow();
        if (!success) success = initOpenCV();
#else
        success = initV4L2();
        if (!success) success = initOpenCV();
#endif
    } else {
        switch (backend) {
#ifdef _WIN32
            case DSHOW:
                success = initDirectShow();
                break;
#endif
#ifndef _WIN32
            case V4L2:
                success = initV4L2();
                break;
#endif
            case OPENCV:
                success = initOpenCV();
                break;
            default:
                break;
        }
    }
    
    initialized_ = success;
    running_ = success;
    
    return success;
}

#ifdef _WIN32
bool CameraCapture::initDirectShow() {
    HRESULT hr;
    
    // Create filter graph
    hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                         IID_IFilterGraph2, reinterpret_cast<void**>(&graph_builder_));
    if (FAILED(hr)) return false;
    
    // Create capture graph builder
    CComPtr<ICaptureGraphBuilder2> capture_builder;
    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER,
                         IID_ICaptureGraphBuilder2, reinterpret_cast<void**>(&capture_builder));
    if (FAILED(hr)) return false;
    
    capture_builder->SetFiltergraph(graph_builder_);
    
    // Get media control
    hr = graph_builder_->QueryInterface(IID_IMediaControl, 
                                       reinterpret_cast<void**>(&media_control_));
    if (FAILED(hr)) return false;
    
    // Create sample grabber
    CComPtr<IBaseFilter> grabber_filter;
    hr = CoCreateInstance(CLSID_SampleGrabber, nullptr, CLSCTX_INPROC_SERVER,
                         IID_IBaseFilter, reinterpret_cast<void**>(&grabber_filter));
    if (FAILED(hr)) return false;
    
    hr = graph_builder_->AddFilter(grabber_filter, L"Sample Grabber");
    if (FAILED(hr)) return false;
    
    hr = grabber_filter->QueryInterface(IID_ISampleGrabber, 
                                       reinterpret_cast<void**>(&sample_grabber_));
    if (FAILED(hr)) return false;
    
    // Set media type
    ZeroMemory(&media_type_, sizeof(media_type_));
    media_type_.majortype = MEDIATYPE_Video;
    media_type_.subtype = MEDIASUBTYPE_RGB24;
    media_type_.formattype = FORMAT_VideoInfo;
    
    hr = sample_grabber_->SetMediaType(&media_type_);
    if (FAILED(hr)) return false;
    
    // Set callback
    SampleGrabberCallback* callback = new SampleGrabberCallback(this);
    hr = sample_grabber_->SetCallback(callback, 1);
    if (FAILED(hr)) return false;
    
    // Create frame event
    frame_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    
    // Build the graph (simplified - needs proper device enumeration)
    // In production, you would enumerate devices and build proper graph
    
    return true;
}

void CameraCapture::cleanupDirectShow() {
    if (media_control_) {
        media_control_->Stop();
        media_control_->Release();
        media_control_ = nullptr;
    }
    
    if (sample_grabber_) {
        sample_grabber_->Release();
        sample_grabber_ = nullptr;
    }
    
    if (graph_builder_) {
        graph_builder_->Release();
        graph_builder_ = nullptr;
    }
    
    if (frame_event_) {
        CloseHandle(frame_event_);
        frame_event_ = nullptr;
    }
}
#else
bool CameraCapture::initV4L2() {
    std::string device_path = "/dev/video" + std::to_string(camera_id_);
    
    v4l2_fd_ = open(device_path.c_str(), O_RDWR | O_NONBLOCK);
    if (v4l2_fd_ < 0) {
        return false;
    }
    
    // Query capabilities
    v4l2_capability cap;
    if (ioctl(v4l2_fd_, VIDIOC_QUERYCAP, &cap) < 0) {
        close(v4l2_fd_);
        v4l2_fd_ = -1;
        return false;
    }
    
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        close(v4l2_fd_);
        v4l2_fd_ = -1;
        return false;
    }
    
    // Set format
    v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width_;
    format.fmt.pix.height = height_;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    format.fmt.pix.field = V4L2_FIELD_NONE;
    
    if (ioctl(v4l2_fd_, VIDIOC_S_FMT, &format) < 0) {
        close(v4l2_fd_);
        v4l2_fd_ = -1;
        return false;
    }
    
    // Request buffers
    v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    
    if (ioctl(v4l2_fd_, VIDIOC_REQBUFS, &req) < 0) {
        close(v4l2_fd_);
        v4l2_fd_ = -1;
        return false;
    }
    
    // Map buffers
    v4l2_buffer buffer;
    for (int i = 0; i < req.count; ++i) {
        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = i;
        
        if (ioctl(v4l2_fd_, VIDIOC_QUERYBUF, &buffer) < 0) {
            cleanupV4L2();
            return false;
        }
        
        v4l2_buffer_ = mmap(nullptr, buffer.length, 
                           PROT_READ | PROT_WRITE, MAP_SHARED,
                           v4l2_fd_, buffer.m.offset);
        
        if (v4l2_buffer_ == MAP_FAILED) {
            cleanupV4L2();
            return false;
        }
        
        v4l2_buffer_size_ = buffer.length;
        
        // Queue buffer
        if (ioctl(v4l2_fd_, VIDIOC_QBUF, &buffer) < 0) {
            cleanupV4L2();
            return false;
        }
    }
    
    // Start streaming
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(v4l2_fd_, VIDIOC_STREAMON, &type) < 0) {
        cleanupV4L2();
        return false;
    }
    
    return true;
}

void CameraCapture::cleanupV4L2() {
    if (v4l2_fd_ >= 0) {
        if (v4l2_buffer_) {
            munmap(v4l2_buffer_, v4l2_buffer_size_);
            v4l2_buffer_ = nullptr;
            v4l2_buffer_size_ = 0;
        }
        
        close(v4l2_fd_);
        v4l2_fd_ = -1;
    }
}
#endif

bool CameraCapture::initOpenCV() {
    try {
        opencv_cap_ = new cv::VideoCapture(camera_id_);
        if (!opencv_cap_->isOpened()) {
            delete opencv_cap_;
            opencv_cap_ = nullptr;
            return false;
        }
        
        opencv_cap_->set(cv::CAP_PROP_FRAME_WIDTH, width_);
        opencv_cap_->set(cv::CAP_PROP_FRAME_HEIGHT, height_);
        opencv_cap_->set(cv::CAP_PROP_FPS, fps_);
        
        return true;
    } catch (...) {
        if (opencv_cap_) {
            delete opencv_cap_;
            opencv_cap_ = nullptr;
        }
        return false;
    }
}

void CameraCapture::cleanupOpenCV() {
    if (opencv_cap_) {
        opencv_cap_->release();
        delete opencv_cap_;
        opencv_cap_ = nullptr;
    }
}

void CameraCapture::shutdown() {
    running_ = false;
    
#ifdef _WIN32
    if (backend_ == DSHOW || backend_ == AUTO) {
        cleanupDirectShow();
    }
#else
    if (backend_ == V4L2 || backend_ == AUTO) {
        cleanupV4L2();
    }
#endif
    
    if (backend_ == OPENCV || backend_ == AUTO) {
        cleanupOpenCV();
    }
    
    initialized_ = false;
}

bool CameraCapture::captureFrame(cv::Mat& frame) {
    if (!initialized_ || !running_) {
        return false;
    }
    
#ifdef _WIN32
    if (backend_ == DSHOW && sample_grabber_) {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        if (!current_frame_.empty()) {
            frame = current_frame_.clone();
            return true;
        }
        return false;
    }
#else
    if (backend_ == V4L2 && v4l2_fd_ >= 0) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(v4l2_fd_, &fds);
        
        timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        
        int r = select(v4l2_fd_ + 1, &fds, nullptr, nullptr, &tv);
        if (r == -1) {
            return false;
        }
        
        v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        
        if (ioctl(v4l2_fd_, VIDIOC_DQBUF, &buffer) < 0) {
            return false;
        }
        
        frame = cv::Mat(height_, width_, CV_8UC3, v4l2_buffer_).clone();
        
        if (ioctl(v4l2_fd_, VIDIOC_QBUF, &buffer) < 0) {
            return false;
        }
        
        return true;
    }
#endif
    
    if (backend_ == OPENCV && opencv_cap_) {
        return opencv_cap_->read(frame);
    }
    
    return false;
}

std::vector<CameraCapture::CameraInfo> CameraCapture::listAvailableCameras() {
    std::vector<CameraInfo> cameras;
    
    // Try OpenCV backend for camera enumeration
    for (int i = 0; i < 10; ++i) {
        cv::VideoCapture cap(i);
        if (cap.isOpened()) {
            CameraInfo info;
            info.id = i;
            info.name = "Camera " + std::to_string(i);
            
            // Try common resolutions
            std::vector<std::pair<int, int>> resolutions = {
                {640, 480}, {800, 600}, {1024, 768}, 
                {1280, 720}, {1920, 1080}
            };
            
            for (const auto& res : resolutions) {
                if (cap.set(cv::CAP_PROP_FRAME_WIDTH, res.first) &&
                    cap.set(cv::CAP_PROP_FRAME_HEIGHT, res.second)) {
                    info.resolutions.push_back(res);
                }
            }
            
            cameras.push_back(info);
            cap.release();
        }
    }
    
    return cameras;
}
