/**
 * GWidget.hpp
 *
 *    Created on: Oct 3, 2023
 *   Last Update: Oct 14, 2023
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *  Copyright 2023 Wade Burch (GPLv3)
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

#ifndef GWIDGET_H
#define GWIDGET_H


#include <QScreen>
#include <QBoxLayout>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QOpenGLFunctions_4_5_Core>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "program.hpp"
#include "quaternion.hpp"

const double H = 6.626070e-34;      // Planck's constant
const double C = 299792458;         // Speed of massless particles
const double HC = 1.98644586e-25;   // Convenience product of above
const double L = 2 * M_PI;          // For this model, lambda = 2pi
const double E = 3.16152677e-26;    // E = HC / L

const int WAVES = 4;                // Number of wave-circles
const int STEPS = 180;              // Wave-circle resolution


class GWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    GWidget(QWidget *parent = nullptr);
    ~GWidget();

public slots:
    void cleanup();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);

private:
    void initVecsAndMatrices();
    void crystalProgram();
    void waveProgram(uint radius);

    QOpenGLContext *gw_context = nullptr;
    Program *crystalProg = nullptr;
    std::vector<Program *> waveProgs;

    GLuint id_crystalVBO = 0;
    GLuint id_crystalEBO = 0;
    GLuint id_waveVBO_1 = 0;
    GLuint id_waveVBO_2 = 0;
    GLuint id_waveVBO_3 = 0;
    GLuint id_waveVBO_4 = 0;
    GLuint id_waveEBO_1 = 0;
    GLuint id_waveEBO_2 = 0;
    GLuint id_waveEBO_3 = 0;
    GLuint id_waveEBO_4 = 0;
    glm::mat4 m4_proj;
    glm::mat4 m4_view;
    glm::mat4 m4_world;
    glm::mat4 m4_rotation;
    glm::mat4 m4_translation;
    glm::vec3 v3_cameraPosition;
    glm::vec3 v3_cameraTarget;
    glm::vec3 v3_cameraUp;
    glm::vec3 v3_orbitBegin;
    glm::vec3 v3_orbitEnd;
    glm::vec3 v3_slideBegin;
    glm::vec3 v3_slideEnd;
    glm::vec3 v3_rollBegin;
    glm::vec3 v3_rollEnd;
    Quaternion q_TotalRot;
    uint gw_faces = 0;
    uint gw_points = 0;
    int gw_frame = 0;
    bool gw_init = false;
    bool gw_orbiting = false;
    bool gw_sliding = false;
    bool gw_rolling = false;
};

#endif