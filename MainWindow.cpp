/** MainWindow.cpp
 * 
 * atomix by Wade Burch
 * (braernoch@gmail.com)
 * 
 * GPL Open Source
*/

#include "MainWindow.hpp"

MainWindow::MainWindow() {
    onAddNew();
}

void MainWindow::onAddNew() {
    setCentralWidget(new Window(this));
}