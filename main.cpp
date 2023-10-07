/**
  atomix by Wade Burch
  (braernoch@gmail.com)

  Open Source (GPL)
 */

//#include <cstdlib>
//#include <iomanip>
//#include <iostream>

#include <QApplication>
#include <QSurfaceFormat>
#include <QScreen>
#include <QCommandLineParser>
#include "GWidget.hpp"
#include "MainWindow.hpp"

//#include <eigen3/Eigen/Eigen>

#define SWIDTH 1280
#define SHEIGHT 720

int main(int argc, char* argv[]) {
    /* Application */
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QApplication app (argc, argv);
    QCoreApplication::setApplicationName("atomix");
    QCoreApplication::setOrganizationName("Nolnoch, LLC");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    /* Exe and CLI Parsing */
    QCommandLineParser qParser;
    qParser.setApplicationDescription(QCoreApplication::applicationName());
    qParser.addHelpOption();
    qParser.addVersionOption();
    qParser.process(app);
    
    /* Surface Format */
    QSurfaceFormat qFmt;
    qFmt.setVersion(3,1);
    qFmt.setDepthBufferSize(24);
    qFmt.setStencilBufferSize(8);
    qFmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(qFmt);

    /* Windows */
    MainWindow mainWindow;
    QRect dispXY = QApplication::primaryScreen()->geometry();
    if (!dispXY.isValid()) {dispXY = QApplication::primaryScreen()->virtualGeometry();}
    float ratio = 0.6;
    int dispX = dispXY.width() * ratio ?: SWIDTH;
    int dispY = dispXY.height() * ratio ?: SHEIGHT;
    mainWindow.resize(dispX, dispY);

    /* Engage */
    mainWindow.show();
    
    return app.exec();
}