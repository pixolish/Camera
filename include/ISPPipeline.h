#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <array>
#include <vector>

class ISPPipeline {
public:
    struct ISPParameters {
        // Demosaic parameters
        enum class DemosaicMethod {
            BILINEAR = 0,
            VNG,
            AHD
        } demosaic_method = VNG;
        
        // White balance
        float wb_red = 1.0f;
        float wb_green = 1.0f;
        float wb_blue = 1.0f;
        bool auto_wb = true;
        
        // Color correction matrix
        cv::Matx33f color_matrix = cv::Matx33f::eye();
        
        // Gamma correction
        float gamma = 2.2f;
        cv::Mat gamma_lut;
        
        // Tone mapping
        float exposure = 1.0f;
        float contrast = 1.0f;
        float brightness = 0.0f;
        
        // Noise reduction
        bool denoise_enabled = true;
        float denoise_strength = 1.0f;
        
        // Sharpening
        bool sharpen_enabled = true;
        float sharpen_strength = 0.5f;
        
        // Lens correction
        cv::Mat distortion_coeffs;
        cv::Mat camera_matrix;
        bool lens_correction = false;
    };

    ISPPipeline();
    ~ISPPipeline();

    void processRaw(const cv::Mat& raw_bayer, cv::Mat& output_rgb);
    void processRGB(const cv::Mat& input_rgb, cv::Mat& output_rgb);
    
    void setParameters(const ISPParameters& params) { params_ = params; }
    ISPParameters& getParameters() { return params_; }
    
    void calibrateWhiteBalance(const cv::Mat& gray_image);
    void generateGammaLUT();
    void loadColorMatrix(const std::string& filename);
    void saveColorMatrix(const std::string& filename);

private:
    void demosaicBayer(const cv::Mat& bayer, cv::Mat& rgb);
    void applyWhiteBalance(cv::Mat& rgb);
    void applyColorCorrection(cv::Mat& rgb);
    void applyGamma(cv::Mat& rgb);
    void applyToneMapping(cv::Mat& rgb);
    void applyDenoising(cv::Mat& rgb);
    void applySharpening(cv::Mat& rgb);
    void applyLensCorrection(cv::Mat& rgb);
    
    ISPParameters params_;
    cv::Ptr<cv::CLAHE> clahe_;
    
    std::vector<cv::Mat> bayer_patterns_;
};
