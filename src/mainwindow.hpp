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

struct AtomixFonts {
    uint main, tabSelected, tabUnselected, large, small, tiny, label, label2, label3, label4, label5;

    void setDPI(int dpi) {
        main = 24 * dpi / 96;
        tabSelected = 20 * dpi / 96;
        tabUnselected = 16 * dpi / 96;
        large = 14 * dpi / 96;
        small = 12 * dpi / 96;
        tiny = 10 * dpi / 96;
        label = 10 * dpi / 96;
        label2 = 8 * dpi / 96;
        label3 = 6 * dpi / 96;
        label4 = 4 * dpi / 96;
        label5 = 2 * dpi / 96;
    }
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
    void handleButtColors(int id);
    void handleWeightChange(int row, int col);
    void handleSlideCullingX(int val);
    void handleSlideCullingY(int val);
    void handleSlideReleased();
    void handleSlideBackground(int val);

    void printHarmap();
    void printList();

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

    QLineEdit *entryOrbit = nullptr;
    QLineEdit *entryAmp = nullptr;
    QLineEdit *entryPeriod = nullptr;
    QLineEdit *entryWavelength = nullptr;
    QLineEdit *entryResolution = nullptr;
    SlideSwitch *slswPara = nullptr;
    SlideSwitch *slswSuper = nullptr;
    SlideSwitch *slswCPU = nullptr;
    SlideSwitch *slswSphere = nullptr;
    /* QComboBox *entryVertex = nullptr;
    QComboBox *entryFrag = nullptr; */
    QComboBox *comboConfigFile = nullptr;

    QButtonGroup *buttGroupOrbits = nullptr;
    QButtonGroup *buttGroupColors = nullptr;

    QPushButton *buttMorbWaves = nullptr;
    QPushButton *buttLockRecipes = nullptr;
    QPushButton *buttClearRecipes = nullptr;
    QPushButton *buttResetRecipes = nullptr;
    QPushButton *buttMorbHarmonics = nullptr;

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
    bool recipeLoaded = false;

    int intGraphWidth = 0;
    int intTabMinWidth = 0;
    int intTabMaxWidth = 0;
    int intTabLabelHeight = 0;
    int intSliderLen = 0;
    int lineWidth = 0;
    int intHarmonicsGroupMaxWidth = 0;
    int slslwWidth = 0;
    
    float lastSliderSentX = 0.0f;
    float lastSliderSentY = 0.0f;

    int mw_width = 0;
    int mw_height = 0;
    int mw_x = 0;
    int mw_y = 0;
    int mw_titleHeight = 0;

    AtomixInfo dInfo;
};

#endif
