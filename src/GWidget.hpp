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

// #include <QScreen>
// #include <QBoxLayout>
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


/* Debug info struct */
typedef struct AtomixInfo {
    float pos = 0.0f;       // Camera position
    float near = 0.0f;      // Near culling distance
    float far = 0.0f;       // Far culling distance
    uint vertex = 0;        // Vertex buffer size
    uint data = 0;          // Data buffer size
    uint index = 0;         // Index buffer size
} AtomixInfo;
Q_DECLARE_METATYPE(AtomixInfo);

enum egs {
    WAVE_MODE =         1,          // Button from Wave tab clicked, only making Waves
    WAVE_RENDER =       2,          // Wave EBO has been loaded
    CLOUD_MODE =        2 << 1,     // Button from Cloud tab clicked, only making Clouds
    CLOUD_RENDER =      2 << 2,     // Cloud EBO has been loaded
    UPD_SHAD_V =        2 << 4,     // Update Vertex Shader
    UPD_SHAD_F =        2 << 5,     // Update Fragment Shader
    UPD_VBO =           2 << 6,    // Cloud VBO needs to be updated
    UPD_DATA =          2 << 7,    // Cloud RDPs need to be loaded into VBO #2
    UPD_EBO =           2 << 8,    // Cloud EBO needs to be updated
    UPD_UNI_COLOUR =    2 << 9,    // [Wave] Colour Uniforms need to be updated
    UPD_UNI_MATHS =     2 << 10,    // [Wave] Maths Uniforms need to be updated
    UPDATE_REQUIRED =   2 << 11     // An update must execute on next render
};

const uint eWaveFlags = egs::WAVE_MODE | egs::WAVE_RENDER;
const uint eCloudFlags = egs::CLOUD_MODE | egs::CLOUD_RENDER;
const uint eModeFlags = egs::WAVE_MODE | egs::CLOUD_MODE;
const uint eUpdateFlags = egs::UPD_SHAD_V | egs::UPD_SHAD_F | egs::UPD_VBO | egs::UPD_DATA | egs::UPD_EBO | egs::UPD_UNI_COLOUR | egs::UPD_UNI_MATHS;


class GWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    GWidget(QWidget *parent = nullptr, ConfigParser *configParser = nullptr);
    ~GWidget();

    void printConfig(AtomixConfig *cfg);
    void setColorsWaves(int id, uint colorChoice);
    int updateBuffersAndShaders();

    void setBGColour(float colour);
    void cullModel(float pct);

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
    void detailsChanged(AtomixInfo *info);

private:
    void initVecsAndMatrices();
    // void initAtomixProg();
    void initCrystalProgram();
    void initWaveProgram();
    void initCloudProgram();
    void changeModes(bool force);
    
    void checkErrors(std::string str);
    std::string withCommas(int64_t value);
    void updateSize();
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
    AtomixInfo gw_info;
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
    float gw_bg = 0.0f;
    float gw_nearScale = 0.05f;
    float gw_farScale = 2.5f;
    
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

    uint crystalRingCount = 0;
    uint crystalRingOffset = 0;
    uint cloudOffset = 0;
};

#endif
