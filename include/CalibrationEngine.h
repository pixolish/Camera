#pragma once

#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <vector>
#include <string>
#include <atomic>

class CalibrationEngine {
public:
    struct CalibrationResult {
        cv::Mat camera_matrix;
        cv::Mat distortion_coeffs;
        std::vector<cv::Mat> rvecs;
        std::vector<cv::Mat> tvecs;
        double reprojection_error;
        cv::Size image_size;
        cv::Size pattern_size;
        float square_size;
    };
    
    struct CalibrationFlags {
        bool fix_principal_point = false;
        bool zero_tangent_dist = true;
        bool fix_aspect_ratio = false;
        bool rational_model = false;
        bool thin_prism_model = false;
        bool fix_k1 = false;
        bool fix_k2 = false;
        bool fix_k3 = false;
        bool fix_k4 = false;
        bool fix_k5 = false;
        bool fix_k6 = false;
    };
    
    CalibrationEngine();
    ~CalibrationEngine();
    
    bool addCalibrationImage(const cv::Mat& image, 
                            cv::Size pattern_size, 
                            float square_size_mm);
    
    bool calibrate(CalibrationFlags flags = CalibrationFlags());
    
    void clearCalibrationData();
    
    bool saveCalibration(const std::string& filename);
    bool loadCalibration(const std::string& filename);
    
    void undistortImage(const cv::Mat& input, cv::Mat& output);
    
    const CalibrationResult& getResult() const { return result_; }
    size_t getNumCalibrationImages() const { return image_points_.size(); }
    bool isCalibrated() const { return calibrated_; }
    
    double computeReprojectionError();
    
    void drawChessboardCorners(cv::Mat& image, 
                              const std::vector<cv::Point2f>& corners,
                              cv::Size pattern_size,
                              bool pattern_found);
    
    static std::vector<cv::Point3f> generateObjectPoints(cv::Size pattern_size, 
                                                        float square_size);

private:
    bool findChessboardCorners(const cv::Mat& image, 
                              cv::Size pattern_size,
                              std::vector<cv::Point2f>& corners);
    
    CalibrationResult result_;
    std::vector<std::vector<cv::Point2f>> image_points_;
    std::vector<std::vector<cv::Point3f>> object_points_;
    cv::Size image_size_;
    
    std::atomic<bool> calibrated_{false};
    std::atomic<bool> calibration_in_progress_{false};
};