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
#include "vkwindow.hpp"

#define RADN(t) (glm::radians((t)))

static const int UNIFORM_DATA_SIZE = 16 * sizeof(float);


VKWindow::VKWindow(QWidget *parent, ConfigParser *configParser)
    : cfgParser(configParser) {
    setSurfaceType(QVulkanWindow::VulkanSurface);
}

VKWindow::~VKWindow() {
    cleanup();
}

void VKWindow::cleanup() {
    fwModel->waitForFinished();
    changeModes(true);
    delete gw_timer;
    delete crystalProg;
}

VKRenderer* VKWindow::createRenderer() {
    this->vw_rend = new VKRenderer(this);
    return this->vw_rend;
}

void VKWindow::createPrograms() {
    crystalProg = new ProgramVK();
    waveProg = new ProgramVK();
    cloudProg = new ProgramVK();
}

void VKWindow::populatePrograms(AtomixDevice *atomixDevice) {
    crystalProg->setInstance(atomixDevice);
    waveProg->setInstance(atomixDevice);
    cloudProg->setInstance(atomixDevice);
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

void VKWindow::initCrystalProgram() {
    fvec crystalRingVertices;
    uvec crystalRingIndices;
    std::string vertName = "crystal.vert";
    std::string fragName = "crystal.frag";

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
        0, 1, 2,
        2, 3, 0,
        3, 1, 0,
        1, 2, 4,
        2, 3, 4,
        3, 4, 1
    };
    this->gw_faces = indices.size();

    // Crystal Ring setup
    int crystalRes = 80;
    double crystalDegFac = PI_TWO / crystalRes;
    double crystalRadius = 0.4f;
    size_t vs = vertices.size() / 6;

    std::copy(vertices.cbegin(), vertices.cend(), std::back_inserter(crystalRingVertices));
    std::copy(indices.cbegin(), indices.cend(), std::back_inserter(crystalRingIndices));

    for (int i = 0; i < crystalRes; i++) {
        double cos_t = cos(i * crystalDegFac);
        double sin_t = sin(i * crystalDegFac);
        crystalRingVertices.push_back(static_cast<float>(crystalRadius * cos_t));
        crystalRingVertices.push_back(0.0f);
        crystalRingVertices.push_back(static_cast<float>(crystalRadius * sin_t));
        crystalRingVertices.push_back(0.9f);
        crystalRingVertices.push_back(0.9f);
        crystalRingVertices.push_back(0.9f);
        crystalRingIndices.push_back(vs + i);
    }
    this->crystalRingCount = crystalRingIndices.size() - gw_faces;
    this->crystalRingOffset = gw_faces * sizeof(uint);

    // Add shaders to program
    crystalProg->addAllShaders(&cfgParser->vshFiles, GL_VERTEX_SHADER);
    crystalProg->addAllShaders(&cfgParser->fshFiles, GL_FRAGMENT_SHADER);
    crystalProg->init();
    
    // Add buffers to program
    BufferInfo crystalVert{};
    crystalVert.type = BufferType::VERTEX;
    crystalVert.binding = 0;
    crystalVert.size = crystalRingVertices.size() * sizeof(float);
    crystalVert.data = &crystalRingVertices[0];
    crystalVert.name = "vertices";
    crystalVert.dataTypes = {DataType::FLOAT_VEC3, DataType::FLOAT_VEC3};
    crystalProg->addBufferConfig(crystalVert);
    crystalProg->createVertexBuffer(&crystalVert);

    BufferInfo crystalInd{};
    crystalInd.type = BufferType::INDEX;
    crystalInd.binding = 1;
    crystalInd.size = crystalRingIndices.size() * sizeof(uint);
    crystalInd.data = &crystalRingIndices[0];
    crystalInd.name = "indices";
    crystalInd.dataTypes = {DataType::UINT};
    crystalProg->addBufferConfig(crystalInd);
}

