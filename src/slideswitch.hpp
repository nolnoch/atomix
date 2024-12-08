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
#include <QAbstractButton>
#include <QPainter>
#include <QPen>
#include <QPropertyAnimation>
#include <QResizeEvent>

class SlideSwitch : public QAbstractButton {
    Q_OBJECT
    Q_DISABLE_COPY(SlideSwitch)

public:
    explicit SlideSwitch(QString strTrue, QString strFalse, int width, int height, QWidget* parent = nullptr);
    ~SlideSwitch() override;

    //-- QWidget methods
    void paintEvent(QPaintEvent* event) override;
    void setEnabled(bool);

    //-- Setters
    void setDuration(int);
    void setValue(bool);

    //-- Getters
    bool value() const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    void _toggleBG();
    void setChecked(bool newValue);
    void click();
    void toggle();

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool hitButton(const QPoint &pos) const override;
    void nextCheckState() override;
    void checkStateSet() override;

private:
    class SwitchCircle;
    class SwitchBackground;
    void _adjust();
    void _update();
    void _toggle();

    QLinearGradient linGrad_border;
    QLinearGradient linGrad_enabledOff;
    QLinearGradient linGrad_disabled;

    QColor slsw_pencolor;
    QColor slsw_offcolor;
    QColor slsw_oncolor;
    int slsw_width = 160;
    int slsw_height = 26;
    int slsw_duration = 100;
    bool slsw_enabled = true;
    bool slsw_value = false;
    int slsw_sub_height = 18;
    int slsw_borderRadius = 12;
    int slsw_extend = 2;
    int slsw_extend2 = 8;
    int fontPx = 12;
    int buttMove = 0;
    double fontScale = 0.50;
    double labMoveScale = 0.18;

    /* bool enabled = false;
    bool checkable = false;
    bool checked = false;
    bool down = false; */
    QString text = "";

    // This order for definition is important (these widgets overlap)
    QLabel*           slsw_LabelOff;
    SwitchBackground* slsw_SwitchBackground;
    QLabel*           slsw_LabelOn;
    SwitchCircle*     slsw_Button;

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

    QFont slsw_font;
    int slsw_margin = 2;
    QString _strOff = QString("QLabel#switchOff { color: %1; }");
    QString _strOn = QString("QLabel#switchOn { color: %1; }");
    QString _strDis = QString("QLabel#switchDis { color: %1; }");
    QString strOff, strOn, strDis;
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
    QColor          slsb_color_en;
    QColor          slsb_color_dis;
    QColor          slsb_pencolor;
    QLinearGradient slsb_linGrad_enabled;
    QLinearGradient slsb_linGrad_disabled;
    int             slsb_borderRadius;
    int             slsb_height;
    int             slsb_width;
    int             slsb_offset = 0;

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
    QRadialGradient slsc_radGrad_disabled;

    bool            slsc_enabled;
};

#endif
