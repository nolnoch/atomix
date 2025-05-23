/**
 * vkwindow.cpp
 *
 *    Created on: Oct 22, 2024
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
#include <ranges>

#include <QTimer>
#include <QObject>

#include "vkwindow.hpp"

#define RADN(t) (glm::radians((t)))


VKWindow::VKWindow(QWidget *parent, FileHandler *fileHandler)
    : fileHandler(fileHandler), vw_parent(parent) {
    this->setSurfaceType(QVulkanWindow::VulkanSurface);
    this->setDeviceExtensions({ "VK_KHR_portability_subset" });
    this->setFlags(QVulkanWindow::PersistentResources);
}

VKWindow::~VKWindow() {
    cleanup();
}

void VKWindow::releaseWindow() {
    delete atomixProg;
    this->atomixProg = nullptr;

    this->savedState = flGraphState.intersection(eModeFlags);
    flGraphState.reset();
}

void VKWindow::cleanup() {
    if (cloudManager || waveManager) {
        fwModel->waitForFinished();
    }
    changeModes(true);
    delete atomixProg;
    delete vw_timer;
}

void VKWindow::changeModes(bool force) {
    if (!waveManager || force) {
        delete cloudManager;
        cloudManager = 0;
        flGraphState.clear(eCloudFlags);
    } else if (!cloudManager || force) {
        delete waveManager;
        waveManager = 0;
        flGraphState.clear(eWaveFlags);
    }
    currentManager = 0;
}

QVulkanWindowRenderer* VKWindow::createRenderer() {
    vw_renderer = new VKRenderer(this);
    vw_renderer->setWindow(this);
    
    return vw_renderer;
}

bool VKWindow::initProgram(AtomixDevice *atomixDevice) {
    bool firstInit = false;

    if (!atomixProg) {
        atomixProg = new ProgramVK(fileHandler);
        atomixProg->setInstance(atomixDevice);
        std::vector<std::string> vshad = atomix::stringlistToVector(fileHandler->getVertexShadersList());
        std::vector<std::string> fshad = atomix::stringlistToVector(fileHandler->getFragmentShadersList());
        atomixProg->addAllShaders(&vshad, GL_VERTEX_SHADER);
        atomixProg->addAllShaders(&fshad, GL_FRAGMENT_SHADER);
        atomixProg->init();
        vw_renderer->setProgram(atomixProg);
        firstInit = true;
    } else {
        atomixProg->setInstance(atomixDevice);
    }

    return firstInit;
}

void VKWindow::initWindow() {
    // Init -- Matrices
    initVecsAndMatrices();

    // Init -- Models
    initModels();
    this->atomixProg->activateModel("crystal");
    this->atomixProg->addModelProgram("crystal");
    this->flGraphState.set(this->savedState);

    // Init -- Time
    vw_timeStart = QDateTime::currentMSecsSinceEpoch();

    // Init -- Threading
    fwModel = new QFutureWatcher<void>;
    connect(fwModel, &QFutureWatcher<void>::finished, this, &VKWindow::threadFinished);

    this->vw_init = true;
}

void VKWindow::newCloudConfig(AtomixCloudConfig *config, harmap *cloudMap, bool generator) {
    flGraphState.set(egs::CLOUD_MODE);
    if (flGraphState.hasAny(eWaveFlags)) {
        changeModes(false);
    }

    if (!cloudManager) {
        cloudManager = new CloudManager();
        currentManager = cloudManager;
    }

    futureModel = QtConcurrent::run(&CloudManager::receiveCloudMapAndConfig, cloudManager, config, cloudMap, generator);
    fwModel->setFuture(futureModel);
    this->max_n = cloudMap->rbegin()->first;
    int divSciExp = std::abs(floor(log10(config->cloudTolerance)));
    this->pConstCloud.maxRadius = cm_maxRadius[divSciExp - 1][max_n - 1];
    emit toggleLoading(true);
}

void VKWindow::newWaveConfig(AtomixWaveConfig *config) {
    flGraphState.set(egs::WAVE_MODE);
    if (flGraphState.hasAny(eCloudFlags)) {
        changeModes(false);
    }

    if (!waveManager) {
        waveManager = new WaveManager();
        currentManager = waveManager;
    }

    waveManager->setTime(this->pConstWave.time);
    futureModel = QtConcurrent::run(&WaveManager::receiveConfig, waveManager, config);
    fwModel->setFuture(futureModel);
    emit toggleLoading(true);
}

void VKWindow::selectRenderedWaves(int id, bool checked) {
    // Process and flag for IBO update
    futureModel = QtConcurrent::run(&WaveManager::selectWaves, waveManager, id, checked);
    fwModel->setFuture(futureModel);
    emit toggleLoading(true);
}

void VKWindow::initCrystalModel() {
    fvec crystalRingVertices;
    uvec crystalRingIndices;

    // Crystal Diamond setup
    float zero, peak, edge, back, forX, forZ, root;
    edge = 0.3f;  // <-- Change this to scale diamond
    peak = edge;
    zero = 0.0f;
    root = sqrt(3);
    back = root / 3 * edge;
    forZ = root / 6 * edge;
    forX = edge / 2;

    const std::array<VKfloat, 30> vertices = {
              //Vertex              //Colour
          zero,  peak,  zero,   0.6f, 0.6f, 0.6f,    // 0 top
         -forX,  zero,  forZ,   0.1f, 0.4f, 0.4f,    // 1 left - cyan
          forX,  zero,  forZ,   0.4f, 0.1f, 0.4f,    // 2 right - magenta
          zero,  zero, -back,   0.4f, 0.4f, 0.1f,    // 3 back - yellow
          zero, -peak,  zero,   0.0f, 0.0f, 0.0f     // 4 bottom
    };

    const std::array<VKuint, 18> indices = {
        1, 0, 3,
        3, 0, 2,
        2, 0, 1,
        1, 4, 2,
        2, 4, 3,
        3, 4, 1
    };
    uint vw_faces = indices.size();

    // Crystal Ring setup
    int crystalRes = 80;
    double crystalDegFac = PI_TWO / crystalRes;
    double crystalRadius = 0.4f;
    uint32_t vs = vertices.size() / 6;

    // Copy Diamond into vector, then append Ring
    std::copy(vertices.cbegin(), vertices.cend(), std::back_inserter(crystalRingVertices));
    std::copy(indices.cbegin(), indices.cend(), std::back_inserter(crystalRingIndices));

    for (int i = 0; i < crystalRes; i++) {
        double cos_t = cos(i * crystalDegFac);
        double sin_t = sin(i * crystalDegFac);
        crystalRingVertices.push_back(static_cast<float>(crystalRadius * cos_t));
        crystalRingVertices.push_back(0.0f);
        crystalRingVertices.push_back(static_cast<float>(crystalRadius * sin_t));
        crystalRingVertices.push_back(0.85f);
        crystalRingVertices.push_back(0.85f);
        crystalRingVertices.push_back(0.85f);
        crystalRingIndices.push_back(vs + i);
    }
    
    // Close the ring because Vulkan does not support GL_LINE_LOOP
    uint i = 0;
    double cos_t = cos(i * crystalDegFac);
    double sin_t = sin(i * crystalDegFac);
    crystalRingVertices.push_back(static_cast<float>(crystalRadius * cos_t));
    crystalRingVertices.push_back(0.0f);
    crystalRingVertices.push_back(static_cast<float>(crystalRadius * sin_t));
    crystalRingVertices.push_back(0.85f);
    crystalRingVertices.push_back(0.85f);
    crystalRingVertices.push_back(0.85f);
    crystalRingIndices.push_back(vs + crystalRes);
    this->crystalRingCount = uint(crystalRingIndices.size()) - vw_faces;
    this->crystalRingOffset = vw_faces;
    
    // Define VBO for Crystal Diamond & Ring
    BufferCreateInfo crystalVert{};
    crystalVert.binding = 0;
    crystalVert.name = "crystalVertices";
    crystalVert.type = BufferType::VERTEX;
    crystalVert.count = crystalRingVertices.size() / 6;
    crystalVert.size = crystalRingVertices.size() * sizeof(float);
    crystalVert.data = &crystalRingVertices[0];
    crystalVert.dataTypes = { DataType::FLOAT_VEC3, DataType::FLOAT_VEC3 };

    // Define IBO for Crystal Diamond & Ring
    BufferCreateInfo crystalInd{};
    crystalInd.name = "crystalIndices";
    crystalInd.type = BufferType::INDEX;
    crystalInd.count = crystalRingIndices.size();
    crystalInd.size = crystalRingIndices.size() * sizeof(uint32_t);
    crystalInd.data = &crystalRingIndices[0];
    crystalInd.dataTypes = {DataType::UINT};

    // Define Crystal Model with above buffers
    ModelCreateInfo crystalModel{};
    crystalModel.name = "crystal";
    crystalModel.vbos = { &crystalVert };
    crystalModel.ibo = &crystalInd;
    crystalModel.ubos = { "WorldState" };
    crystalModel.vertShaders = { "default.vert" };
    crystalModel.fragShaders = { "default.frag" };
    crystalModel.topologies = { VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP };
    crystalModel.bufferCombos = { { 0 } };
    crystalModel.offsets = {
        {   .offset = 0,
            .vertShaderIndex = 0,
            .fragShaderIndex = 0,
            .topologyIndex = 0,
            .bufferComboIndex = 0
        },
        {   .offset = crystalRingOffset,
            .vertShaderIndex = 0,
            .fragShaderIndex = 0,
            .topologyIndex = 1,
            .bufferComboIndex = 0
        }
    };
    crystalModel.programs = {
        { .name = "default",
          .offsets = { 0, 1 }
        }
    };

    // Add Crystal Model to program
    atomixProg->addModel(crystalModel);
}

void VKWindow::initWaveModel() {
    // Define VBO:GPU for Atomix Wave
    BufferCreateInfo waveVert{};
    waveVert.binding = 0;
    waveVert.name = "waveVertices";
    waveVert.type = BufferType::VERTEX;
    waveVert.dataTypes = { DataType::FLOAT_VEC4 };

    // Define VBO:CPU for Atomix Wave
    BufferCreateInfo waveVertCPU{};
    waveVertCPU.binding = 0;
    waveVertCPU.name = "waveVerticesCPU";
    waveVertCPU.type = BufferType::VERTEX;
    waveVertCPU.dataTypes = { DataType::FLOAT_VEC4, DataType::FLOAT_VEC4 };

    // Define IBO for Atomix Wave
    BufferCreateInfo waveInd{};
    waveInd.name = "waveIndices";
    waveInd.type = BufferType::INDEX;
    waveInd.dataTypes = { DataType::UINT };

    // Iff we are here from a mid-run resource reset, we can use the existing data
    if (waveManager) {
        if (waveManager->isCPU()) {
            waveVertCPU.count = waveManager->getVertexCount();
            waveVertCPU.size = waveManager->getVertexSize();
            waveVertCPU.data = waveManager->getVertexData();
        } else {
            waveVert.count = waveManager->getVertexCount();
            waveVert.size = waveManager->getVertexSize();
            waveVert.data = waveManager->getVertexData();
        }
        waveInd.count = waveManager->getIndexCount();
        waveInd.size = waveManager->getIndexSize();
        waveInd.data = waveManager->getIndexData();
    }

    // Define Atomix Wave Model with above buffers
    ModelCreateInfo waveModel{};
    waveModel.name = "wave";
    waveModel.vbos = { &waveVert, &waveVertCPU };
    waveModel.ibo = &waveInd;
    waveModel.ubos = { "WorldState", "WaveState" };
    waveModel.vertShaders = { "gpu_circle.vert", "default.vert", "gpu_sphere.vert" };
    waveModel.fragShaders = { "default.frag" };
    waveModel.pushConstant = "pConstWave";
    waveModel.bufferCombos = { { 0 }, { 1 } };
    waveModel.topologies = { VK_PRIMITIVE_TOPOLOGY_POINT_LIST };
    waveModel.offsets = {
        {   .offset = 0,
            .vertShaderIndex = 0,
            .fragShaderIndex = 0,
            .topologyIndex = 0,
            .bufferComboIndex = 0,
            .pushConstantIndex = 0
        },
        {   .offset = 0,
            .vertShaderIndex = 1,
            .fragShaderIndex = 0,
            .topologyIndex = 0,
            .bufferComboIndex = 1,
            .pushConstantIndex = 0
        },
        {   .offset = 0,
            .vertShaderIndex = 2,
            .fragShaderIndex = 0,
            .topologyIndex = 0,
            .bufferComboIndex = 0,
            .pushConstantIndex = 0
        }
    };
    waveModel.programs = {
        { .name = "default",
          .offsets = { 0 }
        },
        { .name = "cpu",
          .offsets = { 1 }
        },
        { .name = "sphere",
          .offsets = { 2 }
        }
    };

    // Add Atomix Wave Model to program
    atomixProg->addModel(waveModel);

    // Assign push constants data
    atomixProg->updatePushConstant("pConstWave", &this->pConstWave);
}

void VKWindow::initCloudModel() {
    // Define VBO:Vertex:GPU for Atomix Cloud
    BufferCreateInfo cloudVert{};
    cloudVert.binding = 0;
    cloudVert.type = BufferType::VERTEX;
    cloudVert.name = "cloudVertices";
    cloudVert.dataTypes = { DataType::FLOAT_VEC4 };

    // Define VBO:Vertex:CPU for Atomix Cloud
    BufferCreateInfo cloudVertCPU{};
    cloudVertCPU.binding = 0;
    cloudVertCPU.type = BufferType::VERTEX;
    cloudVertCPU.name = "cloudVerticesCPU";
    cloudVertCPU.dataTypes = { DataType::FLOAT_VEC4 };

    // Define VBO:Data:GPU for Atomix Cloud
    BufferCreateInfo cloudData{};
    cloudData.binding = 1;
    cloudData.type = BufferType::DATA;
    cloudData.name = "cloudData";
    cloudData.dataTypes = { DataType::FLOAT };

    // Define VBO:Data:CPU for Atomix Cloud
    BufferCreateInfo cloudDataCPU{};
    cloudDataCPU.binding = 1;
    cloudDataCPU.type = BufferType::DATA;
    cloudDataCPU.name = "cloudDataCPU";
    cloudDataCPU.dataTypes = { DataType::FLOAT_VEC4 };

    // Define IBO for Atomix Cloud
    BufferCreateInfo cloudInd{};
    cloudInd.type = BufferType::INDEX;
    cloudInd.name = "cloudIndices";
    cloudInd.dataTypes = { DataType::UINT };

    // Iff we are here from a mid-run resource reset, we can use the existing data
    if (cloudManager) {
        if (cloudManager->isCPU()) {
            cloudVertCPU.count = cloudManager->getVertexCount();
            cloudVertCPU.size = cloudManager->getVertexSize();
            cloudVertCPU.data = cloudManager->getVertexData();
            cloudDataCPU.count = cloudManager->getDataCount();
            cloudDataCPU.size = cloudManager->getDataSize();
            cloudDataCPU.data = cloudManager->getDataData();
        } else {
            cloudVert.count = cloudManager->getVertexCount();
            cloudVert.size = cloudManager->getVertexSize();
            cloudVert.data = cloudManager->getVertexData();
            cloudData.count = cloudManager->getDataCount();
            cloudData.size = cloudManager->getDataSize();
            cloudData.data = cloudManager->getDataData();
        }
        cloudInd.count = cloudManager->getIndexCount();
        cloudInd.size = cloudManager->getIndexSize();
        cloudInd.data = cloudManager->getIndexData();
    }

    // Define Atomix Cloud Model with above buffers
    ModelCreateInfo cloudModel{};
    cloudModel.name = "cloud";
    cloudModel.vbos = { &cloudVert, &cloudVertCPU, &cloudData, &cloudDataCPU };
    cloudModel.ibo = &cloudInd;
    cloudModel.ubos = { "WorldState" };
    cloudModel.vertShaders = { "gpu_harmonics.vert", "default.vert" };
    cloudModel.fragShaders = { "default.frag" };
    cloudModel.pushConstant = "pConstCloud";
    cloudModel.topologies = { VK_PRIMITIVE_TOPOLOGY_POINT_LIST };
    cloudModel.bufferCombos = { { 0, 2 }, { 1, 3 } };
    cloudModel.offsets = {
        {   .offset = 0,
            .vertShaderIndex = 0,
            .fragShaderIndex = 0,
            .topologyIndex = 0,
            .bufferComboIndex = 0,
            .pushConstantIndex = 0
        },
        {
            .offset = 0,
            .vertShaderIndex = 1,
            .fragShaderIndex = 0,
            .topologyIndex = 0,
            .bufferComboIndex = 1,
            .pushConstantIndex = -1
        }
    };
    cloudModel.programs = {
        { .name = "default",
          .offsets = { 0 }
        },
        { .name = "cpu",
          .offsets = { 1 }
        }
    };

    // Add Atomix Cloud Model to program
    atomixProg->addModel(cloudModel);

    // Assign push constants data
    atomixProg->updatePushConstant("pConstCloud", &this->pConstCloud);
}

void VKWindow::initModels() {
    initCrystalModel();
    initWaveModel();
    initCloudModel();
}

void VKWindow::initVecsAndMatrices() {
    vw_info.start = (flGraphState.hasNone(egs::CLOUD_MODE)) ? 16.0f : (10.0f + 6.0f * (this->max_n * this->max_n));
    vw_info.near = 0.1f;
    vw_info.far = vw_info.start * 2.0f;

    q_TotalRot.zero();
    m4_rotation = glm::mat4(1.0f);
    m4_translation = glm::mat4(1.0f);
    vw_world.m4_proj = glm::mat4(1.0f);
    vw_world.m4_view = glm::mat4(1.0f);
    vw_world.m4_world = glm::mat4(1.0f);

    v3_cameraPosition = glm::vec3(0.0f, 0.0f, vw_info.start);
    v3_cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    v3_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    v3_mouseBegin = glm::vec3(0);
    v3_mouseEnd = glm::vec3(0);

    vw_world.m4_view = glm::lookAt(v3_cameraPosition, v3_cameraTarget, v3_cameraUp);
    
    float width, height;
    if (vw_init) {
        width = static_cast<float>(vw_extent.width);
        height = static_cast<float>(vw_extent.height);
    } else {
        width = this->width();
        height = this->height();
    }
    vw_info.aspect = width / height;
    vw_world.m4_proj = glm::perspective(RADN(45.0f), vw_info.aspect, vw_info.near, vw_info.far);
    vw_world.m4_proj[1][1] *= -1.0f;

    atomixProg->updateClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    vw_info.pos = vw_info.start;
    emit detailsChanged(&vw_info);
}

void VKWindow::wheelEvent(QWheelEvent *e) {
    int scrollClicks = e->angleDelta().y() / -120;
    if (!scrollClicks) {
        return;
    }
    float scrollScale = 1.0f + ((float) scrollClicks / 6);
    v3_cameraPosition = scrollScale * v3_cameraPosition;
    
    vw_info.pos = v3_cameraPosition.z;
    vw_info.far = v3_cameraPosition.z + vw_info.start;
    emit detailsChanged(&vw_info);
    requestUpdate();
}

void VKWindow::mousePressEvent(QMouseEvent *e) {
    glm::vec3 mouseVec = glm::vec3(e->pos().x(), vw_extent.height - e->pos().y(), v3_cameraPosition.z);
    v3_mouseBegin = mouseVec;
    v3_mouseEnd = mouseVec;

    if (!vw_movement && (e->button() & (Qt::LeftButton | Qt::RightButton | Qt::MiddleButton))) {
        this->vw_movement |= e->button();
        // this->vw_tracking = true;
    } else {
        QWindow::mousePressEvent(e);
    }
}

void VKWindow::mouseMoveEvent(QMouseEvent *e) {
    if (!this->vw_movement) {
        return;
    }
    glm::vec3 mouseVec = glm::vec3(e->pos().x(), vw_extent.height - e->pos().y(), v3_cameraPosition.z);
    glm::vec3 cameraVec = v3_cameraPosition - v3_cameraTarget;
    v3_mouseBegin = v3_mouseEnd;
    v3_mouseEnd = mouseVec;

    //VKfloat currentAngle = atanf(cameraVec.y / hypot(cameraVec.x, cameraVec.z));

    if (vw_movement & Qt::RightButton) {
        /* Right-click-drag HORIZONTAL movement will rotate about Y axis */
        if (v3_mouseBegin.x != v3_mouseEnd.x) {
            float dragRatio = (v3_mouseEnd.x - v3_mouseBegin.x) / vw_extent.width;
            VKfloat waveAngleH = TWO_PI * dragRatio;
            glm::vec3 waveAxisH = glm::vec3(0.0, 1.0f, 0.0);
            Quaternion qWaveRotH = Quaternion(waveAngleH, waveAxisH, RAD);
            q_TotalRot = qWaveRotH * q_TotalRot;
        }
        /* Right-click-drag VERTICAL movement will rotate about X and Z axes */
        if (v3_mouseBegin.y != v3_mouseEnd.y) {
            float dragRatio = (v3_mouseBegin.y - v3_mouseEnd.y) / vw_extent.height;
            VKfloat waveAngleV = TWO_PI * dragRatio;
            glm::vec3 cameraUnit = glm::normalize(glm::vec3(cameraVec.x, 0.0f, cameraVec.z));
            glm::vec3 waveAxisV = glm::vec3(cameraUnit.z, 0.0f, -cameraUnit.x);
            Quaternion qWaveRotV = Quaternion(waveAngleV, waveAxisV, RAD);
            q_TotalRot = qWaveRotV * q_TotalRot;
        }
    } else if (vw_movement & Qt::LeftButton) {
        /* Left-click drag will grab and slide world */
        if (v3_mouseBegin != v3_mouseEnd) {
            glm::vec3 deltaSlide = 0.02f * (v3_mouseEnd - v3_mouseBegin);
            glm::vec3 cameraSlide = (cameraVec.z / 25.0f) * glm::vec3(deltaSlide.x, deltaSlide.y, 0.0f);
            m4_translation = glm::translate(m4_translation, cameraSlide);
        }

    } else if (vw_movement & Qt::MiddleButton) {
        /* Middle-click-drag will rotate about camera look vector */
        if (v3_mouseBegin.x != v3_mouseEnd.x) {
            float dragRatio = (v3_mouseBegin.x - v3_mouseEnd.x) / vw_extent.width;
            VKfloat waveAngleL = TWO_PI * dragRatio;
            glm::vec3 waveAxisL = glm::normalize(cameraVec);
            Quaternion qWaveRotL = Quaternion(waveAngleL, waveAxisL, RAD);
            q_TotalRot = qWaveRotL * q_TotalRot;
        }
    }
    requestUpdate();
}