void VKWindow::initWaveProgram() {
    assert(waveManager);

    /* ProgramVK */
    // Create ProgramVK
    waveProg = new ProgramVK(vrend->vdf);

    // Add all shaders
    waveProg->addAllShaders(&cfgParser->vshFiles, GL_VERTEX_SHADER);
    waveProg->addAllShaders(&cfgParser->fshFiles, GL_FRAGMENT_SHADER);
    waveProg->init();

    // Attach shaders, link/validate program, and clean up
    waveProg->attachShader(waveManager->getShaderVert());
    waveProg->attachShader(waveManager->getShaderFrag());
    waveProg->linkAndValidate();
    waveProg->detachShaders();

    /* VAO */
    waveProg->initVAO();
    waveProg->bindVAO();

    /* VBO: Vertices & Colours */
    // Dynamic Draw for updating vertices per-render (CPU) or Static Draw for one-time load (GPU)
    uint static_dynamic = waveManager->isCPU() ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    GLuint vboID = waveProg->bindVBO("vertices", waveManager->getVertexCount(), waveManager->getVertexSize(), waveManager->getVertexData(), static_dynamic);
    waveProg->setAttributeBuffer(0, vboID, 6 * sizeof(GLfloat));
    waveProg->enableAttribute(0);
    waveProg->setAttributePointerFormat(0, 0, 3, GL_FLOAT, 0, 0);                         // x,y,z coords or factorsA
    waveProg->enableAttribute(1);
    waveProg->setAttributePointerFormat(1, 0, 3, GL_FLOAT, 3 * sizeof(GLfloat), 0);       // r,g,b colour or factorsB

    /* EBO: Indices */
    waveProg->bindEBO("indices", waveManager->getIndexCount(), waveManager->getIndexSize(), waveManager->getIndexData(), static_dynamic);

    /* Release */
    waveProg->endRender();
    waveProg->clearBuffers();
    flGraphState.set(egs::WAVE_RENDER);
    flGraphState.set(egs::UPD_UNI_MATHS | egs::UPD_UNI_COLOUR);

    currentProg = waveProg;
    currentManager = waveManager;
}

void VKWindow::initCloudProgram() {
    assert(cloudManager);

    /* ProgramVK */
    // Create ProgramVK
    cloudProg = new ProgramVK(vrend->vdf);

    // Add all shaders
    cloudProg->addAllShaders(&cfgParser->vshFiles, GL_VERTEX_SHADER);
    cloudProg->addAllShaders(&cfgParser->fshFiles, GL_FRAGMENT_SHADER);
    cloudProg->init();

    // Attach shaders, link/validate program, and clean up
    cloudProg->attachShader(cloudManager->getShaderVert());
    cloudProg->attachShader(cloudManager->getShaderFrag());
    cloudProg->linkAndValidate();
    cloudProg->detachShaders();

    /* VAO */
    cloudProg->initVAO();
    cloudProg->bindVAO();

    /* VBO 1: Vertices */
    // Dynamic Draw for updating vertices per-render (CPU) or Static Draw for one-time load (GPU)
    uint static_dynamic = cloudManager->isCPU() ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    GLuint vboIDa = cloudProg->bindVBO("vertices", cloudManager->getVertexCount(), cloudManager->getVertexSize(), cloudManager->getVertexData(), static_dynamic);
    cloudProg->setAttributeBuffer(0, vboIDa, 3 * sizeof(GLfloat));
    cloudProg->enableAttribute(0);
    cloudProg->setAttributePointerFormat(0, 0, 3, GL_FLOAT, 0, 0);                         // x,y,z coords or factorsA

    /* VBO 2: RDPs */
    GLuint vboIDb = cloudProg->bindVBO("pdvs", cloudManager->getDataCount(), cloudManager->getDataSize(), cloudManager->getDataData(), static_dynamic);
    cloudProg->setAttributeBuffer(1, vboIDb, 1 * sizeof(GLfloat));
    cloudProg->enableAttribute(1);
    cloudProg->setAttributePointerFormat(1, 1, 3, GL_FLOAT, 0, 0);                         // r,g,b colour or factorsB

    /* EBO: Indices */
    GLuint eboId = cloudProg->bindEBO("indices", cloudManager->getIndexCount(), cloudManager->getIndexSize(), cloudManager->getIndexData(), static_dynamic);

    /* Release */
    cloudProg->endRender();
    cloudProg->clearBuffers();
    flGraphState.set(CLOUD_RENDER);

    currentProg = cloudProg;
    currentManager = cloudManager;
}

