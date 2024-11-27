/**
 * mainwindow.hpp
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QtWidgets/QMainWindow>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QSlider>
#include <QtWidgets/QProgressBar>
#include <QPixmap>
#include <QIcon>
#include <QSignalBlocker>
#include <QThread>
#include "slideswitch.hpp"
#include "configparser.hpp"

#ifdef USING_QVULKAN
    #include "vkwindow.hpp"
#elifdef USING_QOPENGL
    #include "glwidget.hpp"
#endif

const QString DEFAULT = "default-config.wave";
const int MAX_ORBITS = 8;

struct AtomixStyle {
    void setConstraintsRatio(double min, double max) {
        scaleMin = min;
        scaleMax = max;
    }

    void setConstraintsPixel(int min, int max) {
        dockMin = min;
        dockMax = max;
    }
    
    void setWindowSize(int width, int height) {
        windowWidth = width;
        windowHeight = height;
    }

    void setDockWidth(int width, int tabCount) {
        dockWidth = width;
        tabWidth = dockWidth / tabCount;
    }
    
    void scaleFonts() {
        baseFont = int(round(dockWidth * 0.04));
        descFont = int(round(baseFont * 1.333));
        tabSelectedFont = int(round(baseFont * 1.15));
        tabUnselectedFont = int(round(baseFont * 0.90));
        treeFont = baseFont + 4;
        tableFont = baseFont + 1;
        listFont = baseFont + 1;
        morbFont = baseFont + 5;
    }

    void scaleWidgets() {
        treeCheckSize = int(round(baseFont * 1.5));
        tabLabelHeight = windowHeight / 9;
        sliderTicks = 20;
        sliderInterval = sliderTicks >> 2;
        groupMaxWidth = dockWidth >> 1;
        borderWidth = (isMacOS) ? 1 : 3;
        defaultMargin = 0;
        defaultPadding = 0;
        defaultSpacing = 0;
        listPadding = layDockSpace;
    }

    void updateStyleSheet() {
        scaleFonts();
        scaleWidgets();

        // Note: WidgetItems must have border defined even as 0px for margin/padding to work
        styleStringList = QStringList({
            "QWidget { font-size: %1px; } "
            "QTabBar::tab { height: 40px; width: %3px; font-size: %4px; } "
            "QTabBar::tab::selected { font-size: %5px; } "
            "QLabel { font-size: %1px; } "
            "QLabel#tabDesc { font-size: %2px; } "
            "QTreeWidget { font-family: %15; font-size: %6px; } "
            "QTableWidget { font-family: %15; font-size: %7px; } "
            "QListWidget { font-family: %15; font-size: %8px; } "
            // "QTreeWidget::item { border: 0px; padding-top: 10px; } " <== This works but ruins the formatting
            "QTableWidget::item { border: 0px; margin: %12px; padding: %13px; spacing: %14px; padding-top: %9px; } "
            "QListWidget::item { border: 0px; margin: %12px; padding: %13px; spacing: %14px; padding-top: %9px; } "
            "QPushButton#morb { font-size: %10px; margin-right: %11px; margin-left: %11px; } "
        });

        genStyleString();
    }

    void genStyleString() {
        strStyle = styleStringList.join(" ")
            .arg(QString::number(baseFont))             // 1
            .arg(QString::number(descFont))             // 2
            .arg(QString::number(tabWidth))             // 3
            .arg(QString::number(tabUnselectedFont))    // 4
            .arg(QString::number(tabSelectedFont))      // 5
            .arg(QString::number(treeFont))             // 6
            .arg(QString::number(tableFont))            // 7
            .arg(QString::number(listFont))             // 8
            .arg(QString::number(listPadding))          // 9
            .arg(QString::number(morbFont))             // 10
            .arg(QString::number(morbMargin))           // 11
            .arg(QString::number(defaultMargin))        // 12
            .arg(QString::number(defaultPadding))       // 13
            .arg(QString::number(defaultSpacing))       // 14
            .arg(strFontInc);                           // 15
            // .arg(QString::number(treeCheckSize));       // 15
    }

    QString& getStyleSheet() {
        return strStyle;
    }

    uint baseFont, tabSelectedFont, tabUnselectedFont, descFont, treeFont, tableFont, listFont, morbFont, morbMargin;
    uint dockWidth, tabWidth, tabLabelHeight, sliderTicks, sliderInterval, borderWidth, groupMaxWidth, treeCheckSize;
    uint defaultMargin, defaultPadding, defaultSpacing, listPadding, layDockSpace;

    uint dockMin, dockMax, windowWidth, windowHeight;
    double scaleMin, scaleMax, scale;

    QStringList styleStringList;
    QString strStyle, strFontInc;
    QFont fontInc;
};


class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();
    void init(QRect &windowSize);
    void postInit(int titlebarHeight);

signals:
    void changeRenderedOrbits(uint selectedOrbits);

public slots:
    void updateDetails(AtomixInfo *info);
    void setLoading(bool loading);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupTabs();
    void setupDockWaves();
    void setupDockHarmonics();
    void setupStyleSheet();
    void refreshConfigs();
    void refreshShaders();
    void loadConfig();
    void refreshOrbits();
    void setupDetails();
    void setupLoading();

    void handleComboCfg();
    void handleConfigChanged();
    void handleDoubleClick(QTreeWidgetItem *item, int col);
    void handleRecipeCheck(QTreeWidgetItem *item, int col);
    void handleButtLockRecipes();
    void handleButtClearRecipes();
    void handleButtResetRecipes();
    void handleButtMorbWaves();
    void handleButtMorbHarmonics();
    void handleButtConfig(int id, bool checked);
    void handleButtColors(int id);
    void handleWeightChange(int row, int col);
    void handleSlideCullingX(int val);
    void handleSlideCullingY(int val);
    void handleSlideReleased();
    void handleSlideBackground(int val);

    void printHarmap();
    void printList();

    void _dockResize();
    void _resize();

    AtomixConfig waveConfig;
    AtomixConfig cloudConfig;

    QTabWidget *wTabs = nullptr;
    QWidget *wTabWaves = nullptr;
    QWidget *wTabHarmonics = nullptr;
    QDockWidget *dockTabs = nullptr;
    ConfigParser *cfgParser = nullptr;

    QFont fontDebug;
    QIntValidator *valIntSmall = nullptr;
    QIntValidator *valIntLarge = nullptr;
    QDoubleValidator *valDoubleSmall = nullptr;
    QDoubleValidator *valDoubleLarge = nullptr;

    QVBoxLayout *layDockWaves = nullptr;
    QVBoxLayout *layDockHarmonics = nullptr;
    
    QLineEdit *entryOrbit = nullptr;
    QLineEdit *entryAmp = nullptr;
    QLineEdit *entryPeriod = nullptr;
    QLineEdit *entryWavelength = nullptr;
    QLineEdit *entryResolution = nullptr;
    SlideSwitch *slswPara = nullptr;
    SlideSwitch *slswSuper = nullptr;
    SlideSwitch *slswCPU = nullptr;
    SlideSwitch *slswSphere = nullptr;
    QComboBox *comboConfigFile = nullptr;

    QButtonGroup *buttGroupConfig = nullptr;
    QButtonGroup *buttGroupOrbits = nullptr;
    QButtonGroup *buttGroupColors = nullptr;

    QPushButton *buttMorbWaves = nullptr;
    QPushButton *buttLockRecipes = nullptr;
    QPushButton *buttClearRecipes = nullptr;
    QPushButton *buttResetRecipes = nullptr;
    QPushButton *buttMorbHarmonics = nullptr;

    QGroupBox *groupOptions = nullptr;
    QGroupBox *groupColors = nullptr;
    QGroupBox *groupOrbits = nullptr;
    QGroupBox *groupRecipeBuilder = nullptr;
    QGroupBox *groupRecipeReporter = nullptr;
    QGroupBox *groupGenVertices = nullptr;
    QGroupBox *groupRecipeLocked = nullptr;
    QGroupBox *groupHSlideCulling = nullptr;
    QGroupBox *groupVSlideCulling = nullptr;
    QGroupBox *groupSlideBackground = nullptr;
    QTreeWidget *treeOrbitalSelect = nullptr;
    QListWidget *listOrbitalLocked = nullptr;
    QTableWidget *tableOrbitalReport = nullptr;
    QLineEdit *entryCloudLayers = nullptr;
    QLineEdit *entryCloudRes = nullptr;
    QLineEdit *entryCloudMinRDP = nullptr;
    QSlider *slideBackground = nullptr;
    QSlider *slideCullingX = nullptr;
    QSlider *slideCullingY = nullptr;

    QLabel *labelDetails = nullptr;
    QProgressBar *pbLoading = nullptr;

#ifdef USING_QVULKAN
    VKWindow *vkGraph = nullptr;
    QVulkanInstance vkInst;
    QWidget *vkWinWidWrapper = nullptr;
#elifdef USING_QOPENGL
    GWidget *glGraph = nullptr;
#endif
    QWidget *graph = nullptr;
    QWindow *graphWin = nullptr;

    harmap mapCloudRecipesLocked;
    int numRecipes = 0;
    bool activeModel = false;

    int mw_baseFontSize = 0;
    
    float lastSliderSentX = 0.0f;
    float lastSliderSentY = 0.0f;

    int mw_width = 0;
    int mw_height = 0;
    int mw_x = 0;
    int mw_y = 0;
    int mw_titleHeight = 0;
    int mw_tabWidth = 0;
    int mw_tabHeight = 0;
    int mw_tabCount = 0;

    QPixmap *pmColour = nullptr;

    AtomixInfo dInfo;
    AtomixStyle aStyle;

    QLabel *labelDebug = nullptr;
    QGridLayout *layoutDebug = nullptr;
};

#endif
