/**
 * Window.hpp
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

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QSlider>
#include <QPushButton>
#include <QKeyEvent>
#include "GWidget.hpp"

class GWidget;
class MainWindow;

class Window : public QWidget {
    Q_OBJECT

public:
    Window(MainWindow *mw);

signals:
    void passConfig(WaveConfig *cfg);

private:
    GWidget *graph = nullptr;
    QSlider *slide = nullptr;
    QPushButton *morb = nullptr;
    MainWindow *mainWindow = nullptr;
};

#endif