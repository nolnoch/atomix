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
    graph = new GWidget;
    //graph->setFixedSize(400, 400);
    morb = new QPushButton("Morb", this);
    slide = new QSlider(Qt::Vertical, this);
    slide->setTickPosition(QSlider::TicksRight);
    slide->setTickInterval(1);
    slide->setSingleStep(1);
    slide->setFixedWidth(80);

    QWidget *w = new QWidget;
    QVBoxLayout *verGrid = new QVBoxLayout;
    QHBoxLayout *horGrid = new QHBoxLayout;
    horGrid->addWidget(graph);
    horGrid->addWidget(slide);
    w->setLayout(horGrid);
    verGrid->addWidget(w);
    verGrid->addWidget(morb);
    setLayout(verGrid);

    setWindowTitle(tr("atomix"));
}

void Window::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
        close();
    else
        QWidget::keyPressEvent(e);
}
