/**
 * MainWindow.hpp
 *
 *    Created on: Oct 3, 2023
 *   Last Update: Oct 14, 2023
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *  Copyright 2023 Wade Burch (GPLv3)
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
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QButtonGroup>
#include "Window.hpp"

const QString EMPTY = "Default";

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();
    void lockConfig(WaveConfig *cfg);

protected:
    void keyPressEvent(QKeyEvent *event) override;

signals:
    void sendConfig(WaveConfig *cfg);

private slots:
    void onAddNew();

private:
    //Window *container = nullptr;
    QWidget *wDock = nullptr;
    QDockWidget *controlBox = nullptr;
    QComboBox *qCombo = nullptr;
    QPushButton *qMorb = nullptr;
    QVBoxLayout *layGrid = nullptr;
    QVBoxLayout *cfgGrid = nullptr;
    QTableWidget *cfgTable = nullptr;
    ConfigParser *cfgParser = nullptr;

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

    QButtonGroup *buttGroupOrtho = nullptr;
    QButtonGroup *buttGroupSuper = nullptr;
    QButtonGroup *buttGroupCPU = nullptr;
    QButtonGroup *buttGroupSphere = nullptr;

    GWidget *graph = nullptr;

    void loadConfig();
    void setupDock();
    void refreshConfigs();
    void refreshShaders();

    void handleMorb();
};

#endif