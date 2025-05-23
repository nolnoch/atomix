/**
 * vkwindow.hpp
 *
 *    Created on: Oct 22, 2024
 *   Last Update: Dec 29, 2024
 *  Orig. Author: Wade Burch (dev@nolnoch.com)
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

#ifndef VKWINDOW_H
#define VKWINDOW_H

#include <QVulkanWindow>
#include <QVulkanWindowRenderer>
#include <QVulkanInstance>
#include <QVulkanFunctions>
#include <QWindow>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDateTime>
#include <QFutureWatcher>
#include <QFuture>
#include <QPromise>
#include <QtConcurrent/QtConcurrent>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <array>
#include <algorithm>
#include <ranges>
#include "programVK.hpp"
#include "quaternion.hpp"
#include "wavemanager.hpp"
#include "cloudmanager.hpp"


/* Debug info struct */
struct AtomixInfo {
    float pos = 0.0f;       // Camera position
    float near = 0.0f;      // Near culling distance
    float far = 0.0f;       // Far culling distance
    float start = 0.0f;     // Starting distance
    float aspect = 0.0f;    // Aspect ratio
    uint64_t vertex = 0;    // Vertex buffer size
    uint64_t data = 0;      // Data buffer size
    uint64_t index = 0;     // Index buffer size
};
Q_DECLARE_METATYPE(AtomixInfo);

struct WorldState {
    glm::mat4 m4_world;
    glm::mat4 m4_view;
    glm::mat4 m4_proj;
};
Q_DECLARE_METATYPE(WorldState);

struct WaveState {
    alignas(16) glm::vec3 waveMaths = glm::vec3(0.0f);
    alignas(16) glm::uvec3 waveColours = glm::uvec3(0);
};
Q_DECLARE_METATYPE(WaveState);

struct PushConstantsWave {
    float time = 0.0f;
    uint mode = 0;
};

struct PushConstantsCloud {
    float maxRadius = 0.0f;
};

enum egs {
    WAVE_MODE =         1 << 0,     // Button from Wave tab clicked, only making Waves
    WAVE_RENDER =       1 << 1,     // Wave IBO has been loaded
    CLOUD_MODE =        1 << 2,     // Button from Cloud tab clicked, only making Clouds
    CLOUD_RENDER =      1 << 3,     // Cloud IBO has been loaded
    THREAD_FINISHED =   1 << 4,     // init_X_Manager() has finished
    UPD_SHAD_V =        1 << 5,     // Update Vertex Shader
    UPD_SHAD_F =        1 << 6,     // Update Fragment Shader
    UPD_VBO =           1 << 7,     // Cloud VBO needs to be updated
    UPD_DATA =          1 << 8,     // Cloud RDPs need to be loaded into VBO #2
    UPD_IBO =           1 << 9,     // Cloud IBO needs to be updated
    UPD_IDXOFF =        1 << 10,    // Cloud index offset needs to be updated
    UPD_UNI_COLOUR =    1 << 11,    // [Wave] Colour Uniforms need to be updated
    UPD_UNI_MATHS =     1 << 12,    // [Wave] Maths Uniforms need to be updated
    UPD_PUSH_CONST =    1 << 13,    // Push Constants need to be updated
    UPD_MATRICES =      1 << 14,    // Needs initVecsAndMatrices() to reset position and view
    CPU_RENDER =        1 << 15,    // Render on CPU
    UPDATE_REQUIRED =   1 << 16,    // An update must execute on next render
};

const uint eWaveFlags = egs::WAVE_MODE | egs::WAVE_RENDER;
const uint eCloudFlags = egs::CLOUD_MODE | egs::CLOUD_RENDER;
const uint eModeFlags = egs::WAVE_MODE | egs::CLOUD_MODE;
const uint eRenderFlags = egs::WAVE_RENDER | egs::CLOUD_RENDER;
const uint eUpdateFlags = uint(-1) << 4;

class VKWindow;


class VKRenderer : public QVulkanWindowRenderer {
public:
    VKRenderer(QVulkanWindow *vkWin);
    ~VKRenderer();

    void setProgram(ProgramVK *prog) { atomixProg = prog; }
    void setWindow(VKWindow *win) { vr_vkw = win; }