void VKWindow::changeModes(bool force) {
    if (!waveManager || force) {
        delete cloudManager;
        delete cloudProg;
        cloudManager = 0;
        cloudProg = 0;
        flGraphState.clear(eCloudFlags);
    } else if (!cloudManager || force) {
        delete waveManager;
        delete waveProg;
        waveManager = 0;
        waveProg = 0;
        flGraphState.clear(eWaveFlags);
    }
    currentManager = 0;
    currentProg = 0;
}

void VKWindow::initVecsAndMatrices() {
    gw_startDist = (flGraphState.hasNone(egs::CLOUD_MODE)) ? 16.0f : (10.0f + 6.0f * (this->max_n * this->max_n));
    gw_nearDist = 0.1f;
    gw_farDist = gw_startDist * 2.0f;

    q_TotalRot.zero();
    m4_rotation = glm::mat4(1.0f);
    m4_translation = glm::mat4(1.0f);
    m4_proj = glm::mat4(1.0f);
    m4_view = glm::mat4(1.0f);
    m4_world = glm::mat4(1.0f);
    v3_cameraPosition = glm::vec3(0.0f, 0.0f, gw_startDist);
    v3_cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    v3_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    v3_mouseBegin = glm::vec3(0);
    v3_mouseEnd = glm::vec3(0);

    m4_view = glm::lookAt(v3_cameraPosition, v3_cameraTarget, v3_cameraUp);
    m4_proj = glm::perspective(RADN(45.0f), VKfloat(width()) / height(), gw_nearDist, gw_farDist);

    gw_info.pos = gw_startDist;
    gw_info.start = gw_startDist;
    gw_info.near = gw_nearDist;
    gw_info.far = gw_farDist;
    emit detailsChanged(&gw_info);
}

// void VKWindow::initializeGL() {
//     /* Init -- OpenGL Context and Functions */
//     if (!context()) {
//         gw_context = new QOpenGLContext(this);
//         if (!gw_context->create())
//             std::cout << "Failed to create OpenGL context" << std::endl;
//     } else {
//         gw_context = context();
//     }
//     makeCurrent();
//     if (!gw_init) {
//         if (!initializeOpenGLFunctions())
//             std::cout << "Failed to initialize OpenGL functions" << std::endl;
//         else
//             gw_init = true;
//     }

//     /* Init -- Camera and OpenGL State */
//     glClearColor(gw_bg, gw_bg, gw_bg, 1.0f);
//     glEnable(GL_DEPTH_TEST);
//     glEnable(GL_BLEND);

//     /* Init -- Matrices */
//     initVecsAndMatrices();

//     /* Init -- ProgramVKs and Shaders */
//     initCrystalProgram();

//     /* Init -- Time */
//     gw_timeStart = QDateTime::currentMSecsSinceEpoch();
//     gw_timer = new QTimer(this);
//     connect(gw_timer, &QTimer::timeout, this, QOverload<>::of(&VKWindow::update));
//     gw_timer->start(33);

//     /* Init -- Threading */
//     fwModel = new QFutureWatcher<void>;
//     connect(fwModel, &QFutureWatcher<void>::finished, this, &VKWindow::threadFinished);
// }