void VKWindow::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() & (Qt::RightButton | Qt::LeftButton | Qt::MiddleButton)) {
        this->vw_movement = 0;
        // this->vw_tracking = false;
    } else {
        QWindow::mouseReleaseEvent(e);
    }
}

void VKWindow::keyPressEvent(QKeyEvent *e) {
    QCoreApplication::sendEvent(this->parent(), e);
}

void VKWindow::resizeEvent(QResizeEvent *e) {
    QWindow::resizeEvent(e);
    
    // Update matrices
    // VkExtent2D renderExtent = { (uint32_t)e->size().width(), (uint32_t)e->size().height() };
    // this->updateExtent(renderExtent);
}

void VKWindow::handleHome() {
    initVecsAndMatrices();
    requestUpdate();
}

void VKWindow::handlePause() {
    vw_pause = !vw_pause;
    if (vw_pause) {
        vw_timePaused = QDateTime::currentMSecsSinceEpoch();
    } else {
        vw_timeEnd = QDateTime::currentMSecsSinceEpoch();
        vw_timeStart += vw_timeEnd - vw_timePaused;
    }
    requestUpdate();
}

void VKWindow::setColorsWaves(int id, uint colorChoice) {
    switch (id) {
    case 1:
        waveManager->setPeak(colorChoice);
        break;
    case 2:
        waveManager->setBase(colorChoice);
        break;
    case 3:
        waveManager->setTrough(colorChoice);
        break;
    default:
        break;
    }
    flGraphState.set(egs::UPD_UNI_COLOUR | egs::UPDATE_REQUIRED);
}

