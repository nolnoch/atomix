/**
 * main.cpp
 *
 *    Created on: Oct 3, 2023
 *   Last Update: Sep 9, 2024
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *  Copyright 2023, 2024 Wade Burch (GPLv3)
 * 
 *  This file is part of atomix.
 * 
 *  atomix is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software 
 *  Foundation, either version 3 of the License, or (at your option) any later 
 *  version.
 * 
 *  atomix is distributed in the hope that it will be useful, but WITHOUT ANY 
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License along with 
 *  atomix. If not, see <https://www.gnu.org/licenses/>.
 */


#include <QApplication>
#include <QSurfaceFormat>
#include <QScreen>
#include <QCommandLineParser>
#include <QFont>
#include "mainwindow.hpp"

#define SWIDTH 1280
#define SHEIGHT 720
#define SRATIO 0.75


int main(int argc, char* argv[]) {
    /* Application */
    QApplication app (argc, argv);
    QCoreApplication::setApplicationName("atomix");
    QCoreApplication::setOrganizationName("Nolnoch");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    /* Exe and CLI Parsing */
    QCommandLineParser qParser;
    qParser.setApplicationDescription(QCoreApplication::applicationName());
    qParser.addHelpOption();
    qParser.addVersionOption();
    qParser.process(app);
    
    /* Surface Format */
    QSurfaceFormat qFmt;
    qFmt.setDepthBufferSize(24);
    qFmt.setStencilBufferSize(8);
    qFmt.setSamples(4);
    qFmt.setVersion(4,6);
    qFmt.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(qFmt);

    /* Windows */
    MainWindow mainWindow;
    QRect dispXY = QApplication::primaryScreen()->geometry();
    if (!dispXY.isValid()) {dispXY = QApplication::primaryScreen()->virtualGeometry();}
    double ratio = SRATIO;
    int dispX = dispXY.width() * ratio ?: SWIDTH;
    int dispY = dispXY.height() * ratio ?: SHEIGHT;
    mainWindow.resize(dispX, dispY);
    mainWindow.move(dispXY.center() - mainWindow.frameGeometry().center());
    QIcon icoAtomix("/home/braer/dev/atomix/res/icons/favicon.ico");
    app.setWindowIcon(icoAtomix);

    /* Engage */
    app.processEvents();
    mainWindow.show();
    mainWindow.setupLoading();
    
    return app.exec();
}