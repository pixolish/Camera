#include "CalibrationEngine.h"
#include <iostream>
#include <fstream>
#include <iomanip>

CalibrationEngine::CalibrationEngine() {
    result_.camera_matrix = cv::Mat::eye(3, 3, CV_64F);
    result_.distortion_coeffs = cv::Mat::zeros(8, 1, CV_64F);
}

CalibrationEngine::~CalibrationEngine() {}

bool CalibrationEngine::addCalibrationImage(const cv::Mat& image, 
                                           cv::Size pattern_size, 
                                           float square_size_mm) {
    if (image.empty()) return false;
    
    std::vector<cv::Point2f> corners;
    bool found = findChessboardCorners(image, pattern_size, corners);
    
    if (found) {
        // Refine corner positions
        cv::Mat gray;
        if (image.channels() == 3) {
            cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = image.clone();
        }
        
        cv::cornerSubPix(gray, corners, cv::Size(11, 11), 
                        cv::Size(-1, -1),
                        cv::TermCriteria(cv::TermCriteria::EPS + 
                                        cv::TermCriteria::MAX_ITER, 30, 0.1));
        
        image_points_.push_back(corners);
        object_points_.push_back(generateObjectPoints(pattern_size, square_size_mm));
        
        if (image_size_.width == 0) {
            image_size_ = image.size();
            result_.image_size = image_size_;
        }
        
        result_.pattern_size = pattern_size;
        result_.square_size = square_size_mm;
        
        return true;
    }
    
    return false;
}

bool CalibrationEngine::calibrate(CalibrationFlags flags) {
    if (image_points_.size() < 5) {
        std::cerr << "Need at least 5 calibration images" << std::endl;
        return false;
    }
    
    calibration_in_progress_ = true;
    
    int calib_flags = 0;
    
    if (flags.fix_principal_point) calib_flags |= cv::CALIB_FIX_PRINCIPAL_POINT;
    if (flags.zero_tangent_dist) calib_flags |= cv::CALIB_ZERO_TANGENT_DIST;
    if (flags.fix_aspect_ratio) calib_flags |= cv::CALIB_FIX_ASPECT_RATIO;
    if (flags.rational_model) calib_flags |= cv::CALIB_RATIONAL_MODEL;
    if (flags.thin_prism_model) calib_flags |= cv::CALIB_THIN_PRISM_MODEL;
    if (flags.fix_k1) calib_flags |= cv::CALIB_FIX_K1;
    if (flags.fix_k2) calib_flags |= cv::CALIB_FIX_K2;
    if (flags.fix_k3) calib_flags |= cv::CALIB_FIX_K3;
    if (flags.fix_k4) calib_flags |= cv::CALIB_FIX_K4;
    if (flags.fix_k5) calib_flags |= cv::CALIB_FIX_K5;
    if (flags.fix_k6) calib_flags |= cv::CALIB_FIX_K6;
    
    try {
        result_.reprojection_error = cv::calibrateCamera(
            object_points_, image_points_, image_size_,
            result_.camera_matrix, result_.distortion_coeffs,
            result_.rvecs, result_.tvecs,
            calib_flags,
            cv::TermCriteria(cv::TermCriteria::COUNT + 
                            cv::TermCriteria::EPS, 30, DBL_EPSILON)
        );
        
        calibrated_ = true;
        calibration_in_progress_ = false;
        
        std::cout << "Calibration successful! Reprojection error: " 
                  << result_.reprojection_error << std::endl;
        
        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "Calibration failed: " << e.what() << std::endl;
        calibration_in_progress_ = false;
        return false;
    }
}

double CalibrationEngine::computeReprojectionError() {
    if (!calibrated_) return -1.0;
    
    double total_error = 0.0;
    size_t total_points = 0;
    
    for (size_t i = 0; i < object_points_.size(); ++i) {
        std::vector<cv::Point2f> projected_points;
        cv::projectPoints(object_points_[i],
                         result_.rvecs[i], result_.tvecs[i],
                         result_.camera_matrix, result_.distortion_coeffs,
                         projected_points);
        
        double error = cv::norm(image_points_[i], projected_points, cv::NORM_L2);
        total_error += error * error;
        total_points += object_points_[i].size();
    }
    
    return std::sqrt(total_error / total_points);
}

void CalibrationEngine::clearCalibrationData() {
    image_points_.clear();
    object_points_.clear();
    result_ = CalibrationResult();
    result_.camera_matrix = cv::Mat::eye(3, 3, CV_64F);
    result_.distortion_coeffs = cv::Mat::zeros(8, 1, CV_64F);
    calibrated_ = false;
    image_size_ = cv::Size();
}

bool CalibrationEngine::findChessboardCorners(const cv::Mat& image, 
                                             cv::Size pattern_size,
                                             std::vector<cv::Point2f>& corners) {
    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image.clone();
    }
    
    return cv::findChessboardCorners(gray, pattern_size, corners,
                                    cv::CALIB_CB_ADAPTIVE_THRESH +
                                    cv::CALIB_CB_NORMALIZE_IMAGE +
                                    cv::CALIB_CB_FAST_CHECK);
}

std::vector<cv::Point3f> CalibrationEngine::generateObjectPoints(cv::Size pattern_size, 
                                                                float square_size) {
    std::vector<cv::Point3f> points;
    points.reserve(pattern_size.width * pattern_size.height);
    
    for (int i = 0; i < pattern_size.height; ++i) {
        for (int j = 0; j < pattern_size.width; ++j) {
            points.emplace_back(j * square_size, i * square_size, 0.0f);
        }
    }
    
    return points;
}

void CalibrationEngine::drawChessboardCorners(cv::Mat& image, 
                                            const std::vector<cv::Point2f>& corners,
                                            cv::Size pattern_size,
                                            bool pattern_found) {
    cv::drawChessboardCorners(image, pattern_size, 
                             cv::Mat(corners), pattern_found);
}

void CalibrationEngine::undistortImage(const cv::Mat& input, cv::Mat& output) {
    if (!calibrated_ || input.empty()) {
        output = input.clone();
        return;
    }
    
    cv::undistort(input, output, result_.camera_matrix, 
                 result_.distortion_coeffs);
}

bool CalibrationEngine::saveCalibration(const std::string& filename) {
    if (!calibrated_) return false;
    
    cv::FileStorage fs(filename, cv::FileStorage::WRITE);
    if (!fs.isOpened()) return false;
    
    fs << "camera_matrix" << result_.camera_matrix;
    fs << "distortion_coefficients" << result_.distortion_coeffs;
    fs << "reprojection_error" << result_.reprojection_error;
    fs << "image_width" << result_.image_size.width;
    fs << "image_height" << result_.image_size.height;
    
    fs.release();
    return true;
}

bool CalibrationEngine::loadCalibration(const std::string& filename) {
    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened()) return false;
    
    fs["camera_matrix"] >> result_.camera_matrix;
    fs["distortion_coefficients"] >> result_.distortion_coeffs;
    fs["reprojection_error"] >> result_.reprojection_error;
    
    int width, height;
    fs["image_width"] >> width;
    fs["image_height"] >> height;
    result_.image_size = cv::Size(width, height);
    
    fs.release();
    
    calibrated_ = true;
    return true;
}