void VKWindow::updateExtent(VkExtent2D &renderExtent) {
    vw_extent = renderExtent;
    vw_info.aspect = (float)renderExtent.width / (float)renderExtent.height;
}

void VKWindow::updateBuffersAndShaders() {
    bool threadsFinished = fwModel->isFinished();

    // Re-calculate world-state matrices (per-frame)
    m4_rotation = glm::make_mat4(&q_TotalRot.matrix()[0]);
    vw_world.m4_world = m4_translation * m4_rotation;
    vw_world.m4_view = glm::lookAt(v3_cameraPosition, v3_cameraTarget, v3_cameraUp);
    vw_world.m4_proj = glm::perspective(RADN(45.0f), vw_info.aspect, vw_info.near, vw_info.far);
    vw_world.m4_proj[1][1] *= -1.0f;
    this->q_TotalRot.normalize();

    // Update time (per-frame)
    if (!vw_pause) {
        vw_timeEnd = QDateTime::currentMSecsSinceEpoch();
    }
    pConstWave.time = (vw_timeEnd - vw_timeStart) * 0.001f;
    if ((currentManager) && threadsFinished) {
        currentManager->update(pConstWave.time);
        this->flGraphState.set(egs::UPDATE_REQUIRED);
    }
    
    if (this->flGraphState.hasAny(egs::UPDATE_REQUIRED) && threadsFinished) {
        // Capture updates from currentManager
        this->flGraphState.set(currentManager->clearUpdates());

        // Set current model
        vw_previousModel = vw_currentModel;
        if (flGraphState.hasAny(egs::WAVE_MODE)) {
            vw_currentModel = "wave";
        } else if (flGraphState.hasAny(egs::CLOUD_MODE)) {
            vw_currentModel = "cloud";
        }
        
        // Changing shaders is equivalent to changing model program
        if (flGraphState.hasAny(egs::UPD_SHAD_V | egs::UPD_SHAD_F)) {
            // std::set<VKuint> activePrograms = this->atomixProg->getModelActivePrograms(vw_currentModel);
            std::string newProgram;

            if (flGraphState.hasAny(egs::CPU_RENDER)) {
                newProgram = "cpu";
            } else if (flGraphState.hasAny(egs::WAVE_MODE) && waveManager->getSphere()) {
                newProgram = "sphere";
            } else {
                newProgram = "default";
            }

            this->atomixProg->clearModelPrograms(vw_currentModel);
            this->atomixProg->addModelProgram(vw_currentModel, newProgram);
        }
        BufferUpdateInfo updBuf{};
        updBuf.modelName = vw_currentModel;

        // Update VBO 1: Vertices
        if (flGraphState.hasAny(egs::UPD_VBO)) {
            std::string bufferCPU = (flGraphState.hasAny(egs::CPU_RENDER)) ? "VerticesCPU" : "Vertices";
            updBuf.bufferName = vw_currentModel + bufferCPU;
            updBuf.type = BufferType::VERTEX;
            updBuf.offset = currentManager->getVertexOffset();
            updBuf.count = currentManager->getVertexCount();
            updBuf.size = currentManager->getVertexSize();
            updBuf.data = currentManager->getVertexData();
            this->atomixProg->updateBuffer(updBuf);
        }

        // Update VBO 2: Data
        if (flGraphState.hasAny(egs::UPD_DATA)) {
            std::string bufferCPU = (flGraphState.hasAny(egs::CPU_RENDER)) ? "DataCPU" : "Data";
            updBuf.bufferName = vw_currentModel + bufferCPU;
            updBuf.type = BufferType::DATA;
            if (flGraphState.hasAny(egs::CPU_RENDER)) {
                updBuf.offset = currentManager->getColourOffset();
                updBuf.count = currentManager->getColourCount();
                updBuf.size = currentManager->getColourSize();
                updBuf.data = currentManager->getColourData();
            } else {
                updBuf.offset = currentManager->getDataOffset();
                updBuf.count = currentManager->getDataCount();
                updBuf.size = currentManager->getDataSize();
                updBuf.data = currentManager->getDataData();
            }
            this->atomixProg->updateBuffer(updBuf);
        }

        // Update IBO: Indices
        if (flGraphState.hasAny(egs::UPD_IBO | egs::UPD_IDXOFF)) {
            updBuf.bufferName = vw_currentModel + "Indices";
            updBuf.type = BufferType::INDEX;
            updBuf.offset = currentManager->getIndexOffset();
            updBuf.count = currentManager->getIndexCount();
            updBuf.size = currentManager->getIndexSize();

            if (updBuf.size) {
                if (atomixProg->isSuspended(vw_currentModel)) {
                    atomixProg->resumeModel(vw_currentModel);
                }
                updBuf.data = (flGraphState.hasAny(egs::UPD_IDXOFF)) ? 0 : currentManager->getIndexData();
                this->atomixProg->updateBuffer(updBuf);
            } else {
                this->atomixProg->suspendModel(vw_currentModel);
            }
        }

        // Update Uniforms
        if (flGraphState.hasAny(egs::UPD_UNI_MATHS | egs::UPD_UNI_COLOUR)) {
            if (flGraphState.hasAny(egs::UPD_UNI_MATHS)) {
                this->waveManager->getMaths(this->vw_wave.waveMaths);
            }
            if (flGraphState.hasAny(egs::UPD_UNI_COLOUR)) {
                this->waveManager->getColours(this->vw_wave.waveColours);
            }

            for (int i = 0; i < MAX_CONCURRENT_FRAME_COUNT; i++) {
                this->atomixProg->updateUniformBuffer(i, "WaveState", sizeof(this->vw_wave), &this->vw_wave);
            }
        }

        if (flGraphState.hasAny(egs::UPD_PUSH_CONST)) {
            pConstWave.mode = waveManager->getMode();
        }

        if (flGraphState.hasAny(egs::UPD_MATRICES)) {
            initVecsAndMatrices();
        }

        if (flGraphState.hasNone(egs::WAVE_RENDER | egs::CLOUD_RENDER) && flGraphState.hasAny(egs::WAVE_MODE | egs::CLOUD_MODE)) {
            if (!vw_previousModel.empty()) this->atomixProg->deactivateModel(vw_previousModel);
            this->atomixProg->activateModel(vw_currentModel);
            
            std::string program = "default";
            if (flGraphState.hasAny(egs::CPU_RENDER)) {
                program = "cpu";
            } else if (flGraphState.hasAny(egs::WAVE_MODE) && waveManager->getSphere()) {
                program = "sphere";
            } else {
                program = "default";
            }
            this->atomixProg->addModelProgram(vw_currentModel, program);

            flGraphState.set(flGraphState.hasAny(egs::WAVE_MODE) ? egs::WAVE_RENDER : egs::CLOUD_RENDER);
        }

        flGraphState.clear(eUpdateFlags);
        this->updateBufferSizes();
    }

    atomixProg->updateUniformBuffer(this->currentSwapChainImageIndex(), "WorldState", sizeof(this->vw_world), &this->vw_world);
}

