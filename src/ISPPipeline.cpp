#include "ISPPipeline.h"
#include <cmath>
#include <fstream>
#include <opencv2/opencv.hpp>

ISPPipeline::ISPPipeline() {
    generateGammaLUT();
    clahe_ = cv::createCLAHE();
    clahe_->setClipLimit(2.0);
}

ISPPipeline::~ISPPipeline() {}

void ISPPipeline::processRaw(const cv::Mat& raw_bayer, cv::Mat& output_rgb) {
    if (raw_bayer.empty()) return;
    
    cv::Mat rgb;
    demosaicBayer(raw_bayer, rgb);
    processRGB(rgb, output_rgb);
}

void ISPPipeline::processRGB(const cv::Mat& input_rgb, cv::Mat& output_rgb) {
    if (input_rgb.empty()) {
        output_rgb = cv::Mat();
        return;
    }
    
    cv::Mat processed = input_rgb.clone();
    
    if (params_.lens_correction && !params_.camera_matrix.empty() && 
        !params_.distortion_coeffs.empty()) {
        applyLensCorrection(processed);
    }
    
    applyWhiteBalance(processed);
    applyColorCorrection(processed);
    applyGamma(processed);
    applyToneMapping(processed);
    
    if (params_.denoise_enabled) {
        applyDenoising(processed);
    }
    
    if (params_.sharpen_enabled) {
        applySharpening(processed);
    }
    
    output_rgb = processed;
}

void ISPPipeline::demosaicBayer(const cv::Mat& bayer, cv::Mat& rgb) {
    if (bayer.type() == CV_8UC1) {
        switch (params_.demosaic_method) {
            case BILINEAR:
                cv::cvtColor(bayer, rgb, cv::COLOR_BayerBG2BGR);
                break;
            case VNG:
                cv::cvtColor(bayer, rgb, cv::COLOR_BayerBG2BGR_VNG);
                break;
            case AHD:
                cv::cvtColor(bayer, rgb, cv::COLOR_BayerBG2BGR_EA);
                break;
        }
    } else {
        bayer.convertTo(rgb, CV_8UC3);
    }
}

void ISPPipeline::applyWhiteBalance(cv::Mat& rgb) {
    if (params_.auto_wb) {
        // Simple gray world assumption
        cv::Scalar mean = cv::mean(rgb);
        float avg = (mean[0] + mean[1] + mean[2]) / 3.0f;
        
        if (mean[0] > 0) params_.wb_blue = avg / mean[0];
        if (mean[1] > 0) params_.wb_green = avg / mean[1];
        if (mean[2] > 0) params_.wb_red = avg / mean[2];
    }
    
    std::vector<cv::Mat> channels;
    cv::split(rgb, channels);
    
    channels[0] *= params_.wb_blue;
    channels[1] *= params_.wb_green;
    channels[2] *= params_.wb_red;
    
    cv::merge(channels, rgb);
    cv::normalize(rgb, rgb, 0, 255, cv::NORM_MINMAX);
    rgb.convertTo(rgb, CV_8UC3);
}

void ISPPipeline::applyColorCorrection(cv::Mat& rgb) {
    if (params_.color_matrix.val[0] != 1.0f || 
        params_.color_matrix.val[4] != 1.0f || 
        params_.color_matrix.val[8] != 1.0f) {
        
        cv::Mat float_rgb;
        rgb.convertTo(float_rgb, CV_32FC3);
        
        std::vector<cv::Mat> channels;
        cv::split(float_rgb, channels);
        
        cv::Mat corrected = cv::Mat::zeros(float_rgb.size(), CV_32FC3);
        
        for (int i = 0; i < float_rgb.rows; ++i) {
            for (int j = 0; j < float_rgb.cols; ++j) {
                cv::Vec3f pixel(
                    channels[0].at<float>(i, j),
                    channels[1].at<float>(i, j),
                    channels[2].at<float>(i, j)
                );
                
                cv::Vec3f corrected_pixel = params_.color_matrix * pixel;
                
                corrected.at<cv::Vec3f>(i, j) = corrected_pixel;
            }
        }
        
        corrected.convertTo(rgb, CV_8UC3);
    }
}

void ISPPipeline::generateGammaLUT() {
    params_.gamma_lut = cv::Mat(1, 256, CV_8UC1);
    uchar* lut = params_.gamma_lut.ptr();
    
    float gamma_inv = 1.0f / params_.gamma;
    
    for (int i = 0; i < 256; ++i) {
        lut[i] = cv::saturate_cast<uchar>(
            pow(i / 255.0f, gamma_inv) * 255.0f
        );
    }
}

void ISPPipeline::applyGamma(cv::Mat& rgb) {
    if (!params_.gamma_lut.empty()) {
        std::vector<cv::Mat> channels;
        cv::split(rgb, channels);
        
        for (auto& channel : channels) {
            cv::LUT(channel, params_.gamma_lut, channel);
        }
        
        cv::merge(channels, rgb);
    }
}

void ISPPipeline::applyToneMapping(cv::Mat& rgb) {
    cv::Mat float_rgb;
    rgb.convertTo(float_rgb, CV_32FC3, 1.0/255.0);
    
    // Apply exposure
    float_rgb *= params_.exposure;
    
    // Apply contrast and brightness
    float_rgb = float_rgb * params_.contrast + params_.brightness;
    
    // Clamp values
    float_rgb = cv::max(float_rgb, 0.0f);
    float_rgb = cv::min(float_rgb, 1.0f);
    
    // Convert back
    float_rgb.convertTo(rgb, CV_8UC3, 255.0);
}

void ISPPipeline::applyDenoising(cv::Mat& rgb) {
    cv::Mat denoised;
    cv::fastNlMeansDenoisingColored(rgb, denoised, 
                                   params_.denoise_strength,
                                   params_.denoise_strength * 0.5f,
                                   7, 21);
    rgb = denoised;
}

void ISPPipeline::applySharpening(cv::Mat& rgb) {
    cv::Mat blurred;
    cv::GaussianBlur(rgb, blurred, cv::Size(0, 0), 3.0);
    
    cv::addWeighted(rgb, 1.0 + params_.sharpen_strength,
                   blurred, -params_.sharpen_strength,
                   0, rgb);
}

void ISPPipeline::applyLensCorrection(cv::Mat& rgb) {
    cv::Mat undistorted;
    cv::undistort(rgb, undistorted, 
                  params_.camera_matrix,
                  params_.distortion_coeffs);
    rgb = undistorted;
}

void ISPPipeline::calibrateWhiteBalance(const cv::Mat& gray_image) {
    if (!gray_image.empty()) {
        cv::Scalar mean = cv::mean(gray_image);
        float avg_gray = mean[0];
        
        // Simple white balance calibration
        params_.wb_red = avg_gray / 128.0f;
        params_.wb_green = avg_gray / 128.0f;
        params_.wb_blue = avg_gray / 128.0f;
    }
}

void ISPPipeline::loadColorMatrix(const std::string& filename) {
    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (fs.isOpened()) {
        fs["ColorMatrix"] >> params_.color_matrix;
        fs.release();
    }
}

void ISPPipeline::saveColorMatrix(const std::string& filename) {
    cv::FileStorage fs(filename, cv::FileStorage::WRITE);
    if (fs.isOpened()) {
        fs << "ColorMatrix" << params_.color_matrix;
        fs.release();
    }
}