    void preInitResources() override;
    void initResources() override;
    void initSwapChainResources() override;
    void logicalDeviceLost() override;
    void physicalDeviceLost() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;
    void startNextFrame() override;

private:
    ProgramVK *atomixProg = nullptr;

    QVulkanInstance *vr_vi = nullptr;
    QVulkanFunctions *vr_vf = nullptr;
    QVulkanDeviceFunctions *vr_vdf = nullptr;
    QVulkanWindow *vr_qvw = nullptr;

    VKWindow *vr_vkw = nullptr;
    VkDevice vr_dev;
    VkPhysicalDevice vr_phydev;

    VkExtent2D vr_extent = {0, 0};
    VkDeviceSize vr_minUniAlignment = 0;

    bool vr_isInit = false;
};


class VKWindow : public QVulkanWindow {
    Q_OBJECT
public:
    VKWindow(QWidget *parent = nullptr, FileHandler *configParser = nullptr);
    ~VKWindow();
    
    QVulkanWindowRenderer* createRenderer() override;
    bool initProgram(AtomixDevice *dev);
    void initWindow();
    void releaseWindow();

    void handleHome();
    void handlePause();
    void setColorsWaves(int id, uint colorChoice);
    void updateExtent(VkExtent2D &renderExtent);
    void updateBuffersAndShaders();
    void setBGColour(float colour);
    void estimateSize(AtomixCloudConfig *cfg, harmap *cloudMap, uint *vertex, uint *data, uint *index);

    void resetHang();

    VkSurfaceKHR vw_surface = VK_NULL_HANDLE;
    BitFlag flGraphState;

signals:
    void detailsChanged(AtomixInfo *info);
    void toggleLoading (bool loading);
    void forwardKeyEvent(QKeyEvent *e);

public slots:
    void newCloudConfig(AtomixCloudConfig *cfg, harmap *cloudMap, bool canCreate = true);
    void newWaveConfig(AtomixWaveConfig *cfg);
    void selectRenderedWaves(int id, bool checked);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void cleanup();
    void changeModes(bool force);
 
    void initCrystalModel();
    void initWaveModel();
    void initCloudModel();
    void initModels();
    void initVecsAndMatrices();

    void threadFinished();
    void threadFinishedWithResult(uint result);
    
    std::string withCommas(int64_t value);
    void updateBufferSizes();
    void printSize();
    void printFlags(std::string);

    FileHandler *fileHandler = nullptr;
    QWidget *vw_parent = nullptr;
    VKRenderer *vw_renderer = nullptr;
    std::string vw_currentModel = "";
    std::string vw_previousModel = "";

    AtomixInfo vw_info;
    QOpenGLContext *vw_context = nullptr;
    ProgramVK *atomixProg = nullptr;
    Manager *currentManager = nullptr;
    WaveManager *waveManager = nullptr;
    CloudManager *cloudManager = nullptr;
    QTimer *vw_timer = nullptr;

    QFutureWatcher<void> *fwModel;
    QFuture<void> futureModel;

    glm::mat4 m4_rotation;
    glm::mat4 m4_translation;
    glm::vec3 v3_cameraPosition;
    glm::vec3 v3_cameraTarget;
    glm::vec3 v3_cameraUp;
    glm::vec3 v3_mouseBegin;
    glm::vec3 v3_mouseEnd;
    Quaternion q_TotalRot;
    
    int64_t vw_timeStart;
    int64_t vw_timeEnd;
    int64_t vw_timePaused;
    float vw_bg = 0.0f;
    
    VkExtent2D vw_extent = {0, 0};
    uint vw_movement = 0;
    uint vw_vertexCount = 0;
    bool vw_pause = false;
    bool vw_init = false;
    bool vw_tracking = false;
    
    QMutex modifyingModel;

    int max_n = 1;
    
    BitFlag flWaveCfg;

    uint crystalRingCount = 0;
    uint crystalRingOffset = 0;
    uint cloudOffset = 0;

    WorldState vw_world;
    WaveState vw_wave;
    PushConstantsWave pConstWave = {0.0f, 0};
    PushConstantsCloud pConstCloud = {0.0f};

    uint savedState = 0;
};

#endif