void VKWindow::setBGColour(float colour) {
    vw_bg = colour;
    this->atomixProg->updateClearColor(vw_bg, vw_bg, vw_bg, 1.0f);
}

void VKWindow::estimateSize(AtomixCloudConfig *cfg, harmap *cloudMap, uint *vertex, uint *data, uint *index) {
    uint layer_max = cloudManager->getMaxLayer(cfg->cloudTolerance, cloudMap->rbegin()->first, cfg->cloudLayDivisor);
    uint pixel_count = (layer_max * cfg->cloudResolution * cfg->cloudResolution) >> 1;

    (*vertex) = (pixel_count << 2) * 3;     // (count)   * (3 floats) * (4 B/float) * (1 vector)  -- only allVertices
    (*data) = pixel_count << 2;             // (count)   * (1 float)  * (4 B/float) * (1 vectors) -- only allData [already clear()ing dataStaging; might delete it]
    (*index) = (pixel_count << 1) * 3;      // (count/2) * (1 uint)   * (4 B/uint)  * (3 vectors) -- idxTolerance + idxSlider + allIndices [very rough estimate]
}

void VKWindow::threadFinished() {
    flGraphState.set(currentManager->clearUpdates() | egs::UPDATE_REQUIRED);
    emit toggleLoading(false);
}

