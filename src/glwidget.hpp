/**
 * gwidget.hpp
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

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDateTime>
#include <QFutureWatcher>
#include <QFuture>
#include <QPromise>
#include <QtConcurrent/QtConcurrent>
#include <QtOpenGL/QOpenGLFunctions_4_5_Core>
#include "programGL.hpp"
#include "quaternion.hpp"
#include "wavemanager.hpp"
#include "cloudmanager.hpp"


/* Debug info struct */
struct AtomixInfo {
    float pos = 0.0f;       // Camera position
    float near = 0.0f;      // Near culling distance
    float far = 0.0f;       // Far culling distance
    float start = 0.0f;     // Starting distance
    uint64_t vertex = 0;    // Vertex buffer size
    uint64_t data = 0;      // Data buffer size
    uint64_t index = 0;     // Index buffer size
};
Q_DECLARE_METATYPE(AtomixInfo);

enum egs {
    WAVE_MODE =         1 << 0,     // Button from Wave tab clicked, only making Waves
    WAVE_RENDER =       1 << 1,     // Wave EBO has been loaded
    CLOUD_MODE =        1 << 2,     // Button from Cloud tab clicked, only making Clouds
    CLOUD_RENDER =      1 << 3,     // Cloud EBO has been loaded
    THREAD_FINISHED =   1 << 4,     // init_X_Manager() has finished
    UPD_SHAD_V =        1 << 5,     // Update Vertex Shader
    UPD_SHAD_F =        1 << 6,     // Update Fragment Shader
    UPD_VBO =           1 << 7,     // Cloud VBO needs to be updated
    UPD_DATA =          1 << 8,     // Cloud RDPs need to be loaded into VBO #2
    UPD_EBO =           1 << 9,     // Cloud EBO needs to be updated
    UPD_UNI_COLOUR =    1 << 10,     // [Wave] Colour Uniforms need to be updated
    UPD_UNI_MATHS =     1 << 11,    // [Wave] Maths Uniforms need to be updated
    UPD_MATRICES =      1 << 12,    // Needs initVecsAndMatrices() to reset position and view
    UPDATE_REQUIRED =   1 << 13,    // An update must execute on next render
};

const uint eWaveFlags = egs::WAVE_MODE | egs::WAVE_RENDER;
const uint eCloudFlags = egs::CLOUD_MODE | egs::CLOUD_RENDER;
const uint eModeFlags = egs::WAVE_MODE | egs::CLOUD_MODE;
const uint eUpdateFlags = egs::UPD_SHAD_V | egs::UPD_SHAD_F | egs::UPD_VBO | egs::UPD_DATA | egs::UPD_EBO | egs::UPD_UNI_COLOUR | egs::UPD_UNI_MATHS | egs::UPD_MATRICES | egs::UPDATE_REQUIRED;


class GWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    GWidget(QWidget *parent = nullptr, ConfigParser *configParser = nullptr);
    ~GWidget();

    void setColorsWaves(int id, uint colorChoice);
    void updateBuffersAndShaders();

    void setBGColour(float colour);
    void estimateSize(AtomixConfig *cfg, harmap *cloudMap, uint *vertex, uint *data, uint *index);

signals:
    void detailsChanged(AtomixInfo *info);
    void toggleLoading (bool loading);

public slots:
    void cleanup();
    void newWaveConfig(AtomixConfig *cfg);
    void selectRenderedWaves(int id, bool checked);
    void newCloudConfig(AtomixConfig *cfg, harmap *cloudMap, int numRecipes, bool canCreate = true);

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
    void threadFinished();
    void threadFinishedWithResult(uint result);

    void initVecsAndMatrices();
    void initCrystalProgram();
    void initWaveProgram();
    void initCloudProgram();
    void changeModes(bool force);
    
    void checkErrors(std::string str);
    std::string withCommas(int64_t value);
    void updateSize();
    void printSize();
    void printFlags(std::string);
    void printConfig(AtomixConfig *cfg);

    QOpenGLContext *gw_context = nullptr;
    ProgramGL *crystalProg = nullptr;
    ProgramGL *waveProg = nullptr;
    ProgramGL *cloudProg = nullptr;
    ProgramGL *atomixProg = nullptr; // TODO Consolidate or delete?
    ConfigParser *cfgParser = nullptr;
    WaveManager *waveManager = nullptr;
    CloudManager *cloudManager = nullptr;
    QTimer *gw_timer = nullptr;

    QFutureWatcher<void> *fwModel;
    QFuture<void> futureModel;

    ProgramGL *currentProg = nullptr;
    Manager *currentManager = nullptr;

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
    float gw_bg = 0.0f;
    float gw_nearScale = 0.05f;
    float gw_farScale = 2.20f;
    
    uint gw_faces = 0;
    uint gw_lines = 0;
    int gw_scrHeight = 0;
    int gw_scrWidth = 0;
    uint gw_movement = 0;
    uint gw_vertexCount = 0;
    bool gw_pause = false;
    bool gw_init = false;
    
    QMutex modifyingModel;
    bool isFinal = false;

    int max_n = 1;
    
    BitFlag flWaveCfg;
    BitFlag flGraphState;

    uint crystalRingCount = 0;
    uint crystalRingOffset = 0;
    uint cloudOffset = 0;
};

#endif