// void VKWindow::paintGL() {
//     assert(flGraphState.hasSomeOrNone(egs::WAVE_MODE | egs::CLOUD_MODE));

//     if (!gw_pause)
//         gw_timeEnd = QDateTime::currentMSecsSinceEpoch();
//     float time = (gw_timeEnd - gw_timeStart) / 1000.0f;

//     /* Pre-empt painting for new or updated model configuration */
//     if (flGraphState.hasAny(egs::UPDATE_REQUIRED)) {
//         updateBuffersAndShaders();
//     }

//     /* Per-frame Setup */
//     const qreal retinaScale = devicePixelRatio();
//     glViewport(0, 0, width() * retinaScale, height() * retinaScale);
//     glClearColor(gw_bg, gw_bg, gw_bg, 1.0f);
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

//     /* Re-calculate world state matrices */
//     m4_rotation = glm::make_mat4(&q_TotalRot.matrix()[0]);
//     m4_world = m4_translation * m4_rotation;
//     m4_view = glm::lookAt(v3_cameraPosition, v3_cameraTarget, v3_cameraUp);

//     /* Render -- Crystal */
//     crystalProg->beginRender();
//     crystalProg->setUniformMatrix(4, "worldMat", glm::value_ptr(m4_world));
//     crystalProg->setUniformMatrix(4, "viewMat", glm::value_ptr(m4_view));
//     crystalProg->setUniformMatrix(4, "projMat", glm::value_ptr(m4_proj));
//     glDrawElements(GL_TRIANGLES, gw_faces, GL_UNSIGNED_INT, 0);
//     glDrawElements(GL_LINE_LOOP, crystalRingCount, GL_UNSIGNED_INT, reinterpret_cast<GLvoid *>(crystalRingOffset));
//     crystalProg->endRender();

//     /* Render -- Waves */
//     if (flGraphState.hasAll(egs::WAVE_MODE | egs::WAVE_RENDER) || flGraphState.hasAll(egs::CLOUD_MODE | egs::CLOUD_RENDER)) {
//         currentProg->beginRender();
//         if (flGraphState.hasAny(egs::WAVE_MODE)) {
//             if (currentManager->isCPU()) {
//                 currentManager->update(time);
//                 currentProg->updateVBONamed("vertices", currentManager->getVertexCount(), 0, currentManager->getVertexSize(), currentManager->getVertexData());
//             }
//         }
//         currentProg->setUniformMatrix(4, "worldMat", glm::value_ptr(m4_world));
//         currentProg->setUniformMatrix(4, "viewMat", glm::value_ptr(m4_view));
//         currentProg->setUniformMatrix(4, "projMat", glm::value_ptr(m4_proj));
//         currentProg->setUniform(GL_FLOAT, "time", time);
//         glDrawElements(GL_POINTS, currentProg->getSize("indices"), GL_UNSIGNED_INT, reinterpret_cast<GLvoid *>(cloudOffset));
//         currentProg->endRender();
//     }
//     q_TotalRot.normalize();
// }

// void VKWindow::resizeGL(int w, int h) {
//     gw_scrHeight = height();
//     gw_scrWidth = width();
//     // m4_proj = glm::mat4(1.0f);
//     m4_proj = glm::perspective(RADN(45.0f), GLfloat(w) / h, gw_nearDist, gw_farDist);
// }

void VKWindow::wheelEvent(QWheelEvent *e) {
    int scrollClicks = e->angleDelta().y() / -120;
    float scrollScale = 1.0f + ((float) scrollClicks / 6);
    v3_cameraPosition = scrollScale * v3_cameraPosition;
    
    gw_info.pos = v3_cameraPosition.z;
    gw_info.far = v3_cameraPosition.z + gw_info.start;
    m4_proj = glm::perspective(RADN(45.0f), GLfloat(width()) / height(), gw_nearDist, gw_info.far);
    emit detailsChanged(&gw_info);
    update();
}