void VKWindow::threadFinishedWithResult(uint result) {
    flGraphState.set(currentManager->clearUpdates() | egs::UPDATE_REQUIRED | result);
}

std::string VKWindow::withCommas(int64_t value) {
    std::stringstream ssFmt;
    ssFmt.imbue(std::locale(""));
    ssFmt << std::fixed << value;

    return ssFmt.str();
}

void VKWindow::updateBufferSizes() {
    uint64_t VSize = 0, DSize = 0, ISize = 0;
    vw_info.vertex = 0;
    vw_info.data = 0;
    vw_info.index = 0;

    if (flGraphState.hasAny(egs::WAVE_RENDER | egs::CLOUD_RENDER)) {
        VSize = currentManager->getVertexSize();            // (count)   * (3 floats) * (4 B/float) * (1 vector)  -- only allVertices
        ISize = currentManager->getIndexSize();         // (count/2) * (1 uint)   * (4 B/uint)  * (3 vectors) -- idxTolerance + idxSlider + allIndices [very rough estimate]}
        if (flGraphState.hasAny(egs::CLOUD_RENDER)) {
            DSize = currentManager->getDataSize();          // (count)   * (1 float)  * (4 B/float) * (1 vectors) -- only allData [already clear()ing dataStaging; might delete it]
            ISize *= 3;
        }
    }

    vw_info.vertex = VSize;
    vw_info.data = DSize;
    vw_info.index = ISize;

    emit detailsChanged(&vw_info);
}

