/**
 * vkwindow.cpp
 *
 *    Created on: Oct 22, 2024
 *   Last Update: Oct 22, 2024
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
#include <ranges>

#include <QTimer>
#include <QObject>

#include "vkwindow.hpp"

#define RADN(t) (glm::radians((t)))

static const int UNIFORM_DATA_SIZE = 16 * sizeof(float);


VKWindow::VKWindow(QWidget *parent, ConfigParser *configParser)
    : cfgParser(configParser) {
    setSurfaceType(QVulkanWindow::VulkanSurface);
    this->setDeviceExtensions({ "VK_KHR_portability_subset" });
    std::cout << "Window has been created!" << std::endl;
}

VKWindow::~VKWindow() {
    cleanup();
}

void VKWindow::cleanup() {
    if (cloudManager || waveManager) {
        fwModel->waitForFinished();
    }
    changeModes(true);
    delete vw_timer;
}

QVulkanWindowRenderer* VKWindow::createRenderer() {
    std::cout << "Renderer has been created!" << std::endl;

    vw_renderer = new VKRenderer(this);
    vw_renderer->setWindow(this);
    
    return vw_renderer;
}

void VKWindow::initProgram(AtomixDevice *atomixDevice) {
    atomixProg = new ProgramVK();
    atomixProg->setInstance(atomixDevice);
    atomixProg->addAllShaders(&cfgParser->vshFiles, GL_VERTEX_SHADER);
    atomixProg->addAllShaders(&cfgParser->fshFiles, GL_FRAGMENT_SHADER);
    atomixProg->init();

    vw_renderer->setProgram(atomixProg);

    std::cout << "Program has been initialized!" << std::endl;

    std::cout << "Program has been updated with uniforms!" << std::endl;
}

void VKWindow::initWindow() {
    // Init -- Matrices
    initVecsAndMatrices();

    // Init -- ProgramGLs and Shaders
    initModels();

    this->atomixProg->activateModel("crystal");

    // Init -- Time
    vw_timeStart = QDateTime::currentMSecsSinceEpoch();

    // Init -- Threading
    fwModel = new QFutureWatcher<void>;
    connect(fwModel, &QFutureWatcher<void>::finished, this, &VKWindow::threadFinished);

    this->vw_init = true;
}

void VKWindow::newCloudConfig(AtomixConfig *config, harmap *cloudMap, int numRecipes, bool canCreate) {
    flGraphState.set(egs::CLOUD_MODE);
    if (flGraphState.hasAny(eWaveFlags)) {
        changeModes(false);
    }

    if (!cloudManager && canCreate) {
        // Initialize cloudManager -- will flow to initCloudManager() in PaintGL() for initial uploads since no EBO exists (after thread finishes)
        cloudManager = new CloudManager(config, *cloudMap, numRecipes);
        currentManager = cloudManager;
        futureModel = QtConcurrent::run(&CloudManager::initManager, cloudManager);
        // futureModel = QtConcurrent::run(&CloudManager::testThreadingInit, cloudManager);
    } else if (cloudManager) {
        // Inculdes resetManager() and clearForNext() -- will flow to updateCloudBuffers() in PaintGL() since EBO exists (after thread finishes)
        futureModel = QtConcurrent::run(&CloudManager::receiveCloudMapAndConfig, cloudManager, config, cloudMap, numRecipes);
    }
    if (cloudManager) {
        fwModel->setFuture(futureModel);
        this->max_n = cloudMap->rbegin()->first;
        emit toggleLoading(true);
    }
}

void VKWindow::newWaveConfig(AtomixConfig *config) {
    flGraphState.set(egs::WAVE_MODE);
    if (flGraphState.hasAny(eCloudFlags)) {
        changeModes(false);
    }

    if (!waveManager) {
        // Initialize waveManager -- will flow to initCloudManager() in PaintGL() for initial uploads since no EBO exists (after thread finishes)
        waveManager = new WaveManager(config);
        currentManager = waveManager;
        futureModel = QtConcurrent::run(&WaveManager::initManager, waveManager);
    } else {
        // Inculdes resetManager() and clearForNext() -- will flow to updateCloudBuffers() in PaintGL() since EBO exists (after thread finishes)
        futureModel = QtConcurrent::run(&WaveManager::receiveConfig, waveManager, config);
    }
    fwModel->setFuture(futureModel);
    emit toggleLoading(true);
}

void VKWindow::selectRenderedWaves(int id, bool checked) {
    // Process and flag for EBO update
    flGraphState.set(waveManager->selectWaves(id, checked) | egs::UPDATE_REQUIRED);
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
          zero,  peak,  zero,   0.6f, 0.6f, 0.6f,    //top
         -forX,  zero,  forZ,   0.1f, 0.4f, 0.4f,    //left - cyan
          forX,  zero,  forZ,   0.4f, 0.1f, 0.4f,    //right - magenta
          zero,  zero, -back,   0.4f, 0.4f, 0.1f,    //back - yellow
          zero, -peak,  zero,   0.0f, 0.0f, 0.0f     //bottom
    };

    const std::array<VKuint, 18> indices = {
        1, 0, 2,
        2, 0, 3,
        3, 0, 1,
        1, 4, 3,
        3, 4, 2,
        2, 4, 1
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

    for (uint32_t i = 0; i < crystalRes; i++) {
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
    this->crystalRingCount = crystalRingIndices.size() - vw_faces;
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

    // Add Crystal Model to program
    atomixProg->addModel(crystalModel);
}

void VKWindow::initWaveModel() {
    // Define VBO for Atomix Wave
    BufferCreateInfo waveVert{};
    waveVert.binding = 0;
    waveVert.name = "waveVertices";
    waveVert.type = BufferType::VERTEX;
    waveVert.dataTypes = { DataType::FLOAT_VEC3 };

    // Define IBO for Atomix Wave
    BufferCreateInfo waveInd{};
    waveInd.name = "waveIndices";
    waveInd.type = BufferType::INDEX;
    waveInd.dataTypes = { DataType::UINT };

    // Define Atomix Wave Model with above buffers
    ModelCreateInfo waveModel{};
    waveModel.name = "wave";
    waveModel.vbos = { &waveVert };
    waveModel.ibo = &waveInd;
    waveModel.ubos = { "WorldState", "WaveState" };
    waveModel.vertShaders = { "gpu_circle.vert", "gpu_sphere.vert" };
    waveModel.fragShaders = { "default.frag" };
    waveModel.pushConstant = "pushConst";
    waveModel.topologies = { VK_PRIMITIVE_TOPOLOGY_POINT_LIST };
    waveModel.bufferCombos = { { 0 } };
    waveModel.offsets = {
        {   .offset = 0,
            .vertShaderIndex = 0,
            .fragShaderIndex = 0,
            .topologyIndex = 0,
            .bufferComboIndex = 0,
            .pushConstantIndex = 0
        }
    };

    // Add Atomix Wave Model to program
    atomixProg->addModel(waveModel);

    // Activate Atomix Wave Model
    atomixProg->updatePushConstant("pushConst", &this->vw_renderer->pConst);
}

void VKWindow::initCloudModel() {
    // Define VBO::Vertex for Atomix Cloud
    BufferCreateInfo cloudVert{};
    cloudVert.binding = 0;
    cloudVert.type = BufferType::VERTEX;
    cloudVert.name = "cloudVertices";
    cloudVert.dataTypes = { DataType::FLOAT_VEC3 };

    // Define VBO::Data for Atomix Cloud
    BufferCreateInfo cloudData{};
    cloudData.binding = 1;
    cloudData.type = BufferType::DATA;
    cloudData.name = "cloudData";
    cloudData.dataTypes = { DataType::FLOAT };

    // Define IBO for Atomix Cloud
    BufferCreateInfo cloudInd{};
    cloudInd.type = BufferType::INDEX;
    cloudInd.name = "cloudIndices";
    cloudInd.dataTypes = { DataType::UINT };

    // Define Atomix Cloud Model with above buffers
    ModelCreateInfo cloudModel{};
    cloudModel.name = "cloud";
    cloudModel.vbos = { &cloudVert, &cloudData };
    cloudModel.ibo = &cloudInd;
    cloudModel.ubos = { "WorldState" };
    cloudModel.vertShaders = { "gpu_harmonics.vert" };
    cloudModel.fragShaders = { "default.frag" };
    cloudModel.topologies = { VK_PRIMITIVE_TOPOLOGY_POINT_LIST };
    cloudModel.bufferCombos = { { 0, 1 } };
    cloudModel.offsets = {
        {   .offset = 0,
            .vertShaderIndex = 0,
            .fragShaderIndex = 0,
            .topologyIndex = 0,
            .bufferComboIndex = 0
        }
    };

    // vbo=( 0 , 1 )
    // each vbo has its (binding and) attrs, locations are assigned dynamically

    // Add Atomix Cloud Model to program
    atomixProg->addModel(cloudModel);
}

void VKWindow::initModels() {
    initCrystalModel();
    initWaveModel();
    initCloudModel();
}

void VKWindow::changeModes(bool force) {
    if (!waveManager || force) {
        delete cloudManager;
        cloudManager = 0;
        atomixProg = 0;
        flGraphState.clear(eCloudFlags);
    } else if (!cloudManager || force) {
        delete waveManager;
        waveManager = 0;
        atomixProg = 0;
        flGraphState.clear(eWaveFlags);
    }
    currentManager = 0;
}

void VKWindow::initVecsAndMatrices() {
    vw_startDist = (flGraphState.hasNone(egs::CLOUD_MODE)) ? 16.0f : (10.0f + 6.0f * (this->max_n * this->max_n));
    vw_nearDist = 0.1f;
    vw_farDist = vw_startDist * 2.0f;

    q_TotalRot.zero();
    m4_rotation = glm::mat4(1.0f);
    m4_translation = glm::mat4(1.0f);
    vw_world.m4_proj = glm::mat4(1.0f);
    vw_world.m4_view = glm::mat4(1.0f);
    vw_world.m4_world = glm::mat4(1.0f);

    v3_cameraPosition = glm::vec3(0.0f, 0.0f, vw_startDist);
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
    vw_aspect = width / height;
    vw_world.m4_proj = glm::perspective(RADN(45.0f), vw_aspect, vw_nearDist, vw_farDist);
    vw_world.m4_proj[1][1] *= -1.0f;

    atomixProg->updateClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    vw_info.pos = vw_startDist;
    vw_info.start = vw_startDist;
    vw_info.near = vw_nearDist;
    vw_info.far = vw_farDist;
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
    vw_world.m4_proj = glm::perspective(RADN(45.0f), VKfloat(width()) / height(), vw_nearDist, vw_info.far);
    vw_world.m4_proj[1][1] *= -1.0f;
    emit detailsChanged(&vw_info);
    requestUpdate();
}

void VKWindow::mousePressEvent(QMouseEvent *e) {
    glm::vec3 mouseVec = glm::vec3(e->pos().x(), height() - e->pos().y(), v3_cameraPosition.z);
    v3_mouseBegin = mouseVec;
    v3_mouseEnd = mouseVec;

    if (!vw_movement && (e->button() & (Qt::LeftButton | Qt::RightButton | Qt::MiddleButton)))
        vw_movement |= e->button();
    else
        QWindow::mousePressEvent(e);
}

void VKWindow::mouseMoveEvent(QMouseEvent *e) {
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
    if (e->button() & (Qt::RightButton | Qt::LeftButton | Qt::MiddleButton))
        vw_movement = false;
    else
        QWindow::mouseReleaseEvent(e);
}

void VKWindow::keyPressEvent(QKeyEvent * e) {
    if (e->key() == Qt::Key_Home) {
        initVecsAndMatrices();
        requestUpdate();
    } else if (e->key() == Qt::Key_Space) {
        vw_pause = !vw_pause;
        if (vw_pause) {
            vw_timePaused = QDateTime::currentMSecsSinceEpoch();
        } else {
            vw_timeEnd = QDateTime::currentMSecsSinceEpoch();
            vw_timeStart += vw_timeEnd - vw_timePaused;
        }
        requestUpdate();
    } else {
        QWindow::keyPressEvent(e);
    }
}

void VKWindow::setColorsWaves(int id, uint colorChoice) {
    switch (id) {
    case 1:
        waveManager->waveColours.r = colorChoice;
        break;
    case 2:
        waveManager->waveColours.g = colorChoice;
        break;
    case 3:
        waveManager->waveColours.b = colorChoice;
        break;
    default:
        break;
    }
    flGraphState.set(egs::UPD_UNI_COLOUR | egs::UPDATE_REQUIRED);
}

void VKWindow::updateExtent(VkExtent2D &renderExtent) {
    vw_extent = renderExtent;
}

void VKWindow::updateBuffersAndShaders() {
    if (flGraphState.hasNone(egs::UPDATE_REQUIRED)) {
        return;
    }

    // Capture updates from currentManager
    uint flags = currentManager->clearUpdates();
    flGraphState.set(flags);
    this->updateSize();

    // Set current model
    if (flGraphState.hasAny(egs::WAVE_MODE)) {
        vw_currentModel = "wave";
    } else if (flGraphState.hasAny(egs::CLOUD_MODE)) {
        vw_currentModel = "cloud";
    }
    
    // Shaders
    if (flGraphState.hasAny(egs::UPD_SHAD_V | egs::UPD_SHAD_F)) {
        OffsetInfo off{};
        off.vertShaderIndex = 1;
        off.offset = 0;
        // atomixProg->updateRender("wave", off);
    }
    BufferUpdateInfo updBuf{};
    updBuf.modelName = vw_currentModel;

    // VBO 1: Vertices
    if (flGraphState.hasAny(egs::UPD_VBO)) {
        updBuf.bufferName = vw_currentModel + "Vertices";
        updBuf.type = BufferType::VERTEX;
        updBuf.count = currentManager->getVertexCount();
        updBuf.size = currentManager->getVertexSize();
        updBuf.data = currentManager->getVertexData();
        atomixProg->updateBuffer(updBuf);
    }

    // VBO 2: Data
    if (flGraphState.hasAny(egs::UPD_DATA)) {
        updBuf.bufferName = vw_currentModel + "Data";
        updBuf.type = BufferType::DATA;
        updBuf.count = currentManager->getDataCount();
        updBuf.size = currentManager->getDataSize();
        updBuf.data = currentManager->getDataData();
        atomixProg->updateBuffer(updBuf);
    }

    // EBO: Indices
    if (flGraphState.hasAny(egs::UPD_EBO)) {
        updBuf.bufferName = vw_currentModel + "Indices";
        updBuf.type = BufferType::INDEX;
        updBuf.count = currentManager->getIndexCount();
        updBuf.size = currentManager->getIndexSize();
        updBuf.data = currentManager->getIndexData();
        atomixProg->updateBuffer(updBuf);
    }

    // Uniforms
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
        this->updateWaveMode(this->vw_renderer->pConst);
    }

    if (flGraphState.hasAny(egs::UPD_MATRICES)) {
        initVecsAndMatrices();
    }

    if (flGraphState.hasNone(egs::WAVE_RENDER | egs::CLOUD_RENDER)) {
        this->atomixProg->activateModel(vw_currentModel);
        flGraphState.set(flGraphState.hasAny(egs::WAVE_MODE) ? egs::WAVE_RENDER : egs::CLOUD_RENDER);
    }

    flGraphState.clear(eUpdateFlags);
}

void VKWindow::updateWorldState() {
    // Re-calculate world-state matrices
    m4_rotation = glm::make_mat4(&q_TotalRot.matrix()[0]);
    vw_world.m4_world = m4_translation * m4_rotation;
    vw_world.m4_view = glm::lookAt(v3_cameraPosition, v3_cameraTarget, v3_cameraUp);
    this->q_TotalRot.normalize();

    atomixProg->updateUniformBuffer(this->currentSwapChainImageIndex(), "WorldState", sizeof(this->vw_world), &this->vw_world);
}

void VKWindow::updateTime(PushConstants &pConst) {
    if (!vw_pause) {
        vw_timeEnd = QDateTime::currentMSecsSinceEpoch();
    }
    float time = (vw_timeEnd - vw_timeStart) / 1000.0f;

    pConst.time = time;
}

void VKWindow::updateWaveMode(PushConstants &pConst) {
    pConst.mode = waveManager->getMode();
}

void VKWindow::setBGColour(float colour) {
    vw_bg = colour;
}

void VKWindow::estimateSize(AtomixConfig *cfg, harmap *cloudMap, uint *vertex, uint *data, uint *index) {
    uint layer_max = cloudManager->getMaxRadius(cfg->cloudTolerance, cloudMap->rbegin()->first, cfg->cloudLayDivisor);
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

void VKWindow::updateSize() {
    uint64_t VSize = 0, DSize = 0, ISize = 0;
    vw_info.vertex = 0;
    vw_info.data = 0;
    vw_info.index = 0;

    if (flGraphState.hasAny(egs::WAVE_RENDER | egs::CLOUD_RENDER)) {
        VSize = currentManager->getVertexSize();            // (count)   * (3 floats) * (4 B/float) * (1 vector)  -- only allVertices
        ISize = currentManager->getIndexSize() * 3;         // (count/2) * (1 uint)   * (4 B/uint)  * (3 vectors) -- idxTolerance + idxSlider + allIndices [very rough estimate]}
        if (flGraphState.hasAny(egs::CLOUD_RENDER)) {
            DSize = currentManager->getDataSize();          // (count)   * (1 float)  * (4 B/float) * (1 vectors) -- only allData [already clear()ing dataStaging; might delete it]
        }
    }

    vw_info.vertex = VSize;
    vw_info.data = DSize;
    vw_info.index = ISize;

    emit detailsChanged(&vw_info);
}

void VKWindow::printSize() {
    updateSize();
    
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
                                        "Update VBO", "Update Data", "Update EBO", "Update Uniform Colour", "Update Uniform Maths", "Update Matrices", "Update Required"  };
    std::cout << str << std::endl;
    for (int i = 13; i >= 0; i--) {
        if (flGraphState.hasAny(1 << i)) {
            std::cout << labels[i] << "\n";
        }
    }
    std::cout << std::endl;
}

void VKWindow::printConfig(AtomixConfig *cfg) {
    std::cout << "Waves: " << cfg->waves << "\n";
    std::cout << "Amplitude: " << cfg->amplitude << "\n";
    std::cout << "Period: " << cfg->period << "\n";
    std::cout << "Wavelength: " << cfg->wavelength << "\n";
    std::cout << "Resolution: " << cfg->resolution << "\n";
    std::cout << "Parallel: " << cfg->parallel << "\n";
    std::cout << "Superposition: " << cfg->superposition << "\n";
    std::cout << "CPU: " << cfg->cpu << "\n";
    std::cout << "Sphere: " << cfg->sphere << "\n";
    std::cout << "Vert Shader: " << cfg->vert << "\n";
    std::cout << "Frag Shader: " << cfg->frag << std::endl;
}

/**
 * @brief Returns the smallest value that is greater than or equal to v and a multiple of byteAlign.
 * 
 * This is a utility function for aligning a VkDeviceSize to a particular byte boundary.
 * 
 * @param v A VkDeviceSize object.
 * @param byteAlign The byte boundary to align to.
 * 
 * This function returns the smallest value that is greater than or equal to v and a multiple of byteAlign.
 * 
 * The implementation is based on the formula:
 * 
 *   (v + byteAlign - 1) & ~(byteAlign - 1)
 * 
 * This works by adding the byteAlign to v, subtracting 1, and then using the bitwise and operator to zero out any bits that are not part of the byteAlign.
 * 
 * @return The aligned VkDeviceSize.
 */