void VKWindow::mousePressEvent(QMouseEvent *e) {
    glm::vec3 mouseVec = glm::vec3(e->pos().x(), height() - e->pos().y(), v3_cameraPosition.z);
    v3_mouseBegin = mouseVec;
    v3_mouseEnd = mouseVec;

    if (!gw_movement && (e->button() & (Qt::LeftButton | Qt::RightButton | Qt::MiddleButton)))
        gw_movement |= e->button();
    else
        QWidget::mousePressEvent(e);
}

void VKWindow::mouseMoveEvent(QMouseEvent *e) {
    glm::vec3 mouseVec = glm::vec3(e->pos().x(), gw_scrHeight - e->pos().y(), v3_cameraPosition.z);
    glm::vec3 cameraVec = v3_cameraPosition - v3_cameraTarget;
    v3_mouseBegin = v3_mouseEnd;
    v3_mouseEnd = mouseVec;

    //GLfloat currentAngle = atanf(cameraVec.y / hypot(cameraVec.x, cameraVec.z));

    if (gw_movement & Qt::RightButton) {
        /* Right-click-drag HORIZONTAL movement will rotate about Y axis */
        if (v3_mouseBegin.x != v3_mouseEnd.x) {
            float dragRatio = (v3_mouseEnd.x - v3_mouseBegin.x) / gw_scrWidth;
            GLfloat waveAngleH = TWO_PI * dragRatio;
            glm::vec3 waveAxisH = glm::vec3(0.0, 1.0f, 0.0);
            Quaternion qWaveRotH = Quaternion(waveAngleH, waveAxisH, RAD);
            q_TotalRot = qWaveRotH * q_TotalRot;
        }
        /* Right-click-drag VERTICAL movement will rotate about X and Z axes */
        if (v3_mouseBegin.y != v3_mouseEnd.y) {
            float dragRatio = (v3_mouseBegin.y - v3_mouseEnd.y) / gw_scrHeight;
            GLfloat waveAngleV = TWO_PI * dragRatio;
            glm::vec3 cameraUnit = glm::normalize(glm::vec3(cameraVec.x, 0.0f, cameraVec.z));
            glm::vec3 waveAxisV = glm::vec3(cameraUnit.z, 0.0f, -cameraUnit.x);
            Quaternion qWaveRotV = Quaternion(waveAngleV, waveAxisV, RAD);
            q_TotalRot = qWaveRotV * q_TotalRot;
        }
    } else if (gw_movement & Qt::LeftButton) {
        /* Left-click drag will grab and slide world */
        if (v3_mouseBegin != v3_mouseEnd) {
            glm::vec3 deltaSlide = 0.02f * (v3_mouseEnd - v3_mouseBegin);
            glm::vec3 cameraSlide = (cameraVec.z / 25.0f) * glm::vec3(deltaSlide.x, deltaSlide.y, 0.0f);
            m4_translation = glm::translate(m4_translation, cameraSlide);
        }

    } else if (gw_movement & Qt::MiddleButton) {
        /* Middle-click-drag will rotate about camera look vector */
        if (v3_mouseBegin.x != v3_mouseEnd.x) {
            float dragRatio = (v3_mouseBegin.x - v3_mouseEnd.x) / gw_scrWidth;
            GLfloat waveAngleL = TWO_PI * dragRatio;
            glm::vec3 waveAxisL = glm::normalize(cameraVec);
            Quaternion qWaveRotL = Quaternion(waveAngleL, waveAxisL, RAD);
            q_TotalRot = qWaveRotL * q_TotalRot;
        }
    }
    update();
}

void VKWindow::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() & (Qt::RightButton | Qt::LeftButton | Qt::MiddleButton))
        gw_movement = false;
    else
        QWidget::mouseReleaseEvent(e);
}