void VKWindow::printSize() {
    updateBufferSizes();
    
    std::array<double, 4> bufs = { static_cast<double>(vw_info.vertex), static_cast<double>(vw_info.data), static_cast<double>(vw_info.index), 0 };
    std::array<std::string, 4> labels = { "Vertex:  ", "Data:    ", "Index:   ", "TOTAL:   " };
    std::array<std::string, 4> units = { " B", "KB", "MB", "GB" };
    std::array<int, 4> uIdx = { 0, 0, 0, 0 };
    double div = 1024;
    
    bufs[3] = std::accumulate(bufs.cbegin(), bufs.cend(), 0.0);

    for (auto [b, u] : std::views::zip(bufs, uIdx)) {
        while (b > div) {
            b /= div;
            u++;
        }
    }

    std::cout << "[ Total Buffer Sizes ]\n";
    for (auto [lab, b, u] : std::views::zip(labels, bufs, uIdx)) {
        if (b) {
            std::cout << lab << std::setprecision(2) << std::fixed << std::setw(6) << b << " " << units[u] << "\n";
        }
    }
    std::cout << std::endl;
}

/**
 * @brief Print the current value of flGraphState in a human-readable format.
 * 
 * @param str A string to print before the flags.
 * 
 * This function will print a string followed by a newline, followed by the names of all the flags in flGraphState that are set, each on a new line.
 * 
 * The order is determined by the position of the flag in the enum egs.
 * 
 * Example output:
 * Wave Mode
 * Thread Finished
 * Update Matrices
 * Update Required
 */
