#pragma once

#include <QThread>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>
#include <opencv2/core.hpp>

class CameraCapture;
class ISPPipeline;
class CalibrationEngine;

class ProcessingThread : public QThread {
    Q_OBJECT
    
public:
    enum ProcessingMode {
        MODE_PREVIEW = 0,
        MODE_CALIBRATION,
        MODE_RAW_CAPTURE,
        MODE_UNDISTORT
    };
    
    ProcessingThread(QObject* parent = nullptr);
    ~ProcessingThread();
    
    void setCamera(std::shared_ptr<CameraCapture> camera);
    void setISPPipeline(std::shared_ptr<ISPPipeline> isp);
    void setCalibrationEngine(std::shared_ptr<CalibrationEngine> calib);
    
    void setProcessingMode(ProcessingMode mode);
    void setSaveDirectory(const std::string& directory);
    
    void startCapture();
    void stopCapture();
    void captureCalibrationFrame();
    void captureRawFrame();
    
    bool isCapturing() const { return capturing_; }
    
signals:
    void frameProcessed(const QImage& image);
    void calibrationFrameAdded(int count);
    void calibrationComplete(bool success, double error);
    void errorOccurred(const QString& message);
    
protected:
    void run() override;
    
private:
    void processFrame(const cv::Mat& frame);
    QImage cvMatToQImage(const cv::Mat& mat);
    
    std::shared_ptr<CameraCapture> camera_;
    std::shared_ptr<ISPPipeline> isp_pipeline_;
    std::shared_ptr<CalibrationEngine> calib_engine_;
    
    ProcessingMode processing_mode_ = MODE_PREVIEW;
    std::string save_directory_ = "./";
    
    std::atomic<bool> capturing_{false};
    std::atomic<bool> stop_requested_{false};
    
    QMutex mutex_;
    QWaitCondition condition_;
    
    int frame_counter_ = 0;
    const int max_calibration_frames_ = 20;
};