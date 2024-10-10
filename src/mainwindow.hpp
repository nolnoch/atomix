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

#include <QMainWindow>
#include <QDockWidget>
#include <QMessageBox>
#include <QColorDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QGroupBox>
#include <QButtonGroup>
#include <QComboBox>
#include <QBoxLayout>
#include <QCheckBox>
#include <QListWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QSlider>
#include <QSignalBlocker>
#include <QThread>
#include "GWidget.hpp"

const QString DEFAULT = "default-config.wave";
const int MAX_ORBITS = 8;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();

public slots:
    void updateDetails(AtomixInfo *info);

protected:
    void keyPressEvent(QKeyEvent *event) override;

signals:
    // void sendConfig(AtomixConfig *cfg);
    void changeRenderedOrbits(uint selectedOrbits);

private slots:
    void onAddNew();

private:
    void loadConfig();
    void setupTabs();
    void setupDockWaves();
    void setupDockHarmonics();
    void refreshConfigs();
    void refreshShaders();
    void refreshOrbits();
    void setupDetails();

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
    void handleSlideBackground(int val);
    void handleSlideReleased();

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

    QLineEdit *entryOrbit = nullptr;
    QLineEdit *entryAmp = nullptr;
    QLineEdit *entryPeriod = nullptr;
    QLineEdit *entryWavelength = nullptr;
    QLineEdit *entryResolution = nullptr;
    QRadioButton *entryOrtho = nullptr;
    QRadioButton *entryPara = nullptr;
    QRadioButton *entrySuperOn = nullptr;
    QRadioButton *entrySuperOff = nullptr;
    QRadioButton *entryCPU = nullptr;
    QRadioButton *entryGPU = nullptr;
    QRadioButton *entryCircle = nullptr;
    QRadioButton *entrySphere = nullptr;
    QComboBox *entryVertex = nullptr;
    QComboBox *entryFrag = nullptr;
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
    QGroupBox *groupSlideCulling = nullptr;
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

    GWidget *graph = nullptr;
    harmap mapCloudRecipesLocked;
    int numRecipes = 0;

    int intTabMinWidth = 500;
    int intTabLabelHeight = 200;
    int intSliderLen = 100;
    float lastSliderSentX = 0.0f;
    float lastSliderSentY = 0.0f;

    bool recipeLoaded = false;

    AtomixInfo dInfo;
};

#endif