void VKWindow::printFlags(std::string str) {
    std::vector<std::string> labels = { "Wave Mode", "Wave Render", "Cloud Mode", "Cloud Render", "Thread Finished", "Update Vert Shader", "Update Frag Shader",\
                                        "Update VBO", "Update Data", "Update IBO", "Update Uniform Colour", "Update Uniform Maths", "Update Matrices", "Update Required"  };
    std::cout << str << std::endl;
    for (int i = 13; i >= 0; i--) {
        if (flGraphState.hasAny(1 << i)) {
            std::cout << labels[i] << "\n";
        }
    }
    std::cout << std::endl;
}

/**
 * @brief Returns the smallest value that is greater than or equal to v and a multiple of byteAlign.
 * 
 * This is a utility function for aligning a VkDeviceSize to a particular byte boundary.
 * @param v A VkDeviceSize object.
 * @param byteAlign The byte boundary to align to.
 * 
 * This function returns the smallest value that is greater than or equal to v and a multiple of byteAlign.
 * The implementation is based on the formula:
 *   (v + byteAlign - 1) & ~(byteAlign - 1)
 * This works by adding the byteAlign to v, subtracting 1, and then using the bitwise and operator to zero out any bits that are not part of the byteAlign.
 * 
 * @return The aligned VkDeviceSize.
 */
/* static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign) {
    return (v + byteAlign - 1) & ~(byteAlign - 1);
} */

/*
 * VKRenderer
 * 
 * This class is responsible for the Vulkan rendering of the window.
 * 
 * The class is designed to be used with the QVulkanWindow class.
 * 
 * The class is not thread-safe.
 * 
 */

VKRenderer::VKRenderer(QVulkanWindow *vkWin)
    : vr_qvw(vkWin) {
}

VKRenderer::~VKRenderer() {
}

void VKRenderer::preInitResources() {
    // TODO preInitResources
}

void VKRenderer::initResources() {
    // Set Window pointer
    std::cout << "initResources" << std::endl;

    // Define instance, device, and function pointers
    vr_dev = vr_qvw->device();
    vr_phydev = vr_qvw->physicalDevice();
    vr_vi = vr_qvw->vulkanInstance();
    vr_vdf = vr_vi->deviceFunctions(vr_dev);
    vr_vf = vr_vi->functions();

    // Retrieve physical device constraints
    if (!vr_isInit) {
        VkPhysicalDeviceProperties vr_props;
        vr_vf->vkGetPhysicalDeviceProperties(vr_phydev, &vr_props);

        QVersionNumber version = QVersionNumber(VK_API_VERSION_MAJOR(vr_props.apiVersion), VK_API_VERSION_MINOR(vr_props.apiVersion), VK_API_VERSION_PATCH(vr_props.apiVersion));
        if (version.minorVersion() != VK_MINOR_VERSION) {
            VK_MINOR_VERSION = version.minorVersion();
            if (VK_MINOR_VERSION >= 3) {
                VK_SPIRV_VERSION = 6;
            } else if (VK_MINOR_VERSION == 2) {
                VK_SPIRV_VERSION = 5;
            } else if (VK_MINOR_VERSION == 1) {
                VK_SPIRV_VERSION = 3;
            } else {
                VK_SPIRV_VERSION = 0;
            }
            if (isDebug) {
                std::cout << "Post-Device-Query Reassignment: Vulkan API version: " << version.toString().toStdString() << std::endl;
                std::cout << "Post-Device-Query Reassignment: Vulkan SPIRV version: 1." << VK_SPIRV_VERSION << std::endl;
            }
        }
        
        const VkPhysicalDeviceLimits *phydevLimits = &vr_props.limits;
        this->vr_minUniAlignment = phydevLimits->minUniformBufferOffsetAlignment;
        
        if (isDebug) {
            const VkDeviceSize uniBufferSize = phydevLimits->maxUniformBufferRange;
            std::cout << "uniAlignment: " << this->vr_minUniAlignment << " uniBufferSize: " << uniBufferSize << "\n" << std::endl;

            QString dev_info;
            int deviceCount = int(vr_qvw->availablePhysicalDevices().count());
            dev_info += QString::asprintf("Number of physical devices: %d\n", deviceCount);
            for (int i = 0; i < deviceCount; i++) {
                dev_info += QString::asprintf("Device %d: '%s' version %d.%d.%d\nAPI version %d.%d.%d\n",
                    i,
                    vr_qvw->availablePhysicalDevices().at(i).deviceName,
                    VK_VERSION_MAJOR(vr_qvw->availablePhysicalDevices().at(i).driverVersion),
                    VK_VERSION_MINOR(vr_qvw->availablePhysicalDevices().at(i).driverVersion),
                    VK_VERSION_PATCH(vr_qvw->availablePhysicalDevices().at(i).driverVersion),
                    VK_VERSION_MAJOR(vr_qvw->availablePhysicalDevices().at(i).apiVersion),
                    VK_VERSION_MINOR(vr_qvw->availablePhysicalDevices().at(i).apiVersion),
                    VK_VERSION_PATCH(vr_qvw->availablePhysicalDevices().at(i).apiVersion));
            }

            dev_info += QString::asprintf("Active physical device name: '%s' version %d.%d.%d\nAPI version %d.%d.%d\n",
                vr_props.deviceName,
                VK_VERSION_MAJOR(vr_props.driverVersion), VK_VERSION_MINOR(vr_props.driverVersion),
                VK_VERSION_PATCH(vr_props.driverVersion),
                VK_VERSION_MAJOR(vr_props.apiVersion), VK_VERSION_MINOR(vr_props.apiVersion),
                VK_VERSION_PATCH(vr_props.apiVersion));

            dev_info += QStringLiteral("Supported instance layers:\n");
            for (const QVulkanLayer &layer : vr_vi->supportedLayers())
                dev_info += QString::asprintf("    %s v%u\n", layer.name.constData(), layer.version);
            dev_info += QStringLiteral("Enabled instance layers:\n");
            for (const QByteArray &layer : vr_vi->layers())
                dev_info += QString::asprintf("    %s\n", layer.constData());

            dev_info += QStringLiteral("Supported instance extensions:\n");
            for (const QVulkanExtension &ext : vr_vi->supportedExtensions())
                dev_info += QString::asprintf("    %s v%u\n", ext.name.constData(), ext.version);
            dev_info += QStringLiteral("Enabled instance extensions:\n");
            for (const QByteArray &ext : vr_vi->extensions())
                dev_info += QString::asprintf("    %s\n", ext.constData());

            dev_info += QString::asprintf("Color format: %u\nDepth-stencil format: %u\n",
                                    vr_qvw->colorFormat(), vr_qvw->depthStencilFormat());

            dev_info += QStringLiteral("Supported sample counts:");
            const QList<int> sampleCounts = vr_qvw->supportedSampleCounts();
            for (int count : sampleCounts)
                dev_info += QLatin1Char(' ') + QString::number(count);
            dev_info += QLatin1Char('\n');

            std::cout << dev_info.toStdString() << std::endl;
        }
        vr_isInit = true;
    }

    // Create Program
    AtomixDevice progDev;
    progDev.window = vr_qvw;
    progDev.physicalDevice = vr_phydev;
    progDev.device = vr_dev;
    if (this->vr_vkw->initProgram(&progDev)) {
        this->vr_vkw->initWindow();
    }
}

