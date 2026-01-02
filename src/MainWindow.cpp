#include "MainWindow.h"
#include "CameraCapture.h"
#include "ISPPipeline.h"
#include "CalibrationEngine.h"
#include "ProcessingThread.h"

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QTabWidget>
#include <QListWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QTimer>
#include <QGraphicsScene>
#include <QGraphicsView>

MainWindow::MainWindow(QWidget* parent) 
    : QMainWindow(parent) {
    
    // Initialize components
    camera_ = std::make_shared<CameraCapture>();
    isp_pipeline_ = std::make_shared<ISPPipeline>();
    calib_engine_ = std::make_shared<CalibrationEngine>();
    processing_thread_ = new ProcessingThread(this);
    
    setupUI();
    setupConnections();
    loadSettings();
    
    // Setup timer for camera list refresh
    camera_refresh_timer_ = new QTimer(this);
    connect(camera_refresh_timer_, &QTimer::timeout, 
            this, &MainWindow::updateCameraList);
    camera_refresh_timer_->start(5000); // Refresh every 5 seconds
    
    updateCameraList();
}

MainWindow::~MainWindow() {
    processing_thread_->stopCapture();
    saveSettings();
}

void MainWindow::setupUI() {
    QWidget* central_widget = new QWidget(this);
    setCentralWidget(central_widget);
    
    QVBoxLayout* main_layout = new QVBoxLayout(central_widget);
    
    // Camera controls
    QGroupBox* camera_group = new QGroupBox("Camera Control", central_widget);
    QHBoxLayout* camera_layout = new QHBoxLayout(camera_group);
    
    camera_combo_ = new QComboBox(camera_group);
    resolution_combo_ = new QComboBox(camera_group);
    fps_combo_ = new QComboBox(camera_group);
    
    start_stop_button_ = new QPushButton("Start", camera_group);
    calibration_capture_button_ = new QPushButton("Capture Calibration", camera_group);
    calibrate_button_ = new QPushButton("Calibrate", camera_group);
    save_calib_button_ = new QPushButton("Save Calibration", camera_group);
    load_calib_button_ = new QPushButton("Load Calibration", camera_group);
    
    camera_layout->addWidget(new QLabel("Camera:"));
    camera_layout->addWidget(camera_combo_);
    camera_layout->addWidget(new QLabel("Resolution:"));
    camera_layout->addWidget(resolution_combo_);
    camera_layout->addWidget(new QLabel("FPS:"));
    camera_layout->addWidget(fps_combo_);
    camera_layout->addWidget(start_stop_button_);
    camera_layout->addWidget(calibration_capture_button_);
    camera_layout->addWidget(calibrate_button_);
    camera_layout->addWidget(save_calib_button_);
    camera_layout->addWidget(load_calib_button_);
    
    main_layout->addWidget(camera_group);
    
    // Display area
    display_label_ = new QLabel(central_widget);
    display_label_->setAlignment(Qt::AlignCenter);
    display_label_->setMinimumSize(640, 480);
    display_label_->setStyleSheet("border: 1px solid #ccc; background: #333;");
    main_layout->addWidget(display_label_, 1);
    
    // Tabs for different controls
    tab_widget_ = new QTabWidget(central_widget);
    
    // ISP Tab
    QWidget* isp_tab = new QWidget(tab_widget_);
    QFormLayout* isp_layout = new QFormLayout(isp_tab);
    
    exposure_spin_ = new QDoubleSpinBox(isp_tab);
    exposure_spin_->setRange(0.1, 10.0);
    exposure_spin_->setSingleStep(0.1);
    exposure_spin_->setValue(1.0);
    
    contrast_spin_ = new QDoubleSpinBox(isp_tab);
    contrast_spin_->setRange(0.1, 5.0);
    contrast_spin_->setSingleStep(0.1);
    contrast_spin_->setValue(1.0);
    
    brightness_spin_ = new QDoubleSpinBox(isp_tab);
    brightness_spin_->setRange(-1.0, 1.0);
    brightness_spin_->setSingleStep(0.1);
    brightness_spin_->setValue(0.0);
    
    wb_red_slider_ = new QSlider(Qt::Horizontal, isp_tab);
    wb_red_slider_->setRange(0, 200);
    wb_red_slider_->setValue(100);
    
    wb_green_slider_ = new QSlider(Qt::Horizontal, isp_tab);
    wb_green_slider_->setRange(0, 200);
    wb_green_slider_->setValue(100);
    
    wb_blue_slider_ = new QSlider(Qt::Horizontal, isp_tab);
    wb_blue_slider_->setRange(0, 200);
    wb_blue_slider_->setValue(100);
    
    auto_wb_check_ = new QCheckBox("Auto White Balance", isp_tab);
    auto_wb_check_->setChecked(true);
    
    denoise_check_ = new QCheckBox("Enable Denoising", isp_tab);
    denoise_check_->setChecked(true);
    
    sharpen_check_ = new QCheckBox("Enable Sharpening", isp_tab);
    sharpen_check_->setChecked(true);
    
    lens_correction_check_ = new QCheckBox("Lens Correction", isp_tab);
    lens_correction_check_->setChecked(false);
    
    isp_layout->addRow("Exposure:", exposure_spin_);
    isp_layout->addRow("Contrast:", contrast_spin_);
    isp_layout->addRow("Brightness:", brightness_spin_);
    isp_layout->addRow("WB Red:", wb_red_slider_);
    isp_layout->addRow("WB Green:", wb_green_slider_);
    isp_layout->addRow("WB Blue:", wb_blue_slider_);
    isp_layout->addRow("", auto_wb_check_);
    isp_layout->addRow("", denoise_check_);
    isp_layout->addRow("", sharpen_check_);
    isp_layout->addRow("", lens_correction_check_);
    
    // Calibration Tab
    QWidget* calib_tab = new QWidget(tab_widget_);
    QVBoxLayout* calib_layout = new QVBoxLayout(calib_tab);
    
    calibration_list_ = new QListWidget(calib_tab);
    calib_layout->addWidget(new QLabel("Calibration Frames:"));
    calib_layout->addWidget(calibration_list_);
    
    tab_widget_->addTab(isp_tab, "ISP Settings");
    tab_widget_->addTab(calib_tab, "Calibration");
    
    main_layout->addWidget(tab_widget_);
    
    setWindowTitle("Camera Calibration & ISP Pipeline");
    resize(1024, 768);
}

