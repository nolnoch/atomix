/**
 * slideswitch.cpp
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

#include <iostream>
#include "slideswitch.hpp"


SlideSwitch::SlideSwitch(QString strTrue, QString strFalse, int width, int height, QWidget* parent)
  : slsw_width(width), slsw_height(height), QWidget(parent),
   slsw_value(false), slsw_duration(100), slsw_enabled(true) {
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    this->setFixedSize(QSize(slsw_width, slsw_height));
    this->slsw_borderRadius = (slsw_height >> 1);
    
    // Grab Palette from parent context (dark mode vs light mode colours)
    this->pal.base = this->palette().brush(QPalette::Base);
    this->pal.alt = this->palette().brush(QPalette::AlternateBase);
    this->pal.high = this->palette().brush(QPalette::Highlight);
    this->pal.text = this->palette().brush(QPalette::Text);
    this->pal.textHigh = this->palette().brush(QPalette::HighlightedText);
    this->pal.light = this->palette().brush(QPalette::Light);
    
    // Set linear gradients (not currently used much)
    linGrad_border = QLinearGradient(80,0,80,26);
    linGrad_border.setColorAt(0, QColor(this->pal.alt.color().lighter(120)));
    linGrad_border.setColorAt(0.40, QColor(this->pal.alt.color()));
    linGrad_border.setColorAt(0.60, QColor(this->pal.alt.color()));
    linGrad_border.setColorAt(1, QColor(this->pal.alt.color().lighter(120)));

    linGrad_enabledOff = QLinearGradient(80,0,80,26);
    linGrad_enabledOff.setColorAt(0, QColor(this->pal.base.color().lighter(140)));
    linGrad_enabledOff.setColorAt(0.35, QColor(this->pal.base.color().lighter(120)));
    linGrad_enabledOff.setColorAt(0.50, QColor(this->pal.base.color()));
    linGrad_enabledOff.setColorAt(0.65, QColor(this->pal.base.color().lighter(120)));
    linGrad_enabledOff.setColorAt(1, QColor(this->pal.base.color().lighter(140)));

    linGrad_disabled = QLinearGradient(80,0,80,26);
    linGrad_disabled.setColorAt(0, QColor(Qt::lightGray));
    linGrad_disabled.setColorAt(0.40, QColor(Qt::darkGray));
    linGrad_disabled.setColorAt(0.60, QColor(Qt::darkGray));
    linGrad_disabled.setColorAt(1, QColor(Qt::lightGray));
    
    // Create background and button
    // slsw_offcolor = this->pal.base.color();
    slsw_oncolor = this->pal.high.color();
    slsw_SwitchBackground = new SwitchBackground(slsw_oncolor, this);
    int circleRad = int(double(slsw_borderRadius) * 1.8);
    slsw_Button = new SwitchCircle(circleRad, this);
    
    // Create animations
    prAnim_buttMove = new QPropertyAnimation(this);
    prAnim_backMove = new QPropertyAnimation(this);
    prAnim_buttMove->setTargetObject(slsw_Button);
    prAnim_buttMove->setPropertyName("pos");
    prAnim_backMove->setTargetObject(slsw_SwitchBackground);
    prAnim_backMove->setPropertyName("size");

    // Create labels
    slsw_LabelOff = new QLabel(this);
    slsw_LabelOn = new QLabel(this);
    slsw_LabelOff->setText(strFalse);
    slsw_LabelOn->setText(strTrue);
    QString strOff = QString("QLabel#switchOff { color: %1; font-size: %2 px; }").arg(pal.text.color().name()).arg(10);
    QString strOn = QString("QLabel#switchOn { color: %1; font-size: %2 px; }").arg(pal.textHigh.color().name()).arg(10);
    slsw_LabelOff->setObjectName("switchOff");
    slsw_LabelOn->setObjectName("switchOn");
    slsw_LabelOff->setStyleSheet(strOff);
    slsw_LabelOn->setStyleSheet(strOn);

    // Position labels
    int labOffCenter = slsw_LabelOff->sizeHint().width() >> 1;
    int labOnCenter = slsw_LabelOff->sizeHint().width() >> 1;
    int switchCenter = slsw_width >> 1;
    slsw_LabelOff->move(switchCenter - labOffCenter, 3);
    slsw_LabelOn->move(switchCenter - labOnCenter, 3);

    // Last touches
    slsw_SwitchBackground->resize(slsw_height - 1, slsw_height - 1);
    slsw_SwitchBackground->move(2, 2);
    slsw_Button->move(1, 1);

    slsw_SwitchBackground->hide();
    slsw_LabelOn->hide();
    slsw_LabelOff->show();
}

SlideSwitch::~SlideSwitch() {
    delete slsw_Button;
    delete slsw_SwitchBackground;
    delete slsw_LabelOff;
    delete slsw_LabelOn;
    delete prAnim_buttMove;
    delete prAnim_backMove;
}

void SlideSwitch::redraw() {
    slsw_LabelOff->adjustSize();
    slsw_LabelOn->adjustSize();
}

void SlideSwitch::paintEvent(QPaintEvent*) {
    QPainter* painter = new QPainter;
    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen pen(Qt::NoPen);
    painter->setPen(pen);

    // Set Outer border [constant]
    painter->setBrush(this->pal.alt);
    painter->drawRoundedRect(0, 0, this->width(), this->height(), slsw_borderRadius, slsw_borderRadius);

    // Set middle border [half-replaced]
    // painter->setBrush(linGrad_border);
    painter->setBrush(this->pal.alt);
    painter->drawRoundedRect(1, 1, this->width() - 2, this->height() - 2, slsw_borderRadius, slsw_borderRadius);

    // Set inner border [fully replaced]
    // painter->setBrush(this->pal.alt);
    // painter->drawRoundedRect(2, 2, this->width() - 4, this->height() - 4, slsw_borderRadius, slsw_borderRadius);

    if (slsw_enabled) {
        // Set Enabled-Off colour
        painter->setBrush(this->pal.base);
        painter->drawRoundedRect(2, 2, this->width() - 4, this->height() - 4, slsw_borderRadius, slsw_borderRadius);
    } else {
        // Set Disabled colour
        painter->setBrush(linGrad_disabled);
        painter->drawRoundedRect(2, 2, this->width() - 4, this->height() - 4, slsw_borderRadius, slsw_borderRadius);
    }
    painter->end();
}

void SlideSwitch::mousePressEvent(QMouseEvent*) {
    this->toggle();
}

void SlideSwitch::setEnabled(bool flag) {
    slsw_enabled = flag;
    slsw_Button->setEnabled(flag);
    slsw_SwitchBackground->setEnabled(flag);
    if (flag) {
        // If switch disabled
    } else {
        // If switch enabled
        if (slsw_value) {
            slsw_LabelOn->show();
            slsw_LabelOff->hide();
        } else {
            slsw_LabelOff->show();
            slsw_LabelOn->hide();
        }
    }
    QWidget::setEnabled(flag);
}

void SlideSwitch::setDuration(int time) {
    slsw_duration = time;
}

void SlideSwitch::setValue(bool flag) {
    if (flag == value()) {
        return;
    } else {
        toggle();
        // _update();
        // setEnabled(slsw_enabled);
    }

    if (slsw_value) {
        slsw_LabelOn->show();
        slsw_LabelOff->hide();
    } else {
        slsw_LabelOff->show();
        slsw_LabelOn->hide();
    }
}

bool SlideSwitch::value() const {
    return slsw_value;
}

void SlideSwitch::toggle() {
    if (!slsw_enabled) {
        return;
    }

    prAnim_buttMove->stop();
    prAnim_backMove->stop();

    prAnim_buttMove->setDuration(slsw_duration);
    prAnim_backMove->setDuration(slsw_duration);

    int hback = 10;
    QSize initial_size(hback, hback);
    QSize final_size(this->width() - 4, hback);

    int xi = 1;
    int y  = 1;
    int xf = this->width() - 19;

    if (slsw_value) {
        final_size = QSize(hback, hback);
        initial_size = QSize(this->width() - 4, hback);

        xi = xf;
        xf = 1;
    }
    // Assigning new current value
    slsw_value = !slsw_value;
    if (slsw_value) {
        slsw_LabelOff->hide();
        slsw_LabelOn->show();
        slsw_SwitchBackground->show();
    } else {
        slsw_LabelOff->show();
        slsw_LabelOn->hide();
    }

    prAnim_buttMove->setStartValue(QPoint(xi, y));
    prAnim_buttMove->setEndValue(QPoint(xf, y));

    prAnim_backMove->setStartValue(initial_size);
    prAnim_backMove->setEndValue(final_size);

    prAnim_buttMove->start();
    prAnim_backMove->start();

    emit valueChanged(slsw_value);
}

void SlideSwitch::_update() {
    int hback = 20;
    QSize final_size(this->width() - 4, hback);

    int y = 2;
    int xf = this->width() - 22;

    if (slsw_value) {
        final_size = QSize(hback, hback);
        xf = 2;
        slsw_LabelOn->show();
        slsw_LabelOff->hide();
    } else {
        slsw_LabelOff->show();
        slsw_LabelOn->hide();
    }

    slsw_Button->move(QPoint(xf, y));
    slsw_SwitchBackground->resize(final_size);
}

SlideSwitch::SwitchBackground::SwitchBackground(QColor color, SlideSwitch *parent)
  : QWidget(parent), parentPtr(parent), slsb_color(color) {
    setFixedHeight(parentPtr->slsw_height - 4);

    slsb_linGrad_enabled = QLinearGradient(80, 0, 80, 20);
    slsb_linGrad_enabled.setColorAt(0, slsb_color.darker(120));
    slsb_linGrad_enabled.setColorAt(0.20, slsb_color.darker(110));
    slsb_linGrad_enabled.setColorAt(0.50, slsb_color);
    slsb_linGrad_enabled.setColorAt(0.80, slsb_color.darker(110));
    slsb_linGrad_enabled.setColorAt(1, slsb_color.darker(120));

    slsb_linGrad_disabled = QLinearGradient(0, 25, 70, 0);
    slsb_linGrad_disabled.setColorAt(0, QColor(190, 190, 190));
    slsb_linGrad_disabled.setColorAt(0.25, QColor(230, 230, 230));
    slsb_linGrad_disabled.setColorAt(0.95, QColor(190, 190, 190));

    slsb_borderRadius = parentPtr->slsw_borderRadius - 2;

    slsb_enabled = true;
}

SlideSwitch::SwitchBackground::~SwitchBackground() {
}

void SlideSwitch::SwitchBackground::paintEvent(QPaintEvent*) {
    QPainter* painter = new QPainter;
    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen pen(Qt::NoPen);
    painter->setPen(pen);
    if (slsb_enabled) {
        // painter->setBrush(parentPtr->pal.light);
        // painter->drawRoundedRect(0, 0, this->width(), this->height(), slsb_borderRadius, slsb_borderRadius);

        painter->setBrush(slsb_linGrad_enabled);
        painter->drawRoundedRect(0, 0, this->width(), this->height(), slsb_borderRadius, slsb_borderRadius);
    } else {
        painter->setBrush(parentPtr->pal.alt);
        painter->drawRoundedRect(0, 0, this->width(), this->height(), slsb_borderRadius, slsb_borderRadius);

        painter->setBrush(slsb_linGrad_disabled);
        painter->drawRoundedRect(1, 1, this->width() - 2, this->height() - 2, slsb_borderRadius, slsb_borderRadius);
    }
    painter->end();
}

void SlideSwitch::SwitchBackground::setEnabled(bool flag) {
    slsb_enabled = flag;
}

SlideSwitch::SwitchCircle::SwitchCircle(int radius, SlideSwitch *parent)
  : QWidget(parent), parentPtr(parent), slsc_buttRadius(radius), slsc_borderRadius(12) {
    setFixedSize(slsc_buttRadius, slsc_buttRadius);

    slsc_radGrad_button = QRadialGradient(static_cast<int>(this->width() / 2), static_cast<int>(this->height() / 2), slsc_buttRadius / 2);
    slsc_radGrad_button.setColorAt(0, parentPtr->pal.light.color().lighter(275));
    slsc_radGrad_button.setColorAt(0.9, parentPtr->pal.light.color());
    // radGrad_button.setColorAt(1, parentPtr->pal.base.color().darker(150));

    /* _lg = QLinearGradient(3, 18, 20, 4);
    _lg.setColorAt(0, QColor(255, 255, 255));
    _lg.setColorAt(0.55, QColor(230, 230, 230));
    _lg.setColorAt(0.72, QColor(255, 255, 255));
    _lg.setColorAt(1, QColor(255, 255, 255)); */

    slsc_linGrad_disabled = QLinearGradient(3, 18, 20, 4);
    slsc_linGrad_disabled.setColorAt(0, QColor(230, 230, 230));
    slsc_linGrad_disabled.setColorAt(0.55, QColor(210, 210, 210));
    slsc_linGrad_disabled.setColorAt(0.72, QColor(230, 230, 230));
    slsc_linGrad_disabled.setColorAt(1, QColor(230, 230, 230));

    slsc_enabled = true;
}

SlideSwitch::SwitchCircle::~SwitchCircle() {
}

void SlideSwitch::SwitchCircle::paintEvent(QPaintEvent*) {
    QPainter* painter = new QPainter;
    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen pen(Qt::NoPen);
    painter->setPen(pen);
    painter->setBrush(parentPtr->pal.base);
    painter->drawEllipse(0, 0, slsc_buttRadius, slsc_buttRadius);

    painter->setBrush(parentPtr->pal.high);
    painter->drawEllipse(1, 1, slsc_buttRadius - 2, slsc_buttRadius - 2);

    painter->setBrush(parentPtr->pal.light);
    painter->drawEllipse(2, 2, slsc_buttRadius - 4, slsc_buttRadius - 4);

    if (slsc_enabled) {
        painter->setBrush(slsc_radGrad_button);
        painter->drawEllipse(3, 3, slsc_buttRadius - 6, slsc_buttRadius - 6);
    } else {
        painter->setBrush(slsc_linGrad_disabled);
        painter->drawEllipse(3, 3, slsc_buttRadius - 6, slsc_buttRadius - 6);
    }

    painter->end();
}
void SlideSwitch::SwitchCircle::setEnabled(bool flag) {
    slsc_enabled = flag;
}
