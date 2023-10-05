/**
  atomix by Wade Burch
  (braernoch@gmail.com)

  Open Source
 */

//#include <cstdlib>
//#include <iomanip>
//#include <iostream>

#include <QApplication>
#include <QPushButton>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QScreen>
#include <QCommandLineParser>
#include <QOffscreenSurface>
#include <QtOpenGL/QOpenGLFunctions_3_1>
#include <QtOpenGL/QOpenGLVersionFunctionsFactory>
//#include <eigen3/Eigen/Eigen>

#define SWIDTH 2304
#define SHEIGHT 1296

int main(int argc, char* argv[]) {
    /* Application */
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
    qFmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(qFmt);

    /* Topology: Main Window */
    QWidget window;
    QRect dispXY = QApplication::primaryScreen()->geometry();
    if (!dispXY.isValid()) {dispXY = QApplication::primaryScreen()->virtualGeometry();}
    short ratio = 0.6;
    int dispX = dispXY.width() * ratio ?: SWIDTH;
    int dispY = dispXY.height() * ratio ?: SHEIGHT;
    window.setFixedSize(dispX, dispY);

    /* Topology: Extras */
    QPushButton sButton ("Morb", &window);
    sButton.setGeometry(550, 850, 200, 50);

    /* OpenGL Context and Function Constraint (3.1) */
    auto *qOff = new QOffscreenSurface;
    qOff->setFormat(qFmt);
    qOff->create();

    QOpenGLContext *qglContext = new QOpenGLContext(&window);
    qglContext->setFormat(qFmt);
    qglContext->create();
    qglContext->makeCurrent(qOff);

    auto *qf = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_1>(qglContext);
    qf->initializeOpenGLFunctions();
    
    /* Engage */
    window.show();
    return app.exec();
}