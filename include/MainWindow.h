#pragma once

#include <QMainWindow>
#include <QTimer>
#include <memory>

QT_BEGIN_NAMESPACE
class QLabel;
class QComboBox;
class QPushButton;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QSlider;
class QTabWidget;
class QListWidget;
class QGraphicsScene;
class QGraphicsView;
QT_END_NAMESPACE

class ProcessingThread;
class CameraCapture;
class ISPPipeline;
class CalibrationEngine;

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    
protected:
    void closeEvent(QCloseEvent* event) override;
    
private slots:
    void onCameraSelected(int index);
    void onResolutionSelected(int index);
    void onStartStopClicked();
    void onCaptureCalibrationClicked();
    void onCalibrateClicked();
    void onSaveCalibrationClicked();
    void onLoadCalibrationClicked();
    void onISPParameterChanged();
    
    void onFrameProcessed(const QImage& image);
    void onCalibrationFrameAdded(int count);
    void onCalibrationComplete(bool success, double error);
    void onErrorOccurred(const QString& message);
    
    void updateCameraList();
    
private:
    void setupUI();
    void setupConnections();
    void initializeCamera();
    void updateCameraControls();
    void updateISPControls();
    void saveSettings();
    void loadSettings();
    
    // UI Components
    QLabel* display_label_ = nullptr;
    QComboBox* camera_combo_ = nullptr;
    QComboBox* resolution_combo_ = nullptr;
    QComboBox* fps_combo_ = nullptr;
    QPushButton* start_stop_button_ = nullptr;
    QPushButton* calibration_capture_button_ = nullptr;
    QPushButton* calibrate_button_ = nullptr;
    QPushButton* save_calib_button_ = nullptr;
    QPushButton* load_calib_button_ = nullptr;
    
    QTabWidget* tab_widget_ = nullptr;
    QListWidget* calibration_list_ = nullptr;
    
    // ISP Controls
    QDoubleSpinBox* exposure_spin_ = nullptr;
    QDoubleSpinBox* contrast_spin_ = nullptr;
    QDoubleSpinBox* brightness_spin_ = nullptr;
    QSlider* wb_red_slider_ = nullptr;
    QSlider* wb_green_slider_ = nullptr;
    QSlider* wb_blue_slider_ = nullptr;
    QCheckBox* auto_wb_check_ = nullptr;
    QCheckBox* denoise_check_ = nullptr;
    QCheckBox* sharpen_check_ = nullptr;
    QCheckBox* lens_correction_check_ = nullptr;
    
    // Camera and processing
    std::shared_ptr<CameraCapture> camera_;
    std::shared_ptr<ISPPipeline> isp_pipeline_;
    std::shared_ptr<CalibrationEngine> calib_engine_;
    ProcessingThread* processing_thread_ = nullptr;
    
    QTimer* camera_refresh_timer_ = nullptr;
    
    bool is_capturing_ = false;
};