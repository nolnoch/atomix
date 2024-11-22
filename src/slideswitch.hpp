/**
 * slideswitch.hpp
 *
 *    Created on: Oct 15, 2024
 *   Last Update: Oct 15, 2024
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *  Copyright 2024 Wade Burch (GPLv3)
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

#ifndef SLIDESWITCH_H
#define SLIDESWITCH_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QPainter>
#include <QPen>
#include <QPropertyAnimation>
#include <QResizeEvent>

class SlideSwitch : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(SlideSwitch)

public:
    explicit SlideSwitch(QString strTrue, QString strFalse, int width, int height, QWidget* parent = nullptr);
    ~SlideSwitch() override;

    void redraw();

    //-- QWidget methods
    void mousePressEvent(QMouseEvent *) override;
    void paintEvent(QPaintEvent* event) override;
    void setEnabled(bool);

    //-- Setters
    void setDuration(int);
    void setValue(bool);

    //-- Getters
    bool value() const;
    QSize sizeHint() const override { return QSize(slsw_width, slsw_height); }

signals:
    void valueChanged(bool newvalue);

public slots:
    void _toggleBG();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    class SwitchCircle;
    class SwitchBackground;
    void _update();
    void toggle();

    bool slsw_value;
    int  slsw_duration;

    QLinearGradient linGrad_border;
    QLinearGradient linGrad_enabledOff;
    QLinearGradient linGrad_disabled;

    QColor slsw_pencolor;
    QColor slsw_offcolor;
    QColor slsw_oncolor;
    int slsw_borderRadius = 12;
    int slsw_width = 160;
    int slsw_height = 26;
    int fontPx = 12;
    int buttMove = 0;

    // This order for definition is important (these widgets overlap)
    QLabel*           slsw_LabelOff;
    SwitchBackground* slsw_SwitchBackground;
    QLabel*           slsw_LabelOn;
    SwitchCircle*     slsw_Button;

    bool slsw_enabled;

    QPropertyAnimation* prAnim_buttMove;
    QPropertyAnimation* prAnim_backMove;

    struct myPalette {
        QBrush base;
        QBrush alt;
        QBrush high;
        QBrush text;
        QBrush textHigh;
        QBrush light;
    } pal;

    QString _strOff = QString("QLabel#switchOff { color: %1; font-size: %2px; }");
    QString _strOn = QString("QLabel#switchOn { color: %1; font-size: %2px; }");
};

class SlideSwitch::SwitchBackground : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(SwitchBackground)

public:
    explicit SwitchBackground(QColor color, SlideSwitch *parent = nullptr);
    ~SwitchBackground() override;

    void updateSize();

    //-- QWidget methods
    void paintEvent(QPaintEvent* event) override;
    void setEnabled(bool);

private:
    SlideSwitch     *parentPtr;
    QColor          slsb_color;
    QColor          slsb_pencolor;
    QLinearGradient slsb_linGrad_enabled;
    QLinearGradient slsb_linGrad_disabled;
    int             slsb_borderRadius;

    bool            slsb_enabled;
};


class SlideSwitch::SwitchCircle : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(SwitchCircle)

public:
    explicit SwitchCircle(int radius, SlideSwitch* parent = nullptr);
    ~SwitchCircle() override;

    void updateSize();

    //-- QWidget methods
    void paintEvent(QPaintEvent* event) override;
    void setEnabled(bool);

private:
    SlideSwitch     *parentPtr;
    int             slsc_buttRadius;
    int             slsc_borderRadius;
    QColor          slsc_color;
    QColor          slsc_pencolor;
    QRadialGradient slsc_radGrad_button;
    // QLinearGradient _lg;
    QLinearGradient slsc_linGrad_disabled;

    bool            slsc_enabled;
};

#endif
