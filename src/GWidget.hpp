/**
 * GWidget.hpp
 *
 *    Created on: Oct 3, 2023
 *   Last Update: Sep 9, 2024
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *  Copyright 2023, 2024 Wade Burch (GPLv3)
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
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "program.hpp"
#include "quaternion.hpp"
#include "wavemanager.hpp"
#include "cloudmanager.hpp"


/* Program/Wave pointer struct */
/* typedef struct {
    std::vector<std::vector<Program *> *> apProgs;      // Vector of pointers to Programs
    std::vector<std::vector<WaveManager *> *> apWaves;       // Vector of pointers to Waves
    std::vector<WaveConfig *> apConfigs;                // Vector of configs
    int apIdxRender = 0;                                // Current pointer index
    int apIdxCreate = 0;                                // Future pointer index
} AtomixProgs;
Q_DECLARE_METATYPE(AtomixProgs); */

enum flagsConfig {ORBITS = 1, AMPLITUDE = 2, PERIOD = 4, WAVELENGTH = 8, RESOLUTION = 16, PARALLEL = 32, SUPERPOSITION = 64, CPU = 128, SPHERE = 256, VERTSHADER = 512, FRAGSHADER = 1024};


class GWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    GWidget(QWidget *parent = nullptr, ConfigParser *configParser = nullptr);
    ~GWidget();

    void printConfig(AtomixConfig *cfg);
    void setColorsWaves(int id, uint colorChoice);

public slots:
    void cleanup();
    void newWaveConfig(AtomixConfig *cfg);
    void selectRenderedWaves(int id, bool checked);
    void newCloudConfig();

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
    int initAtomixProg();
    int initCrystalProgram();
    int initWaveProgram();
    int initCloudProgram();
    void changeModes();
    void processConfigChange();
    void swapShaders();
    void swapBuffers();
    void swapVertices();
    void swapIndices();
    void checkErrors(std::string str);
    void printSize();

    QOpenGLContext *gw_context = nullptr;
    Program *crystalProg = nullptr;
    Program *waveProg = nullptr;
    Program *cloudProg = nullptr;
    Program *atomixProg = nullptr; // TODO Consolidate or delete?
    ConfigParser *cfgParser = nullptr;
    WaveManager *waveManager = nullptr;
    CloudManager *cloudManager = nullptr;
    QTimer *gw_timer = nullptr;

    Program *currentProg = nullptr;
    Manager *currentManager = nullptr;

    AtomixConfig renderConfig;
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
    ushort renderedWaves = 0;
    float gw_startDist = 0.0f;
    float gw_farDist = 0.0f;
    float gw_nearDist = 0.0f;
    
    uint gw_faces = 0;
    uint gw_lines = 0;
    int gw_scrHeight = 0;
    int gw_scrWidth = 0;
    uint gw_movement = 0;
    bool gw_pause = false;
    bool gw_init = false;
    
    uint updateFlags = 0;
    // uint static_dynamic = 0;
    
    bool notChecked = true;
    bool newConfig = false;
    bool newVertices = false;
    bool newIndices = false;
    bool newUniformsMaths = false;
    bool newUniformsColor = false;
    bool newCloud = false;
    bool updateRequired = false;

    bool renderCloud = false;
    bool renderWave = false;
    bool waveMode = false;
    bool cloudMode = false;

    fvec crystalRingVertices;
    uvec crystalRingIndices;
    uint crystalRingCount;
    uint crystalRingOffset;
};

#endif
