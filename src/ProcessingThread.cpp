#include "ProcessingThread.h"
#include "CameraCapture.h"
#include "ISPPipeline.h"
#include "CalibrationEngine.h"
#include <QImage>
#include <QDir>
#include <QDateTime>
#include <opencv2/imgproc.hpp>

ProcessingThread::ProcessingThread(QObject* parent) 
    : QThread(parent) {}

ProcessingThread::~ProcessingThread() {
    stopCapture();
    wait();
}

void ProcessingThread::setCamera(std::shared_ptr<CameraCapture> camera) {
    std::lock_guard<std::mutex> lock(mutex_);
    camera_ = camera;
}

void ProcessingThread::setISPPipeline(std::shared_ptr<ISPPipeline> isp) {
    std::lock_guard<std::mutex> lock(mutex_);
    isp_pipeline_ = isp;
}

void ProcessingThread::setCalibrationEngine(std::shared_ptr<CalibrationEngine> calib) {
    std::lock_guard<std::mutex> lock(mutex_);
    calib_engine_ = calib;
}

void ProcessingThread::setProcessingMode(ProcessingMode mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    processing_mode_ = mode;
}

void ProcessingThread::setSaveDirectory(const std::string& directory) {
    std::lock_guard<std::mutex> lock(mutex_);
    save_directory_ = directory;
    
    // Create directory if it doesn't exist
    QDir dir(QString::fromStdString(directory));
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

void ProcessingThread::startCapture() {
    if (!capturing_) {
        capturing_ = true;
        stop_requested_ = false;
        start();
    }
}

void ProcessingThread::stopCapture() {
    if (capturing_) {
        capturing_ = false;
        stop_requested_ = true;
        condition_.wakeAll();
        wait();
    }
}

void ProcessingThread::captureCalibrationFrame() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (calib_engine_ && calib_engine_->getNumCalibrationImages() < max_calibration_frames_) {
        cv::Mat frame;
        if (camera_ && camera_->captureFrame(frame) && !frame.empty()) {
            // Process in calibration mode
            processFrame(frame);
        }
    }
}

void ProcessingThread::captureRawFrame() {
    std::lock_guard<std::mutex> lock(mutex_);
    cv::Mat frame;
    if (camera_ && camera_->captureFrame(frame) && !frame.empty()) {
        // Save raw frame
        QString filename = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
        QString filepath = QString::fromStdString(save_directory_) + 
                          "/raw_" + filename + ".png";
        cv::imwrite(filepath.toStdString(), frame);
    }
}

void ProcessingThread::run() {
    while (!stop_requested_) {
        mutex_.lock();
        
        if (!camera_ || !camera_->isInitialized()) {
            mutex_.unlock();
            msleep(100);
            continue;
        }
        
        cv::Mat frame;
        bool frame_captured = camera_->captureFrame(frame);
        
        mutex_.unlock();
        
        if (frame_captured && !frame.empty()) {
            processFrame(frame);
        }
        
        msleep(1); // Prevent CPU overuse
    }
    
    capturing_ = false;
}

void ProcessingThread::processFrame(const cv::Mat& frame) {
    cv::Mat processed;
    
    switch (processing_mode_) {
        case MODE_PREVIEW: {
            if (isp_pipeline_) {
                isp_pipeline_->processRGB(frame, processed);
            } else {
                processed = frame.clone();
            }
            break;
        }
        
        case MODE_CALIBRATION: {
            processed = frame.clone();
            
            if (calib_engine_) {
                // Try to find chessboard
                std::vector<cv::Point2f> corners;
                bool found = cv::findChessboardCorners(
                    frame, cv::Size(9, 6), corners,
                    cv::CALIB_CB_ADAPTIVE_THRESH + 
                    cv::CALIB_CB_NORMALIZE_IMAGE
                );
                
                if (found) {
                    cv::drawChessboardCorners(processed, cv::Size(9, 6), 
                                             corners, found);
                }
            }
            break;
        }
        
        case MODE_UNDISTORT: {
            if (calib_engine_ && calib_engine_->isCalibrated()) {
                calib_engine_->undistortImage(frame, processed);
            } else {
                processed = frame.clone();
            }
            break;
        }
        
        case MODE_RAW_CAPTURE: {
            processed = frame.clone();
            
            // Apply minimal processing for display
            if (isp_pipeline_) {
                isp_pipeline_->processRGB(frame, processed);
            }
            break;
        }
    }
    
    // Convert to QImage and emit signal
    QImage qimage = cvMatToQImage(processed);
    emit frameProcessed(qimage);
    
    // Handle calibration frame capture
    if (processing_mode_ == MODE_CALIBRATION && 
        calib_engine_ && 
        calib_engine_->getNumCalibrationImages() < max_calibration_frames_ &&
        frame_counter_ % 30 == 0) { // Capture every 30 frames
        
        if (calib_engine_->addCalibrationImage(frame, cv::Size(9, 6), 25.0f)) {
            emit calibrationFrameAdded(calib_engine_->getNumCalibrationImages());
        }
    }
    
    frame_counter_++;
}

QImage ProcessingThread::cvMatToQImage(const cv::Mat& mat) {
    if (mat.empty()) {
        return QImage();
    }
    
    switch (mat.type()) {
        case CV_8UC1: {
            QImage image(mat.data, mat.cols, mat.rows, 
                        static_cast<int>(mat.step), 
                        QImage::Format_Grayscale8);
            return image.copy();
        }
        
        case CV_8UC3: {
            QImage image(mat.data, mat.cols, mat.rows, 
                        static_cast<int>(mat.step), 
                        QImage::Format_BGR888);
            return image.copy();
        }
        
        case CV_8UC4: {
            QImage image(mat.data, mat.cols, mat.rows, 
                        static_cast<int>(mat.step), 
                        QImage::Format_ARGB32);
            return image.copy();
        }
        
        default: {
            cv::Mat converted;
            mat.convertTo(converted, CV_8UC3);
            QImage image(converted.data, converted.cols, converted.rows, 
                        static_cast<int>(converted.step), 
                        QImage::Format_BGR888);
            return image.copy();
        }
    }
}