void VKWindow::keyPressEvent(QKeyEvent * e) {
    if (e->key() == Qt::Key_Home) {
        initVecsAndMatrices();
        update();
    } else if (e->key() == Qt::Key_Space) {
        gw_pause = !gw_pause;
        if (gw_pause) {
            gw_timePaused = QDateTime::currentMSecsSinceEpoch();
        } else {
            gw_timeEnd = QDateTime::currentMSecsSinceEpoch();
            gw_timeStart += gw_timeEnd - gw_timePaused;
        }
        update();
    } else {
        QWidget::keyPressEvent(e);
    }
}

void VKWindow::checkErrors(std::string str) {
    GLenum err;
    int messages = 0;

    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "\n" << str << std::hex << err;
        messages++;
    }

    if (messages)
        std::cout << std::endl;
}

void VKWindow::setColorsWaves(int id, uint colorChoice) {
    switch (id) {
    case 1:
        waveManager->peak = colorChoice;
        break;
    case 2:
        waveManager->base = colorChoice;
        break;
    case 3:
        waveManager->trough = colorChoice;
        break;
    default:
        break;
    }
    flGraphState.set(egs::UPD_UNI_COLOUR | egs::UPDATE_REQUIRED);
}

void VKWindow::updateBuffersAndShaders() {
    /* Set up ProgramVK with buffers for the first time */
    if (!currentProg || !currentProg->hasBuffer("vertices")) {
        (flGraphState.hasAny(egs::CLOUD_MODE)) ? initCloudProgram() : initWaveProgram();
        initVecsAndMatrices();
    } else {
        uint flags = currentManager->clearUpdates(); // TODO Broken
        flGraphState.set(flags);
    }
    this->updateSize();

    /* Continue with ProgramVK update */
    assert(flGraphState.hasAny(egs::WAVE_RENDER | egs::CLOUD_RENDER));
    uint static_dynamic = currentManager->isCPU() ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    
    /* Bind */
    currentProg->beginRender();

    /* Shaders */
    if (flGraphState.hasAny(egs::UPD_SHAD_V | egs::UPD_SHAD_F)) {
        // TODO needed? - Detach current shaders
        currentProg->detachShaders();

        // Attach shaders, link/validate program, and clean up
        currentProg->attachShader(currentManager->getShaderVert());
        currentProg->attachShader(currentManager->getShaderFrag());
        currentProg->linkAndValidate();
        currentProg->detachShaders();
    }

    /* VBO 1: Vertices */
    if (flGraphState.hasAny(egs::UPD_VBO)) {
        if (currentManager->getVertexCount() > currentProg->getSize("vertices")) {
            // std::cout << "Resizing VBO: Vertices from " << currentProg->getSize("vertices") << " to " << currentManager->getVertexCount() << std::endl;
            currentProg->resizeVBONamed("vertices", currentManager->getVertexCount(), currentManager->getVertexSize(), currentManager->getVertexData(), static_dynamic);
        } else {
            // std::cout << "Updating VBO" << std::endl;
            currentProg->updateVBONamed("vertices", currentManager->getVertexCount(), 0, currentManager->getVertexSize(), currentManager->getVertexData());
        }
    }

    /* VBO 2: Data */
    if (flGraphState.hasAny(egs::UPD_DATA)) {
        if (currentManager->getDataCount() > currentProg->getSize("pdvs")) {
            // std::cout << "Resizing VBO: Datas from " << currentProg->getSize("pdvs") << " to " << currentManager->getDataCount() << std::endl;
            currentProg->resizeVBONamed("pdvs", currentManager->getDataCount(), currentManager->getDataSize(), currentManager->getDataData(), static_dynamic);
        } else {
            // std::cout << "Updating VBO: Datas" << std::endl;
            currentProg->updateVBONamed("pdvs", currentManager->getDataCount(), 0, currentManager->getDataSize(), currentManager->getDataData());
        }
    }

    /* EBO: Indices */
    if (flGraphState.hasAny(egs::UPD_EBO)) {
        if (currentManager->getIndexCount() > currentProg->getSize("indices")) {
            // std::cout << "Resizing EBO from " << currentProg->getSize("indices") << " to " << currentManager->getIndexCount() << std::endl;
            currentProg->resizeEBONamed("indices", currentManager->getIndexCount(), currentManager->getIndexSize(), currentManager->getIndexData(), static_dynamic);
        } else {
            // std::cout << "Updating EBO" << std::endl;
            currentProg->updateEBONamed("indices", currentManager->getIndexCount(), 0, currentManager->getIndexSize(), currentManager->getIndexData());
        }
    }

    /* Uniforms */
    if (flGraphState.hasAny(egs::UPD_UNI_MATHS)) {
        waveProg->setUniform(GL_FLOAT, "two_pi_L", waveManager->two_pi_L);
        waveProg->setUniform(GL_FLOAT, "two_pi_T", waveManager->two_pi_T);
        waveProg->setUniform(GL_FLOAT, "amp", waveManager->waveAmplitude);
        flGraphState.clear(egs::UPD_UNI_MATHS);
    }
    if (flGraphState.hasAny(egs::UPD_UNI_COLOUR)) {
        waveProg->setUniform(GL_UNSIGNED_INT, "peak", waveManager->peak);
        waveProg->setUniform(GL_UNSIGNED_INT, "base", waveManager->base);
        waveProg->setUniform(GL_UNSIGNED_INT, "trough", waveManager->trough);
        flGraphState.clear(egs::UPD_UNI_COLOUR);
    }

    /* Release */
    currentProg->endRender();
    currentProg->clearBuffers();

    if (flGraphState.hasAny(egs::UPD_MATRICES)) {
        initVecsAndMatrices();
    }

    flGraphState.clear(eUpdateFlags);
}

