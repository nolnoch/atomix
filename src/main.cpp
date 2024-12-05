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


AtomixFiles atomixFiles;
bool isDebug;
bool isMacOS;
bool isProfiling;


int main(int argc, char* argv[]) {
    // Platform
    QString arch = QSysInfo::currentCpuArchitecture();
    QString os = QSysInfo::prettyProductName();
    isMacOS = os.contains("macOS");

    // Application
    QApplication::setStyle("Fusion");
    QApplication app (argc, argv);
    // app.setStyle("Fusion");
    QCoreApplication::setApplicationName("atomix");
    QCoreApplication::setOrganizationName("Nolnoch");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    // Exe and CLI Parsing
    QCommandLineParser qParser;
    qParser.setApplicationDescription(QCoreApplication::applicationName());
    QCommandLineOption cliVerbose("verbose", QCoreApplication::translate("main", "ALL debug and information messages"));
    QCommandLineOption cliAtomixDir({ "d", "atomix-dir" }, QCoreApplication::translate("main", "parent directory of atomix shaders, configs, etc. (default: application directory)"), "directory", QCoreApplication::applicationDirPath());
    QCommandLineOption cliProfiling({ "p", "profiling" }, QCoreApplication::translate("main", "enable profiling"));
    qParser.addHelpOption();
    qParser.addVersionOption();
    qParser.addOption(cliVerbose);
    qParser.addOption(cliAtomixDir);
    qParser.addOption(cliProfiling);
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
    QString strAtomixDir = qParser.value(cliAtomixDir);

    // Debug Info
    if (isDebug) {
        std::cout << "OS: " << os.toStdString() << " (" << arch.toStdString() << ")" << std::endl;
        std::cout << "Qt Version: " << QT_VERSION_STR << std::endl;
    }

    // Set atomix directory and icon
    QDir execDir = QDir(strAtomixDir);
    QDir atomixDir = QDir(execDir.relativeFilePath("../../../"));       // TODO : Set to execDir for release
    if (!atomixFiles.setRoot(atomixDir.absolutePath().toStdString())) {
        QString dir = QFileDialog::getExistingDirectory(
            nullptr,
            "Select Atomix Directory",
            execDir.absolutePath(),
            QFileDialog::ShowDirsOnly
        );
        if (dir.isEmpty()) {return 0;}
        execDir = QDir(dir);
        atomixFiles.setRoot(dir.toStdString());
    }
    QIcon icoAtomix(QString::fromStdString(atomixFiles.resources()) + QString::fromStdString("icons/favicon.ico"));
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
    MainWindow mainWindow;
    QRect dispXY = QApplication::primaryScreen()->geometry();
    if (!dispXY.isValid()) {dispXY = QApplication::primaryScreen()->virtualGeometry();}
    dispXY = QRect(0, 0, dispXY.width() + 1, dispXY.height() + 1);
    
    mainWindow.setWindowTitle(QCoreApplication::applicationName());
    mainWindow.init(dispXY);

    // I would like the ship to go.
    // Now.
    mainWindow.show();
    app.processEvents();
    mainWindow.postInit();
    
    return app.exec();
}
