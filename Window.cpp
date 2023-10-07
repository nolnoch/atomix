/** Window.cpp
 * 
 * atomix by Wade Burch
 * (braernoch@gmail.com)
 * 
 * GPL Open Source
*/

#include "Window.hpp"

Window::Window(MainWindow *mw)
    : mainWindow(mw) {
    /* Create and arrange window components */
    QWidget *w = new QWidget;
    graph = new GWidget(w);
    slide = new QSlider(Qt::Vertical, w);
    morb = new QPushButton("Morb", this);
    QVBoxLayout *verGrid = new QVBoxLayout;
    QHBoxLayout *horGrid = new QHBoxLayout;

    slide->setTickPosition(QSlider::TicksRight);
    slide->setTickInterval(1);
    slide->setSingleStep(1);
    slide->setFixedWidth(80);

    //horGrid->addWidget(graph);
    horGrid->addWidget(slide);
    w->setLayout(horGrid);
    verGrid->addWidget(w);
    verGrid->addWidget(morb);
    this->setLayout(verGrid);

    setWindowTitle(tr("atomix"));
}

void Window::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
        close();
    else
        QWidget::keyPressEvent(e);
}
