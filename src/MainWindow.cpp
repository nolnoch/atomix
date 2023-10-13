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

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
        close();
    else
        QWidget::keyPressEvent(e);
}