static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

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
    std::cout << "preInitResources" << std::endl;
}

void VKRenderer::initResources() {
    // Set Window pointer
    // vr_qvw->setRenderer(this);

    // Define instance, device, and function pointers
    vr_dev = vr_qvw->device();
    vr_phydev = vr_qvw->physicalDevice();
    vr_vi = vr_qvw->vulkanInstance();
    vr_vdf = vr_vi->deviceFunctions(vr_dev);
    vr_vf = vr_vi->functions();

    // Retrieve physical device constraints
    QString dev_info;
    dev_info += QString::asprintf("Number of physical devices: %d\n", int(vr_qvw->availablePhysicalDevices().count()));

    VkPhysicalDeviceProperties vr_props;
    vr_vf->vkGetPhysicalDeviceProperties(vr_phydev, &vr_props);

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

    const int concFrameCount = vr_qvw->concurrentFrameCount();
    const VkPhysicalDeviceLimits *phydevLimits = &vr_props.limits;
    this->vr_minUniAlignment = phydevLimits->minUniformBufferOffsetAlignment;
    const VkDeviceSize uniBufferSize = phydevLimits->maxUniformBufferRange;

    std::cout << "uniAlignment: " << this->vr_minUniAlignment << " uniBufferSize: " << uniBufferSize << "\n" << std::endl;

    // Create Program
    AtomixDevice *progDev = new AtomixDevice();
    progDev->window = vr_qvw;
    progDev->physicalDevice = vr_phydev;
    progDev->device = vr_dev;
    this->vr_vkw->initProgram(progDev);
    this->vr_vkw->initWindow();
}