void VKWindow::setBGColour(float colour) {
    gw_bg = colour;
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
    gw_info.vertex = 0;
    gw_info.data = 0;
    gw_info.index = 0;

    if (flGraphState.hasAny(egs::WAVE_RENDER | egs::CLOUD_RENDER)) {
        VSize = currentManager->getVertexSize();            // (count)   * (3 floats) * (4 B/float) * (1 vector)  -- only allVertices
        ISize = currentManager->getIndexSize() * 3;         // (count/2) * (1 uint)   * (4 B/uint)  * (3 vectors) -- idxTolerance + idxSlider + allIndices [very rough estimate]}
        if (flGraphState.hasAny(egs::CLOUD_RENDER)) {
            DSize = currentManager->getDataSize();          // (count)   * (1 float)  * (4 B/float) * (1 vectors) -- only allData [already clear()ing dataStaging; might delete it]
        }
    }

    gw_info.vertex = VSize;
    gw_info.data = DSize;
    gw_info.index = ISize;

    emit detailsChanged(&gw_info);
}

void VKWindow::printSize() {
    updateSize();
    
    std::array<double, 4> bufs = { static_cast<double>(gw_info.vertex), static_cast<double>(gw_info.data), static_cast<double>(gw_info.index), 0 };
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

static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

VKRenderer::VKRenderer(VKWindow *vkWin)
    : vr_vkw(vkWin) {
}

VKRenderer::~VKRenderer() {
    // TODO Destructor
}

void VKRenderer::initResources() {

    // Define instance, device, and function pointers
    vr_dev = vr_vkw->device();
    vr_phydev = vr_vkw->physicalDevice();
    vr_vi = vr_vkw->vulkanInstance();
    vr_vdf = vr_vi->deviceFunctions(vr_dev);
    vr_vf = vr_vi->functions();

    // Retrieve physical device constraints
    const int concFrameCount = vr_vkw->concurrentFrameCount();
    const VkPhysicalDeviceLimits *pdLimits = &vr_vkw->physicalDeviceProperties()->limits;
    const VkDeviceSize uniAlignment = pdLimits->minUniformBufferOffsetAlignment;
    // const VkDeviceSize vertexAllocSize = aligned(sizeof(vertexData), uniAlignment);  // Example of using aligned() -- not used

    // Create Program
    AtomixDevice *progDev = new AtomixDevice();
    progDev->window = vr_vkw;
    progDev->physicalDevice = vr_phydev;
    progDev->device = vr_dev;
    this->vr_vkw->populatePrograms(progDev);
}

QueueFamilyIndices VKRenderer::findQueueFamilies(VkPhysicalDevice device) {
    /* QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    this->vr_vf->vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    this->vr_vf->vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            break;
        }
        i++;
    }

    return indices; */
}

SwapChainSupportInfo VKRenderer::querySwapChainSupport(VkPhysicalDevice device) {
    /* SwapChainSupportInfo info;
    
    this->vr_vf->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, this->vr_vkw->vw_surface, &info.capabilities);
    
    VKuint formatCount = 0;
    this->vr_vkw->vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->vr_vkw->vw_surface, &formatCount, nullptr);
    if (formatCount) {
        info.formats.resize(formatCount);
        this->vr_vkw->vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->vr_vkw->vw_surface, &formatCount, info.formats.data());
    }

    VKuint presentModeCount = 0;
    this->vr_vkw->vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->vr_vkw->vw_surface, &presentModeCount, nullptr);
    if (presentModeCount) {
        info.presentModes.resize(presentModeCount);
        this->vr_vkw->vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->vr_vkw->vw_surface, &presentModeCount, info.presentModes.data());
    }
    
    return info; */
}

void VKRenderer::initSwapChainResources() {
    /* QSize swapChainImageSize = this->vr_vkw->swapChainImageSize();
    VkExtent2D extent = {swapChainImageSize.width(), swapChainImageSize.height()};
    this->vr_vkw->vw_surface = QVulkanInstance::surfaceForWindow(this->vr_vkw);
    SwapChainSupportInfo swapChainSupport = querySwapChainSupport(this->vr_phydev);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = this->vr_vkw->vw_surface;
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

    this->vr_vkw->setSwapChain(swapchain); */
    QVulkanWindowRenderer::initSwapChainResources();

    vm4_proj = vr_vkw->clipCorrectionMatrix();
    const QSize vkwSize = vr_vkw->swapChainImageSize();
    vm4_proj.perspective(45.0f, (float)vkwSize.width() / (float)vkwSize.height(), 0.1f, 100.0f);
    // m4_proj = glm::mat4(vm4_proj.transposed().constData());
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
    if (vr_pipeline) {
        vr_vdf->vkDestroyPipeline(vr_dev, vr_pipeline, nullptr);
    }
    if (vr_pipelineLayout) {
        vr_vdf->vkDestroyPipelineLayout(vr_dev, vr_pipelineLayout, nullptr);
    }
    if (vr_pipelineCache) {
        vr_vdf->vkDestroyPipelineCache(vr_dev, vr_pipelineCache, nullptr);
    }
    if (vr_descSetLayout) {
        vr_vdf->vkDestroyDescriptorSetLayout(vr_dev, vr_descSetLayout, nullptr);
    }
    if (vr_descPool) {
        vr_vdf->vkDestroyDescriptorPool(vr_dev, vr_descPool, nullptr);
    }
    if (vr_bufVert) {
        vr_vdf->vkDestroyBuffer(vr_dev, vr_bufVert, nullptr);
    }
    if (vr_bufMemVert) {
        vr_vdf->vkFreeMemory(vr_dev, vr_bufMemVert, nullptr);
    }
}

void VKRenderer::startNextFrame() {
    VkResult err;
    VkCommandBuffer commandBuffer = vr_vkw->currentCommandBuffer();
    const QSize vkwSize = vr_vkw->swapChainImageSize();
    VkExtent2D renderExtent = {vkwSize.width(), vkwSize.height()};

    // Call Program to render
}