void VKRenderer::initSwapChainResources() {
    std::cout << "initSwapChainResources" << std::endl;

    this->atomixProg->resumeActiveModels();

    // Update render extent
    const QSize vkwSize = vr_qvw->swapChainImageSize();
    VkExtent2D renderExtent = {static_cast<uint32_t>(vkwSize.width()), static_cast<uint32_t>(vkwSize.height())};
    this->vr_vkw->updateExtent(renderExtent);
    this->vr_extent = renderExtent;

    /* this->d_framebuffer = this->vr_vkw->currentFramebuffer();
    this->d_swapCount = this->vr_vkw->swapChainImageCount();
    this->d_swapIndex = this->vr_vkw->currentSwapChainImageIndex();
    this->d_renderPass = this->vr_vkw->defaultRenderPass(); */
}

void VKRenderer::logicalDeviceLost() {
    // TODO logicalDeviceLost
    std::cout << "logicalDeviceLost" << std::endl;
}

void VKRenderer::physicalDeviceLost() {
    // TODO physicalDeviceLost
    std::cout << "physicalDeviceLost" << std::endl;
}

void VKRenderer::releaseSwapChainResources() {
    // TODO releaseSwapChainResources
    std::cout << "releaseSwapChainResources" << std::endl;

    this->atomixProg->suspendActiveModels();
}

/**
 * @brief Release all Vulkan resources allocated by this object.
 *
 * This function should be called when the renderer is no longer needed.
 * It will release all Vulkan resources allocated by the renderer,
 * including pipelines, pipeline layouts, pipeline caches, descriptor
 * set layouts, descriptor pools, buffers, and buffer memory.
 *
 * Note that this function does not release the Vulkan instance or
 * physical device, as these are owned by the QVulkanWindow.
 */
void VKRenderer::releaseResources() {
    std::cout << "releaseResources" << std::endl;
    
    this->vr_vkw->releaseWindow();
}

void VKRenderer::startNextFrame() {
    /*
    VkFramebuffer t_framebuffer = this->vr_vkw->currentFramebuffer();
    VkRenderPass t_renderPass = this->vr_vkw->defaultRenderPass();
    int t_swapCount = this->vr_vkw->swapChainImageCount();
    int t_swapIndex = this->vr_vkw->currentSwapChainImageIndex();

    if (t_framebuffer == this->d_framebuffer) {
        std::cout << "framebuffer NOT changed" << std::endl;
    } else {
        this->d_framebuffer = t_framebuffer;
    }

    if (t_renderPass != this->d_renderPass) {
        std::cout << "renderPass changed" << std::endl;
    }

    if (t_swapCount != this->d_swapCount) {
        std::cout << "swapCount changed" << std::endl;
    }

    if (t_swapIndex == this->d_swapIndex) {
        std::cout << "swapIndex NOT changed" << std::endl;
    } else {
        this->d_swapIndex = t_swapIndex;
    }

    std::cout << d_idx++ << std::endl;
    */

    // Reap zombie buffers
    this->atomixProg->reapZombies();

    // Update buffers and shaders if necessary
    this->vr_vkw->updateBuffersAndShaders();

    // Call Program to render
    atomixProg->render(vr_extent);
    
    // Prepare for next frame
    vr_qvw->frameReady();
    vr_qvw->requestUpdate();
}
