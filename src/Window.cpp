/**
 * Window.cpp
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

#include "Window.hpp"

Window::Window(MainWindow *mw)
    : mainWindow(mw) {
    /* Create and arrange window components */
    //QWidget *wWindow = new QWidget;
    graph = new GWidget(this);
    //qCombo = new QComboBox(this);
    //slide = new QSlider(Qt::Vertical, this);
    //qMorb = new QPushButton("Morb", this);
    //QVBoxLayout *verGrid = new QVBoxLayout;
    QHBoxLayout *horGrid = new QHBoxLayout;

    //slide->setTickPosition(QSlider::TicksRight);
    //slide->setTickInterval(1);
    //slide->setSingleStep(1);
    //slide->setFixedWidth(80);

    //verGrid->addWidget(qCombo);
    //verGrid->addWidget(qMorb);
    //w->setLayout(verGrid);
    horGrid->addWidget(graph);
    //horGrid->addWidget(w);
    this->setLayout(horGrid);

    connect(this, &Window::passConfig, graph, &GWidget::configReceived, Qt::DirectConnection);

    setWindowTitle(tr("atomix"));
}