void MainWindow::setupConnections() {
    // Camera connections
    connect(camera_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onCameraSelected);
    connect(start_stop_button_, &QPushButton::clicked,
            this, &MainWindow::onStartStopClicked);
    connect(calibration_capture_button_, &QPushButton::clicked,
            this, &MainWindow::onCaptureCalibrationClicked);
    connect(calibrate_button_, &QPushButton::clicked,
            this, &MainWindow::onCalibrateClicked);
    connect(save_calib_button_, &QPushButton::clicked,
            this, &MainWindow::onSaveCalibrationClicked);
    connect(load_calib_button_, &QPushButton::clicked,
            this, &MainWindow::onLoadCalibrationClicked);
    
    // ISP connections
    connect(exposure_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onISPParameterChanged);
    connect(contrast_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onISPParameterChanged);
    connect(brightness_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onISPParameterChanged);
    connect(wb_red_slider_, &QSlider::valueChanged,
            this, &MainWindow::onISPParameterChanged);
    connect(wb_green_slider_, &QSlider::valueChanged,
            this, &MainWindow::onISPParameterChanged);
    connect(wb_blue_slider_, &QSlider::valueChanged,
            this, &MainWindow::onISPParameterChanged);
    connect(auto_wb_check_, &QCheckBox::stateChanged,
            this, &MainWindow::onISPParameterChanged);
    connect(denoise_check_, &QCheckBox::stateChanged,
            this, &MainWindow::onISPParameterChanged);
    connect(sharpen_check_, &QCheckBox::stateChanged,
            this, &MainWindow::onISPParameterChanged);
    connect(lens_correction_check_, &QCheckBox::stateChanged,
            this, &MainWindow::onISPParameterChanged);
    
    // Processing thread connections
    processing_thread_->setCamera(camera_);
    processing_thread_->setISPPipeline(isp_pipeline_);
    processing_thread_->setCalibrationEngine(calib_engine_);
    
    connect(processing_thread_, &ProcessingThread::frameProcessed,
            this, &MainWindow::onFrameProcessed);
    connect(processing_thread_, &ProcessingThread::calibrationFrameAdded,
            this, &MainWindow::onCalibrationFrameAdded);
    connect(processing_thread_, &ProcessingThread::calibrationComplete,
            this, &MainWindow::onCalibrationComplete);
    connect(processing_thread_, &ProcessingThread::errorOccurred,
            this, &MainWindow::onErrorOccurred);
}

void MainWindow::updateCameraList() {
    auto cameras = camera_->listAvailableCameras();
    
    int current_index = camera_combo_->currentIndex();
    QString current_text = camera_combo_->currentText();
    
    camera_combo_->clear();
    
    for (const auto& cam : cameras) {
        camera_combo_->addItem(QString::fromStdString(cam.name), cam.id);
    }
    
    if (camera_combo_->count() > 0) {
        if (current_index >= 0 && current_index < camera_combo_->count() &&
            camera_combo_->itemText(current_index) == current_text) {
            camera_combo_->setCurrentIndex(current_index);
        } else {
            camera_combo_->setCurrentIndex(0);
            onCameraSelected(0);
        }
    }
}

