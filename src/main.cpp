/**
 * main.cpp
 *
 *    Created on: Oct 3, 2023
 *   Last Update: Dec 29, 2024
 *  Orig. Author: Wade Burch (dev@nolnoch.com)
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
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QLoggingCategory>
#include <QSurfaceFormat>
#include "mainwindow.hpp"


bool isDebug;
bool isMacOS;
bool isProfiling;
bool isTesting;


int main(int argc, char* argv[]) {
    // Application
    QApplication app(argc, argv);
    app.setApplicationName("atomix");
    app.setOrganizationName("nolnoch");
    app.setApplicationVersion(ATOMIX_VERSION);

#if defined(Q_OS_MACOS)
    // Qt looks for the Vulkan loader with a bare dlopen("libvulkan.dylib"), which doesn't search the
    // app bundle. Point it at the loader we ship in Contents/Frameworks (unless the user overrides it).
    if (!qEnvironmentVariableIsSet("QT_VULKAN_LIB")) {
        const QString bundledLoader = QCoreApplication::applicationDirPath() + "/../Frameworks/libvulkan.1.dylib";
        if (QFileInfo::exists(bundledLoader)) {
            qputenv("QT_VULKAN_LIB", QFileInfo(bundledLoader).canonicalFilePath().toUtf8());
        }
    }
#endif

    app.setStyle("fusion");
    MainWindow mainWindow;
    QSettings settings;

    // Exe and CLI Parsing
    QCommandLineParser qParser;
    qParser.setApplicationDescription(QApplication::applicationName());
    QCommandLineOption cliVerbose("verbose", QApplication::translate("main", "ALL debug and information messages"));
    QCommandLineOption cliAtomixDir({ "d", "atomix-dir" }, QApplication::translate("main", "parent directory of atomix shaders, configs, etc. (default: the copy shipped with the application)"), "directory");
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

    // Platform
    QString arch = QSysInfo::currentCpuArchitecture();
    QString os = QSysInfo::prettyProductName();
    isMacOS = os.contains("macOS");

    // Locate the atomix files directory (the one holding configs/ and shaders/). Candidates in order:
    //   1. --atomix-dir on the command line
    //   2. the packaged location, relative to the executable (never the working directory, which is
    //      arbitrary when launched from Finder, a file manager, or a Start-menu shortcut)
    //   3. a directory the user previously picked in the dialog below
    //   4. the source tree this binary was built from (developer builds)
    // If none of them has configs/ and shaders/, the user is asked to pick a folder.
    const QString exeDir = QCoreApplication::applicationDirPath();
#if defined(Q_OS_MACOS)
    const QString bundledDir = exeDir + "/../Resources";             // atomix.app/Contents/Resources
#elif defined(Q_OS_WIN)
    const QString bundledDir = exeDir;                               // flat folder next to atomix.exe
#else
    const QString bundledDir = exeDir + "/../share/atomix";          // <prefix>/share/atomix (AppImage: usr/share/atomix)
#endif
    settings.beginGroup("atomixFiles");
    const QString savedDir = settings.value("root").toString();
    settings.endGroup();

    QStringList candidates;
    if (qParser.isSet(cliAtomixDir)) {
        candidates << qParser.value(cliAtomixDir);
    }
    candidates << bundledDir;
    if (!savedDir.isEmpty()) {
        candidates << savedDir;
    }
    candidates << QStringLiteral(ATOMIX_DEV_DIR);

    QString strAtomixDir;
    for (const QString &candidate : candidates) {
        if (mainWindow.getAtomixFiles().setRoot(QDir(candidate).absolutePath().toStdString())) {
            strAtomixDir = QDir(candidate).absolutePath();
            break;
        }
    }

    // Nothing found: show dialog until the user picks a valid folder or cancels. Remember the choice.
    QDir atomixDir(strAtomixDir.isEmpty() ? QDir::homePath() : strAtomixDir);
    bool pickedByUser = false;
    while (strAtomixDir.isEmpty() && !mainWindow.getAtomixFiles().setRoot(atomixDir.absolutePath().toStdString())) {
        pickedByUser = true;
        QMessageBox dialogConfim;
        dialogConfim.setText("Please choose the folder (\"atomix Files Directory\") containing the \"configs\" and \"shaders\" folders shipped with the program or provided by you.");
        dialogConfim.setStandardButtons(QMessageBox::Ok);
        dialogConfim.setDefaultButton(QMessageBox::Ok);
        dialogConfim.exec();

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
    if (strAtomixDir.isEmpty()) {
        strAtomixDir = atomixDir.absolutePath();
    }
    QIcon icoAtomix(QString::fromStdString(mainWindow.getAtomixFiles().resources()) + QString::fromStdString("icons/favicon.ico"));
    app.setWindowIcon(icoAtomix);

    // Remember a folder the user had to pick by hand. (Packaged locations are found again automatically,
    // and saving them would pin an old install's files after the app is moved or upgraded.)
    if (pickedByUser) {
        settings.beginGroup("atomixFiles");
        settings.setValue("root", strAtomixDir);
        settings.endGroup();
    }

    // Debug Info
    if (isDebug) {
        std::cout << "OS: " << os.toStdString() << " (" << arch.toStdString() << ")" << std::endl;
        std::cout << "Qt Version: " << QT_VERSION_STR << std::endl;
        std::cout << "Atomix Directory: " << strAtomixDir.toStdString() << std::endl;
        // QLoggingCategory::setFilterRules("*:debug=true");
        QLoggingCategory::setFilterRules("qt.vulkan=true");
    }
    
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
