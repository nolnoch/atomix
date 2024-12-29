/**
 * slideswitch.cpp
 *
 *    Created on: Oct 15, 2024
 *   Last Update: Dec 29, 2024
 *  Orig. Author: Wade Burch (dev@nolnoch.com)
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
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QStackedWidget>
#include <QFontMetrics>
#include "slideswitch.hpp"


SlideSwitch::SlideSwitch(QString strTrue, QString strFalse, int width, int height, QWidget* parent)
  : slsw_width(width), slsw_height(height), slsw_duration(100), slsw_enabled(true), slsw_value(false) {
    setParent(parent);
    slsw_extend2 = slsw_extend << 1;
    this->slsw_sub_height = slsw_height - slsw_extend2;
    this->slsw_borderRadius = slsw_sub_height >> 1;
    QSizePolicy sp = this->sizePolicy();
    sp.setHorizontalPolicy(QSizePolicy::Preferred);
    sp.setVerticalPolicy(QSizePolicy::Expanding);
    this->setSizePolicy(sp);
    
    this->setCheckable(true);
    
    // Grab Palette from parent context (dark mode vs light mode colours)
    this->pal.base = this->palette().brush(QPalette::Base);
    this->pal.alt = this->palette().brush(QPalette::AlternateBase);
    this->pal.high = this->palette().brush(QPalette::Highlight);
    this->pal.text = this->palette().brush(QPalette::Text);
    this->pal.textHigh = this->palette().brush(QPalette::HighlightedText);
    this->pal.light = this->palette().brush(QPalette::Light);
    
    // Set linear gradients (not currently used much)
    int midline = slsw_width >> 1;
    linGrad_border = QLinearGradient(midline, 0, midline, slsw_height);
    linGrad_border.setColorAt(0, QColor(this->pal.alt.color().lighter(120)));
    linGrad_border.setColorAt(0.40, QColor(this->pal.alt.color()));
    linGrad_border.setColorAt(0.60, QColor(this->pal.alt.color()));
    linGrad_border.setColorAt(1, QColor(this->pal.alt.color().lighter(120)));

    linGrad_enabledOff = QLinearGradient(midline, 0, midline, slsw_height);
    linGrad_enabledOff.setColorAt(0, QColor(this->pal.base.color().lighter(140)));
    linGrad_enabledOff.setColorAt(0.35, QColor(this->pal.base.color().lighter(120)));
    linGrad_enabledOff.setColorAt(0.50, QColor(this->pal.base.color()));
    linGrad_enabledOff.setColorAt(0.65, QColor(this->pal.base.color().lighter(120)));
    linGrad_enabledOff.setColorAt(1, QColor(this->pal.base.color().lighter(140)));

    linGrad_disabled = QLinearGradient(midline, 0, midline, slsw_height);
    linGrad_disabled.setColorAt(0, QColor(Qt::lightGray));
    linGrad_disabled.setColorAt(0.40, QColor(Qt::darkGray));
    linGrad_disabled.setColorAt(0.60, QColor(Qt::darkGray));
    linGrad_disabled.setColorAt(1, QColor(Qt::lightGray));
    
    // Create background and button
    slsw_offcolor = this->pal.base.color();
    slsw_oncolor = this->pal.high.color();
    slsw_SwitchBackground = new SwitchBackground(slsw_oncolor, this);
    slsw_Button = new SwitchCircle(slsw_height, this);
    
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
    text = strFalse;
    slsw_LabelOff->setObjectName("switchOff");
    slsw_LabelOn->setObjectName("switchOn");
    strOff = _strOff.arg(pal.text.color().name());
    strOn = _strOn.arg(pal.textHigh.color().name());
    strDis = _strDis.arg(pal.text.color().darker(200).name());
    slsw_LabelOff->setStyleSheet(strOff);
    slsw_LabelOn->setStyleSheet(strOn);
    
    _adjust();

    // Last touches
    slsw_SwitchBackground->resize(slsw_height - 1, slsw_height - 1);
    slsw_SwitchBackground->move(0, 1);
    // buttMove = int(double(slsw_height) * 0.06);
    buttMove = 0;
    slsw_Button->move(buttMove, 0);

    slsw_SwitchBackground->hide();
    slsw_LabelOn->hide();
    slsw_LabelOff->show();

    connect(prAnim_backMove, &QPropertyAnimation::finished, this, &SlideSwitch::_toggleBG);
}

SlideSwitch::~SlideSwitch() {
    delete slsw_Button;
    delete slsw_SwitchBackground;
    delete slsw_LabelOff;
    delete slsw_LabelOn;
    delete prAnim_buttMove;
    delete prAnim_backMove;
}

void SlideSwitch::paintEvent(QPaintEvent*) {
    QPainter* painter = new QPainter;
    QPen penDark(this->pal.base.color().darker(160), 1, Qt::SolidLine);
    // QPen penDebug(Qt::green, 1, Qt::SolidLine);
    QRect rect;

    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing, true);

    painter->setPen(penDark);
    painter->setBrush(this->pal.base);
    rect = QRect(2, slsw_extend, slsw_width - 4, slsw_sub_height);
    painter->drawRoundedRect(rect, slsw_borderRadius, slsw_borderRadius);

    painter->end();
}

void SlideSwitch::setEnabled(bool flag) {
    slsw_enabled = flag;
    slsw_Button->setEnabled(flag);
    slsw_SwitchBackground->setEnabled(flag);
    if (flag) {
        // If switch enabled
        slsw_LabelOff->setStyleSheet(strOff);
        slsw_LabelOn->setStyleSheet(strOn);
    } else {
        // If switch disabled
        slsw_LabelOff->setStyleSheet(strDis);
        slsw_LabelOn->setStyleSheet(strDis);
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
        _toggle();
    }
}

bool SlideSwitch::value() const {
    return slsw_value;
}

QSize SlideSwitch::sizeHint() const {
    // Calculate ideal size based on text and font size
    QFontMetrics fm(slsw_LabelOff->font());
    int fmHeight = fm.height();
    int minHeight = fmHeight + (slsw_margin << 1);

    int offWidth = fm.horizontalAdvance(slsw_LabelOff->text());
    int onWidth = fm.horizontalAdvance(slsw_LabelOn->text());
    int padding = minHeight * 3;
    int minWidth = std::max(offWidth, onWidth) + padding;
    
    minHeight += slsw_extend2;

    return QSize(minWidth, minHeight);
}

QSize SlideSwitch::minimumSizeHint() const {
    // Calculate minimum size based on text and font size
    QFontMetrics fm(slsw_LabelOff->font());
    int fmHeight = fm.height();
    int minHeight = fmHeight + (slsw_margin << 1);

    int offWidth = fm.horizontalAdvance(slsw_LabelOff->text());
    int onWidth = fm.horizontalAdvance(slsw_LabelOn->text());
    int padding = minHeight * 3;
    int minWidth = std::max(offWidth, onWidth) + padding;

    minHeight += slsw_extend2;

    return QSize(minWidth, minHeight);
}

void SlideSwitch::_toggle() {
    if (!slsw_enabled) {
        return;
    }

    prAnim_buttMove->stop();
    prAnim_backMove->stop();

    prAnim_buttMove->setDuration(slsw_duration);
    prAnim_backMove->setDuration(slsw_duration);

    // Movement values
    int hback = slsw_borderRadius >> 1;
    QSize initial_size(hback, hback);
    QSize final_size(slsw_width - hback, hback);

    int xi = buttMove;
    int y  = 0;
    // int xf = this->width() - (slsw_height);
    int xf = slsw_width - slsw_height;

    if (slsw_value) {
        final_size = QSize(hback, hback);
        initial_size = QSize(slsw_width - hback, hback);

        xi = xf;
        xf = buttMove;
    }

    prAnim_buttMove->setStartValue(QPoint(xi, y));
    prAnim_buttMove->setEndValue(QPoint(xf, y));

    prAnim_backMove->setStartValue(initial_size);
    prAnim_backMove->setEndValue(final_size);

    // Start animation
    prAnim_buttMove->start();
    prAnim_backMove->start();

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
}

void SlideSwitch::_toggleBG() {
    if (!slsw_value) slsw_SwitchBackground->hide();
}

void SlideSwitch::click() {
    QAbstractButton::click();
    this->_toggle();
}

void SlideSwitch::setChecked(bool newValue) {
    QAbstractButton::setChecked(newValue);
    this->setValue(newValue);
}

void SlideSwitch::toggle() {
    QAbstractButton::toggle();
    this->_toggle();
}

void SlideSwitch::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    QSize newSize = event->size();
    this->slsw_width  = newSize.width();
    this->slsw_height = newSize.height();

    this->slsw_sub_height = this->slsw_height - this->slsw_extend2;
    this->slsw_borderRadius = this->slsw_sub_height >> 1;

    slsw_SwitchBackground->updateSize();
    slsw_Button->updateSize();

    _adjust();

    // Position button
    // buttMove = int(double(slsw_height) * 0.06);
    buttMove = 0;
    QPoint newPos((slsw_value) ? (this->width() - this->height()) : buttMove, 0);
    slsw_Button->move(newPos);
}

bool SlideSwitch::hitButton(const QPoint &pos) const {
    QRect rect = QRect(0, 0, this->width(), this->height());
    bool hit = rect.contains(pos);
    return hit;
}

void SlideSwitch::nextCheckState() {
    if (isCheckable()) {
        setChecked(!isChecked());
    }
}

void SlideSwitch::checkStateSet() {
    this->setValue(isChecked());
}

void SlideSwitch::_adjust() {
    // Adjust font size
    int pxSize = (this->font().pixelSize() > 0) ? this->font().pixelSize() : 17;
    slsw_font.setPixelSize(pxSize);
    slsw_LabelOff->setFont(slsw_font);
    slsw_LabelOn->setFont(slsw_font);

    // Adjust labels
    QFontMetrics fm(slsw_LabelOff->font());
    slsw_LabelOff->adjustSize();
    slsw_LabelOn->adjustSize();
    int labOffCenter = slsw_LabelOff->sizeHint().width() >> 1;
    int labOnCenter = slsw_LabelOn->sizeHint().width() >> 1;
    int switchCenter = slsw_width >> 1;
    // slsw_margin = (fm.height() >> 2);
    slsw_LabelOff->move(switchCenter - labOffCenter, slsw_margin + slsw_extend);
    slsw_LabelOn->move(switchCenter - labOnCenter, slsw_margin + slsw_extend);
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
  : QWidget(parent), parentPtr(parent), slsb_color_en(color) {
    slsb_color_dis = parentPtr->pal.base.color();
    this->slsb_width = parentPtr->slsw_width - 4;
    this->slsb_height = parentPtr->slsw_sub_height - 4;
    this->slsb_borderRadius = this->slsb_height >> 1;
    this->slsb_offset = parentPtr->slsw_extend;
    setFixedSize(this->slsb_width, this->slsb_height);
    int cx = this->slsb_width;
    int cy = this->slsb_height;

    slsb_linGrad_enabled = QLinearGradient(cx, 0, cx, cy);
    slsb_linGrad_enabled.setColorAt(0, slsb_color_en.darker(140));
    slsb_linGrad_enabled.setColorAt(0.10, slsb_color_en.darker(120));
    slsb_linGrad_enabled.setColorAt(0.20, slsb_color_en.darker(110));
    slsb_linGrad_enabled.setColorAt(0.40, slsb_color_en.lighter(105));
    slsb_linGrad_enabled.setColorAt(0.60, slsb_color_en.darker(110));
    slsb_linGrad_enabled.setColorAt(0.80, slsb_color_en.darker(120));
    slsb_linGrad_enabled.setColorAt(1, slsb_color_en.darker(140));

    slsb_linGrad_disabled = QLinearGradient(cx, 0, cx, cy);
    slsb_linGrad_disabled.setColorAt(0, slsb_color_dis.darker(140));
    slsb_linGrad_disabled.setColorAt(0.10, slsb_color_dis.darker(120));
    slsb_linGrad_disabled.setColorAt(0.20, slsb_color_dis.darker(110));
    slsb_linGrad_disabled.setColorAt(0.40, slsb_color_dis.lighter(105));
    slsb_linGrad_disabled.setColorAt(0.60, slsb_color_dis.darker(110));
    slsb_linGrad_disabled.setColorAt(0.80, slsb_color_dis.darker(120));
    slsb_linGrad_disabled.setColorAt(1, slsb_color_dis.darker(140));

    slsb_enabled = true;
}

SlideSwitch::SwitchBackground::~SwitchBackground() {
}

void SlideSwitch::SwitchBackground::paintEvent(QPaintEvent*) {
    QPainter* painter = new QPainter;
    QPen penNo(Qt::NoPen);

    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing, true);

    painter->setPen(penNo);
    painter->setBrush((slsb_enabled) ? slsb_linGrad_enabled : slsb_linGrad_disabled);
    QRect rect = QRect(1, 0, slsb_width - 2, slsb_height);
    painter->drawRoundedRect(rect, slsb_borderRadius, slsb_borderRadius);

    painter->end();
}

void SlideSwitch::SwitchBackground::setEnabled(bool flag) {
    slsb_enabled = flag;
}

void SlideSwitch::SwitchBackground::updateSize() {
    this->slsb_width = parentPtr->slsw_width - 2;
    this->slsb_height = parentPtr->slsw_sub_height;
    this->slsb_borderRadius = this->slsb_height >> 1;
    setFixedSize(this->slsb_width, this->slsb_height);
    int cx = this->slsb_width;
    int cy = this->slsb_height;

    slsb_linGrad_enabled.setStart(cx, 0);
    slsb_linGrad_enabled.setFinalStop(cx, cy);
    slsb_linGrad_disabled.setStart(cx, 0);
    slsb_linGrad_disabled.setFinalStop(cx, cy);

    this->move(1,slsb_offset);
}

SlideSwitch::SwitchCircle::SwitchCircle(int radius, SlideSwitch *parent)
  : QWidget(parent), parentPtr(parent), slsc_buttRadius(radius), slsc_borderRadius(12) {
    setFixedSize(slsc_buttRadius, slsc_buttRadius);
    int buttRad = slsc_buttRadius >> 1;

    slsc_radGrad_button = QRadialGradient(buttRad, buttRad, buttRad);
    slsc_radGrad_button.setColorAt(0, parentPtr->pal.light.color().lighter(200));
    slsc_radGrad_button.setColorAt(0.20, parentPtr->pal.light.color().lighter(220));
    slsc_radGrad_button.setColorAt(0.65, parentPtr->pal.light.color().lighter(275));
    slsc_radGrad_button.setColorAt(0.88, parentPtr->pal.base.color());
    slsc_radGrad_button.setColorAt(1, parentPtr->pal.base.color().darker(150));

    slsc_radGrad_disabled = QRadialGradient(buttRad, buttRad, buttRad);
    slsc_radGrad_disabled.setColorAt(0, parentPtr->pal.alt.color().lighter(200));
    slsc_radGrad_disabled.setColorAt(0.20, parentPtr->pal.alt.color().lighter(220));
    slsc_radGrad_disabled.setColorAt(0.65, parentPtr->pal.alt.color().lighter(275));
    slsc_radGrad_disabled.setColorAt(0.88, parentPtr->pal.alt.color().darker(150));
    slsc_radGrad_disabled.setColorAt(1, parentPtr->pal.alt.color().darker(250));

    slsc_enabled = true;
}

SlideSwitch::SwitchCircle::~SwitchCircle() {
}

void SlideSwitch::SwitchCircle::paintEvent(QPaintEvent*) {
    QPainter* painter = new QPainter;
    QPen pen(Qt::NoPen);

    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(pen);

    // Outer border
    painter->setBrush(parentPtr->pal.base);
    painter->drawEllipse(0, 0, slsc_buttRadius, slsc_buttRadius);

    // Middle border [highlight]
    painter->setBrush((slsc_enabled) ? parentPtr->pal.high : parentPtr->pal.light.color().lighter(200));
    painter->drawEllipse(1, 1, slsc_buttRadius - 2, slsc_buttRadius - 2);

    // Inner border
    painter->setBrush(parentPtr->pal.base);
    painter->drawEllipse(3, 3, slsc_buttRadius - 6, slsc_buttRadius - 6);

    // Button
    painter->setBrush((slsc_enabled) ? slsc_radGrad_button : slsc_radGrad_disabled);
    painter->drawEllipse(4, 4, slsc_buttRadius - 8, slsc_buttRadius - 8);

    painter->end();
}
void SlideSwitch::SwitchCircle::setEnabled(bool flag) {
    slsc_enabled = flag;
}

void SlideSwitch::SwitchCircle::updateSize() {
    int circleRad = parentPtr->slsw_height;
    slsc_buttRadius = circleRad;

    int buttRad = slsc_buttRadius >> 1;
    setFixedSize(circleRad, circleRad);
    slsc_radGrad_button.setCenter(buttRad, buttRad);
    slsc_radGrad_button.setFocalPoint(buttRad, buttRad);
    slsc_radGrad_button.setRadius(buttRad);
}