void MainWindow::onCameraSelected(int index) {
    if (index >= 0 && camera_) {
        int camera_id = camera_combo_->itemData(index).toInt();
        
        // Stop current capture
        if (is_capturing_) {
            processing_thread_->stopCapture();
            start_stop_button_->setText("Start");
            is_capturing_ = false;
        }
        
        // Update resolution combo
        resolution_combo_->clear();
        auto cameras = camera_->listAvailableCameras();
        for (const auto& cam : cameras) {
            if (cam.id == camera_id) {
                for (const auto& res : cam.resolutions) {
                    QString res_str = QString("%1x%2").arg(res.first).arg(res.second);
                    resolution_combo_->addItem(res_str, QVariant::fromValue(res));
                }
                break;
            }
        }
        
        if (resolution_combo_->count() > 0) {
            resolution_combo_->setCurrentIndex(0);
        }
        
        // Update FPS combo
        fps_combo_->clear();
        std::vector<int> fps_options = {15, 30, 60};
        for (int fps : fps_options) {
            fps_combo_->addItem(QString::number(fps), fps);
        }
        fps_combo_->setCurrentIndex(1); // 30 FPS
        
        initializeCamera();
    }
}

void MainWindow::onStartStopClicked() {
    if (!is_capturing_) {
        if (camera_->initialize(camera_combo_->currentData().toInt())) {
            processing_thread_->startCapture();
            start_stop_button_->setText("Stop");
            is_capturing_ = true;
        } else {
            QMessageBox::warning(this, "Error", "Failed to initialize camera");
        }
    } else {
        processing_thread_->stopCapture();
        start_stop_button_->setText("Start");
        is_capturing_ = false;
    }
}

void MainWindow::onCaptureCalibrationClicked() {
    if (calib_engine_->getNumCalibrationImages() >= 20) {
        QMessageBox::information(this, "Info", 
                               "Maximum calibration frames (20) reached");
        return;
    }
    
    processing_thread_->captureCalibrationFrame();
}

void MainWindow::onCalibrateClicked() {
    if (calib_engine_->getNumCalibrationImages() < 5) {
        QMessageBox::warning(this, "Warning", 
                           "Need at least 5 calibration images");
        return;
    }
    
    // Show calibration dialog or progress
    bool success = calib_engine_->calibrate();
    
    if (success) {
        double error = calib_engine_->computeReprojectionError();
        QMessageBox::information(this, "Calibration Complete",
                               QString("Calibration successful!\n"
                                      "Reprojection error: %1")
                               .arg(error, 0, 'f', 3));
        
        // Update ISP pipeline with calibration data
        auto result = calib_engine_->getResult();
        auto isp_params = isp_pipeline_->getParameters();
        isp_params.camera_matrix = result.camera_matrix;
        isp_params.distortion_coeffs = result.distortion_coeffs;
        isp_pipeline_->setParameters(isp_params);
        
        updateISPControls();
    } else {
        QMessageBox::warning(this, "Calibration Failed", 
                           "Camera calibration failed");
    }
}

void MainWindow::onSaveCalibrationClicked() {
    QString filename = QFileDialog::getSaveFileName(this,
        "Save Calibration", "", "Calibration Files (*.yml *.yaml *.xml)");
    
    if (!filename.isEmpty()) {
        if (calib_engine_->saveCalibration(filename.toStdString())) {
            QMessageBox::information(this, "Success", 
                                   "Calibration saved successfully");
        } else {
            QMessageBox::warning(this, "Error", 
                               "Failed to save calibration");
        }
    }
}

void MainWindow::onLoadCalibrationClicked() {
    QString filename = QFileDialog::getOpenFileName(this,
        "Load Calibration", "", "Calibration Files (*.yml *.yaml *.xml)");
    
    if (!filename.isEmpty()) {
        if (calib_engine_->loadCalibration(filename.toStdString())) {
            QMessageBox::information(this, "Success", 
                                   "Calibration loaded successfully");
            
            // Update ISP pipeline with loaded calibration
            auto result = calib_engine_->getResult();
            auto isp_params = isp_pipeline_->getParameters();
            isp_params.camera_matrix = result.camera_matrix;
            isp_params.distortion_coeffs = result.distortion_coeffs;
            isp_pipeline_->setParameters(isp_params);
            
            updateISPControls();
        } else {
            QMessageBox::warning(this, "Error", 
                               "Failed to load calibration");
        }
    }
}

