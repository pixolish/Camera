#include "MainWindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    // Set application information
    app.setApplicationName("Camera Calibration ISP");
    app.setOrganizationName("CameraCalibrationISP");
    app.setApplicationVersion("1.0.0");
    
    // Set fusion style for consistent look across platforms
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Create and show main window
    MainWindow main_window;
    main_window.show();
    
    return app.exec();
}