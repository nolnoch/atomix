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

enum ewc { ORBITS = 1, AMPLITUDE = 2, PERIOD = 4, WAVELENGTH = 8, RESOLUTION = 16, PARALLEL = 32, SUPERPOSITION = 64, CPU = 128, SPHERE = 256, VERTSHADER = 512, FRAGSHADER = 1024 };
enum egs {
    WAVE_MODE =             1,          // Button from Wave tab clicked, only making Waves
    WAVE_PROG_INIT =        2,          // Wave program has been initialized (pointer valid)
    WAVE_MANAGER_INIT =     2 << 1,     // Wave Manager has been initialized (pointer valid)
    WAVE_VBO_BOUND =        2 << 2,     // Wave VBO has been loaded
    WAVE_EBO_BOUND =        2 << 3,     // Wave EBO has been loaded
    CLOUD_MODE =            2 << 4,     // Button from Cloud tab clicked, only making Clouds
    CLOUD_PROG_INIT =       2 << 5,     // Cloud program has been initialized (pointer valid)
    CLOUD_MANAGER_INIT =    2 << 6,     // Cloud Manager has been initialized (pointer valid)
    CLOUD_VBO_BOUND =       2 << 7,    // Cloud VBO has been loaded
    CLOUD_RDP_BOUND =       2 << 8,    // Cloud RDPs loaded into VBO #2
    CLOUD_EBO_BOUND =       2 << 9,    // Cloud EBO has been loaded
    UPD_CFG =               2 << 10,    // Cloud AtomixCfg has been changed
    UPD_VBO =               2 << 11,    // Cloud VBO needs to be updated
    UPD_DATA =              2 << 12,    // Cloud RDPs need to be loaded into VBO #2
    UPD_EBO =               2 << 13,    // Cloud EBO needs to be updated
    UPD_UNI_COLOUR =        2 << 14,    // [Wave] Colour Uniforms need to be updated
    UPD_UNI_MATHS =         2 << 15,    // [Wave] Maths Uniforms need to be updated
    UPDATE_REQUIRED =       2 << 16     // An update must execute on next render
};

const uint eWaveFlags = egs::WAVE_MODE | egs::WAVE_PROG_INIT | egs::WAVE_MANAGER_INIT | egs::WAVE_VBO_BOUND | egs::WAVE_EBO_BOUND;
const uint eCloudFlags = egs::CLOUD_MODE | egs::CLOUD_PROG_INIT | egs::CLOUD_MANAGER_INIT | egs::CLOUD_VBO_BOUND | egs::CLOUD_RDP_BOUND | egs::CLOUD_EBO_BOUND;
const uint eModes = egs::WAVE_MODE | egs::CLOUD_MODE;


class GWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    GWidget(QWidget *parent = nullptr, ConfigParser *configParser = nullptr);
    ~GWidget();

    void printConfig(AtomixConfig *cfg);
    void setColorsWaves(int id, uint colorChoice);
    void addCloudRecipes(int n, int l, int m_l);
    void genCloudVertices();
    int genCloudRDPs();
    void genCloudIndices();
    void updateCloudBuffers();

    bool lockCloudConfigAndOrbitals(AtomixConfig *cfg, harmap &cloudMap, int numRecipes);

    float* getCameraPosition();

public slots:
    void cleanup();
    void newWaveConfig(AtomixConfig *cfg);
    void selectRenderedWaves(int id, bool checked);
    void newCloudConfig(AtomixConfig *cfg, harmap &cloudMap, int numRecipes);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);

signals:
    void cameraChanged();

private:
    void initVecsAndMatrices();
    void initAtomixProg();
    void initCrystalProgram();
    void initWaveProgram();
    void initCloudProgram();
    void changeModes();
    void processWaveConfigChange();
    void swapShaders();
    void swapBuffers();
    void swapVertices();
    void swapIndices();
    void checkErrors(std::string str);
    std::string withCommas(int64_t value);
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
    float gw_startDist = 0.0f;
    float gw_farDist = 0.0f;
    float gw_nearDist = 0.0f;
    float gw_cam[3] = {0,0,0};
    
    uint gw_faces = 0;
    uint gw_lines = 0;
    int gw_scrHeight = 0;
    int gw_scrWidth = 0;
    uint gw_movement = 0;
    uint gw_vertexCount = 0;
    bool gw_pause = false;
    bool gw_init = false;

    int max_n = 1;
    
    BitFlag flWaveCfg;
    BitFlag flGraphState;

    uint crystalRingCount;
    uint crystalRingOffset;
};

#endif
