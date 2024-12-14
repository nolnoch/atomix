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

#include <QSysInfo>
#include <QtWidgets/QApplication>
#include <QCommandLineParser>
#include <QSurfaceFormat>
#include "mainwindow.hpp"


bool isDebug;
bool isMacOS;
bool isProfiling;
bool isTesting;


int main(int argc, char* argv[]) {
    // Application
    QApplication app (argc, argv);
    app.setStyle("Fusion");
    QApplication::setApplicationName("atomix");
    QApplication::setOrganizationName("Nolnoch");
    QApplication::setApplicationVersion(QT_VERSION_STR);
    MainWindow mainWindow;

    // Exe and CLI Parsing
    QCommandLineParser qParser;
    qParser.setApplicationDescription(QApplication::applicationName());
    QCommandLineOption cliVerbose("verbose", QApplication::translate("main", "ALL debug and information messages"));
    QCommandLineOption cliAtomixDir({ "d", "atomix-dir" }, QApplication::translate("main", "parent directory of atomix shaders, configs, etc. (default: application directory)"), "directory", QApplication::applicationDirPath());
    QCommandLineOption cliProfiling({ "p", "profiling" }, QApplication::translate("main", "enable profiling"));
    QCommandLineOption cliTesting({ "t", "testing" }, QApplication::translate("main", "enable testing"));
    QCommandLineOption cliResetGeometry({ "r", "reset-geometry" }, QApplication::translate("main", "reset window geometry (instead of loading saved geometry)"));
    qParser.addHelpOption();
    qParser.addVersionOption();
    qParser.addOption(cliVerbose);
    qParser.addOption(cliAtomixDir);
    qParser.addOption(cliProfiling);
    qParser.addOption(cliTesting);
    qParser.addOption(cliResetGeometry);
    qParser.process(app);

    // CLI option results
    if (qParser.isSet(cliVerbose)) {
        std::cout << "Verbosity Level: 9001 !!!1one" << std::endl;
        isDebug = true;
    }
    if (qParser.isSet(cliProfiling)) {
        std::cout << "Profiling Enabled" << std::endl;
        isProfiling = true;
    }
    if (qParser.isSet(cliTesting)) {
        std::cout << "Testing Enabled" << std::endl;
        isTesting = true;
    }
    if (qParser.isSet(cliResetGeometry)) {
        std::cout << "Reset Geometry Enabled" << std::endl;
        mainWindow.resetGeometry();
    }
    QString strAtomixDir = qParser.value(cliAtomixDir);

    // Platform
    QString arch = QSysInfo::currentCpuArchitecture();
    QString os = QSysInfo::prettyProductName();
    isMacOS = os.contains("macOS");

    // Debug Info
    if (isDebug) {
        std::cout << "OS: " << os.toStdString() << " (" << arch.toStdString() << ")" << std::endl;
        std::cout << "Qt Version: " << QT_VERSION_STR << std::endl;
    }

    // Set atomix directory and icon
    QDir atomixDir = QDir(QDir(strAtomixDir).relativeFilePath("../"));
    while (!mainWindow.getAtomixFiles().setRoot(atomixDir.absolutePath().toStdString())) {
        QString dir = QFileDialog::getExistingDirectory(
            nullptr,
            "Select atomix Files Directory",
            atomixDir.absolutePath(),
            QFileDialog::ShowDirsOnly
        );
        if (dir.isEmpty()) {
            std::cout << "Canceled." << std::endl;
            return 0;
        }
        atomixDir = QDir(dir);
    }
    QIcon icoAtomix(QString::fromStdString(mainWindow.getAtomixFiles().resources()) + QString::fromStdString("icons/favicon.ico"));
    app.setWindowIcon(icoAtomix);
    
    // Surface Format
    QSurfaceFormat qFmt;
    qFmt.setDepthBufferSize(24);
    qFmt.setStencilBufferSize(8);
    qFmt.setSamples(4);
    qFmt.setVersion(4,6);
    qFmt.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(qFmt);

    // Windows
    QRect dispXY = QApplication::primaryScreen()->geometry();
    if (!dispXY.isValid()) {dispXY = QApplication::primaryScreen()->virtualGeometry();}
    dispXY = QRect(0, 0, dispXY.width() + 1, dispXY.height() + 1);
    mainWindow.setWindowTitle(QApplication::applicationName());
    mainWindow.init(dispXY);

    // I would like the ship to go.
    mainWindow.show();
    app.processEvents();
    // Now.
    mainWindow.postInit();
    
    return app.exec();
}
