/** Window.hpp
 * 
 * atomix by Wade Burch
 * (braernoch@gmail.com)
 * 
 * GPL Open Source
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

private:
    GWidget *graph = nullptr;
    QSlider *slide = nullptr;
    QPushButton *morb = nullptr;
    MainWindow *mainWindow = nullptr;
};

#endif