void MainWindow::onISPParameterChanged() {
    auto params = isp_pipeline_->getParameters();
    
    params.exposure = exposure_spin_->value();
    params.contrast = contrast_spin_->value();
    params.brightness = brightness_spin_->value();
    
    params.wb_red = wb_red_slider_->value() / 100.0f;
    params.wb_green = wb_green_slider_->value() / 100.0f;
    params.wb_blue = wb_blue_slider_->value() / 100.0f;
    params.auto_wb = auto_wb_check_->isChecked();
    
    params.denoise_enabled = denoise_check_->isChecked();
    params.sharpen_enabled = sharpen_check_->isChecked();
    params.lens_correction = lens_correction_check_->isChecked();
    
    isp_pipeline_->setParameters(params);
}

void MainWindow::onFrameProcessed(const QImage& image) {
    display_label_->setPixmap(QPixmap::fromImage(image).scaled(
        display_label_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::onCalibrationFrameAdded(int count) {
    calibration_list_->addItem(QString("Calibration Frame %1").arg(count));
}

void MainWindow::onCalibrationComplete(bool success, double error) {
    if (success) {
        QMessageBox::information(this, "Success", 
                               QString("Calibration complete\nError: %1").arg(error));
    } else {
        QMessageBox::warning(this, "Error", "Calibration failed");
    }
}

void MainWindow::onErrorOccurred(const QString& message) {
    QMessageBox::warning(this, "Error", message);
}

void MainWindow::initializeCamera() {
    if (camera_combo_->count() > 0) {
        // Camera will be initialized when start is clicked
    }
}

void MainWindow::updateCameraControls() {
    // Update UI based on camera state
    bool camera_available = camera_combo_->count() > 0;
    
    start_stop_button_->setEnabled(camera_available);
    resolution_combo_->setEnabled(camera_available);
    fps_combo_->setEnabled(camera_available);
}

void MainWindow::updateISPControls() {
    auto params = isp_pipeline_->getParameters();
    
    exposure_spin_->setValue(params.exposure);
    contrast_spin_->setValue(params.contrast);
    brightness_spin_->setValue(params.brightness);
    
    wb_red_slider_->setValue(params.wb_red * 100);
    wb_green_slider_->setValue(params.wb_green * 100);
    wb_blue_slider_->setValue(params.wb_blue * 100);
    
    auto_wb_check_->setChecked(params.auto_wb);
    denoise_check_->setChecked(params.denoise_enabled);
    sharpen_check_->setChecked(params.sharpen_enabled);
    lens_correction_check_->setChecked(params.lens_correction);
}

void MainWindow::saveSettings() {
    QSettings settings("CameraCalibrationISP", "Settings");
    
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/state", saveState());
    
    // Save ISP parameters
    auto params = isp_pipeline_->getParameters();
    settings.setValue("isp/exposure", params.exposure);
    settings.setValue("isp/contrast", params.contrast);
    settings.setValue("isp/brightness", params.brightness);
    settings.setValue("isp/wb_red", params.wb_red);
    settings.setValue("isp/wb_green", params.wb_green);
    settings.setValue("isp/wb_blue", params.wb_blue);
    settings.setValue("isp/auto_wb", params.auto_wb);
    settings.setValue("isp/denoise", params.denoise_enabled);
    settings.setValue("isp/sharpen", params.sharpen_enabled);
    
    // Save camera selection
    if (camera_combo_->count() > 0) {
        settings.setValue("camera/index", camera_combo_->currentIndex());
    }
}

void MainWindow::loadSettings() {
    QSettings settings("CameraCalibrationISP", "Settings");
    
    restoreGeometry(settings.value("window/geometry").toByteArray());
    restoreState(settings.value("window/state").toByteArray());
    
    // Load ISP parameters
    auto params = isp_pipeline_->getParameters();
    
    params.exposure = settings.value("isp/exposure", 1.0).toDouble();
    params.contrast = settings.value("isp/contrast", 1.0).toDouble();
    params.brightness = settings.value("isp/brightness", 0.0).toDouble();
    params.wb_red = settings.value("isp/wb_red", 1.0).toFloat();
    params.wb_green = settings.value("isp/wb_green", 1.0).toFloat();
    params.wb_blue = settings.value("isp/wb_blue", 1.0).toFloat();
    params.auto_wb = settings.value("isp/auto_wb", true).toBool();
    params.denoise_enabled = settings.value("isp/denoise", true).toBool();
    params.sharpen_enabled = settings.value("isp/sharpen", true).toBool();
    
    isp_pipeline_->setParameters(params);
    updateISPControls();
    
    // Load camera selection
    int camera_index = settings.value("camera/index", 0).toInt();
    if (camera_index >= 0 && camera_index < camera_combo_->count()) {
        camera_combo_->setCurrentIndex(camera_index);
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    processing_thread_->stopCapture();
    saveSettings();
    event->accept();
}