/* SwapChainSupportInfo VKRenderer::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportInfo info;
    
    this->vr_vf->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, this->vr_qvw->vw_surface, &info.capabilities);
    
    VKuint formatCount = 0;
    this->vr_qvw->vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->vr_qvw->vw_surface, &formatCount, nullptr);
    if (formatCount) {
        info.formats.resize(formatCount);
        this->vr_qvw->vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->vr_qvw->vw_surface, &formatCount, info.formats.data());
    }

    VKuint presentModeCount = 0;
    this->vr_qvw->vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->vr_qvw->vw_surface, &presentModeCount, nullptr);
    if (presentModeCount) {
        info.presentModes.resize(presentModeCount);
        this->vr_qvw->vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->vr_qvw->vw_surface, &presentModeCount, info.presentModes.data());
    }
    
    return info;
} */

void VKRenderer::initSwapChainResources() {
    /* QSize swapChainImageSize = this->vr_qvw->swapChainImageSize();
    VkExtent2D extent = {swapChainImageSize.width(), swapChainImageSize.height()};
    this->vr_qvw->vw_surface = QVulkanInstance::surfaceForWindow(this->vr_qvw);
    SwapChainSupportInfo swapChainSupport = querySwapChainSupport(this->vr_phydev);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = this->vr_qvw->vw_surface;
    createInfo.minImageCount = 2;
    createInfo.imageFormat = swapChainSupport.formats[0].format;
    createInfo.imageColorSpace = swapChainSupport.formats[0].colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(this->vr_phydev);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = swapChainSupport.presentModes[0];
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain;
    if ((this->vr_vf->vkCreateSwapchainKHR(this->vr_dev, &createInfo, nullptr, &swapchain)) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }

    this->vr_qvw->setSwapChain(swapchain); */
    // QVulkanWindowRenderer::initSwapChainResources();

    // Update render extent
    const QSize vkwSize = vr_qvw->swapChainImageSize();
    VkExtent2D renderExtent = {static_cast<uint32_t>(vkwSize.width()), static_cast<uint32_t>(vkwSize.height())};
    this->vr_vkw->updateExtent(renderExtent);
    this->vr_extent = renderExtent;
}

void VKRenderer::releaseSwapChainResources() {
    // TODO releaseSwapChainResources
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
    delete atomixProg;
    atomixProg = 0;
}

void VKRenderer::startNextFrame() {
    // Update buffers and shaders if necessary
    this->vr_vkw->updateBuffersAndShaders();

    // Update world state (per-frame)
    this->vr_vkw->updateWorldState();

    // Update time (per-frame)
    if (this->vr_vkw->flGraphState.hasAny(egs::WAVE_RENDER)) {
        this->vr_vkw->updateTime(this->pConst);
    }

    // Call Program to render
    atomixProg->render(vr_extent);
    
    // Prepare for next frame
    vr_qvw->frameReady();
    vr_qvw->requestUpdate();
}
