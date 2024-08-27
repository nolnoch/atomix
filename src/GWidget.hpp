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
#include <QDateTime>
#include <QOpenGLFunctions_4_5_Core>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "program.hpp"
#include "quaternion.hpp"
#include "orbitmanager.hpp"


/* Program/Orbit pointer struct */
typedef struct {
    std::vector<std::vector<Program *> *> apProgs;      // Vector of pointers to Programs
    std::vector<std::vector<OrbitManager *> *> apOrbits;       // Vector of pointers to Orbits
    std::vector<WaveConfig *> apConfigs;                // Vector of configs
    int apIdxRender = 0;                                // Current pointer index
    int apIdxCreate = 0;                                // Future pointer index
} AtomixProgs;
Q_DECLARE_METATYPE(AtomixProgs);

enum changeFlags {ORBITS = 1, AMPLITUDE = 2, PERIOD = 4, WAVELENGTH = 8, RESOLUTION = 16, PARALLEL = 32, SUPERPOSITION = 64, CPU = 128, SPHERE = 256, VERTSHADER = 512, FRAGSHADER = 1024};


class GWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    GWidget(QWidget *parent = nullptr, ConfigParser *configParser = nullptr);
    ~GWidget();

    void printConfig(WaveConfig *cfg);

public slots:
    void cleanup();
    void configReceived(WaveConfig *cfg);

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
    void initWaveProgram();
    void processConfigChange();
    void swapShaders();
    void swapBuffers();
    void swapVertices();
    void clearProgram(uint i);
    void checkErrors(string str);

    QOpenGLContext *gw_context = nullptr;
    Program *crystalProg = nullptr;
    Program *waveProg = nullptr;
    ConfigParser *cfgParser = nullptr;
    OrbitManager *orbitManager = nullptr;
    QTimer *gw_timer = nullptr;

    WaveConfig renderConfig;
    glm::mat4 m4_proj;
    glm::mat4 m4_view;
    glm::mat4 m4_world;
    glm::mat4 m4_rotation;
    glm::mat4 m4_translation;
    glm::vec3 v3_cameraPosition;
    glm::vec3 v3_cameraTarget;
    glm::vec3 v3_cameraUp;
    glm::vec3 v3_mouseBegin;
    glm::vec3 v3_mouseEnd;
    Quaternion q_TotalRot;
    int64_t gw_timeStart;
    int64_t gw_timeEnd;
    int64_t gw_timePaused;
    
    uint gw_faces = 0;
    int gw_scrHeight = 0;
    int gw_scrWidth = 0;
    uint gw_movement = 0;
    bool gw_pause = false;
    bool gw_init = false;
    uint updateFlags = 0;
    bool notChecked = true;
    bool updateRequired = false;
};

#endif
