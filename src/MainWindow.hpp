/** MainWindow.hpp
 * 
 * atomix by Wade Burch
 * (braernoch@gmail.com)
 * 
 * GPL Open Source
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Window.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onAddNew();
};

#endif