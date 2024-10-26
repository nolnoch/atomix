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
    // setFocusPolicy(Qt::StrongFocus);
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
    return new VKRenderer(this);
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

    float zero, peak, edge, back, forX, forZ, root;
    edge = 0.3f;  // <-- Change this to scale diamond
    peak = edge;
    zero = 0.0f;
    root = sqrt(3);
    back = root / 3 * edge;
    forZ = root / 6 * edge;
    forX = edge / 2;

    const std::array<GLfloat, 30> vertices = {
              //Vertex              //Colour
          zero,  peak,  zero,   0.6f, 0.6f, 0.6f,    //top
         -forX,  zero,  forZ,   0.1f, 0.4f, 0.4f,    //left - cyan
          forX,  zero,  forZ,   0.4f, 0.1f, 0.4f,    //right - magenta
          zero,  zero, -back,   0.4f, 0.4f, 0.1f,    //back - yellow
          zero, -peak,  zero,   0.0f, 0.0f, 0.0f     //bottom
    };

    const std::array<GLuint, 18> indices = {
        0, 1, 2,
        2, 3, 0,
        3, 1, 0,
        1, 2, 4,
        2, 3, 4,
        3, 4, 1
    };
    this->gw_faces = indices.size();

    // vVec3 crystalRingVertices;
    // uvec crystalRingIndices;
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

    /* ProgramVK */
    crystalProg = new ProgramVK(vrend->vdf);
    crystalProg->addShader(vertName, GL_VERTEX_SHADER);
    crystalProg->addShader(fragName, GL_FRAGMENT_SHADER);
    crystalProg->init();
    crystalProg->attachShader(vertName);
    crystalProg->attachShader(fragName);
    crystalProg->linkAndValidate();
    crystalProg->detachDelete();
    crystalProg->initVAO();
    crystalProg->bindVAO();
    GLuint vboID = crystalProg->bindVBO("vertices", crystalRingVertices.size(), sizeof(float) * crystalRingVertices.size(), &crystalRingVertices[0], GL_STATIC_DRAW);
    crystalProg->setAttributeBuffer(0, vboID, 6 * sizeof(GLfloat));
    crystalProg->enableAttribute(0);
    crystalProg->setAttributePointerFormat(0, 0, 3, GL_FLOAT, 0, 0);                       // Vertices
    crystalProg->enableAttribute(1);
    crystalProg->setAttributePointerFormat(1, 0, 3, GL_FLOAT, 3 * sizeof(GLfloat), 0);     // Colours
    crystalProg->bindEBO("indices", crystalRingIndices.size(), sizeof(uint) * crystalRingIndices.size(), &crystalRingIndices[0], GL_STATIC_DRAW);

    /* Release */
    crystalProg->endRender();
    crystalProg->clearBuffers();
}

/* void VKWindow::initAtomixProg() {
    // TODO Consolidate?!
} */

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
    m4_proj = glm::perspective(RADN(45.0f), GLfloat(width()) / height(), gw_nearDist, gw_farDist);

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
    : vkw(vkWin) {
}

VKRenderer::~VKRenderer() {
    // TODO Destructor
}

void VKRenderer::initResources() {
    VkResult err;
    float vertexData[] = { 0.0f, 0.0f };

    // Define instance, device, and function pointers
    dev = vkw->device();
    VkPhysicalDevice pdev = vkw->physicalDevice();
    vi = vkw->vulkanInstance();
    vdf = vi->deviceFunctions(dev);
    vf = vi->functions();

    // Retrieve physical device constraints
    const int concFrameCount = vkw->concurrentFrameCount();
    const VkPhysicalDeviceLimits *pdLimits = &vkw->physicalDeviceProperties()->limits;
    const VkDeviceSize uniAlignment = pdLimits->minUniformBufferOffsetAlignment;

    // Assign vertex buffer and uniforms
    VkBufferCreateInfo bufCreateInfo;
    memset(&bufCreateInfo, 0, sizeof(bufCreateInfo));
    bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    const VkDeviceSize vertexAllocSize = aligned(sizeof(vertexData), uniAlignment);
    const VkDeviceSize uniformAllocSize = aligned(sizeof(float) * 16, uniAlignment);
    bufCreateInfo.size = vertexAllocSize + concFrameCount * uniformAllocSize;
    bufCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    // Create vertex buffer
    err = vdf->vkCreateBuffer(dev, &bufCreateInfo, nullptr, &vr_bufVert);
    if (err != VK_SUCCESS) {
        qFatal("Failed to create vertex buffer: %d", err);
    }

    // Get memory requirements for buffer vr_buf
    VkMemoryRequirements memReq;
    vdf->vkGetBufferMemoryRequirements(dev, vr_bufVert, &memReq);
    VkMemoryAllocateInfo memAllocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memReq.size,
        vkw->hostVisibleMemoryIndex()
    };

    // Allocate, bind, and map memory for buffer vr_buf
    err = vdf->vkAllocateMemory(dev, &memAllocInfo, nullptr, &vr_bufMemVert);
    if (err != VK_SUCCESS) {
        qFatal("Failed to allocate memory for vr_buf: %d", err);
    }
    err = vdf->vkBindBufferMemory(dev, vr_bufVert, vr_bufMemVert, 0);
    if (err != VK_SUCCESS) {
        qFatal("Failed to bind buffer memory: %d", err);
    }
    uint8_t *p;
    err = vdf->vkMapMemory(dev, vr_bufMemVert, 0, memReq.size, 0, reinterpret_cast<void **>(&p));
    if (err != VK_SUCCESS) {
        qFatal("Failed to map buffer memory: %d", err);
    }

    // Copy data to mapped memory, then unmap
    memcpy(p, vertexData, sizeof(vertexData));
    glm::mat4 ident;
    memset(vr_uniformBufInfo, 0, sizeof(vr_uniformBufInfo));
    for (int i = 0; i < concFrameCount; i++) {
        const VkDeviceSize offset = vertexAllocSize + (i * uniformAllocSize);
        memcpy(p + offset, glm::value_ptr(ident), 16 * sizeof(float));
        vr_uniformBufInfo[i].buffer = vr_bufVert;
        vr_uniformBufInfo[i].offset = offset;
        vr_uniformBufInfo[i].range = uniformAllocSize;
    }
    vdf->vkUnmapMemory(dev, vr_bufMemVert);

    // Describe vertex data
    VkVertexInputBindingDescription vertexBindingDesc{};
    memset(&vertexBindingDesc, 0, sizeof(vertexBindingDesc));
    vertexBindingDesc.binding = 0;
    vertexBindingDesc.stride = 5 * sizeof(float);
    vertexBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    // Describe vertex attributes
    VkVertexInputAttributeDescription vertexAttrDesc[2]{};
    memset(vertexAttrDesc, 0, sizeof(vertexAttrDesc));
    // Position
    vertexAttrDesc[0].location = 0;
    vertexAttrDesc[0].binding = 0;
    vertexAttrDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
    vertexAttrDesc[0].offset = 0;
    // Colour
    vertexAttrDesc[1].location = 1;
    vertexAttrDesc[1].binding = 0;
    vertexAttrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttrDesc[1].offset = 2 * sizeof(float);

    // Describe Pipeline using above structs (which define vertex data and attributes)
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    memset(&vertexInputInfo, 0, sizeof(vertexInputInfo));
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttrDesc;

    // Define descriptor pool with layout
    VkDescriptorPoolSize descPoolSizes = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uint32_t(concFrameCount) };
    VkDescriptorPoolCreateInfo descPoolInfo;
    memset(&descPoolInfo, 0, sizeof(descPoolInfo));
    descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolInfo.maxSets = concFrameCount;
    descPoolInfo.poolSizeCount = 1;
    descPoolInfo.pPoolSizes = &descPoolSizes;

    // Create descriptor pool
    err = vdf->vkCreateDescriptorPool(dev, &descPoolInfo, nullptr, &vr_descPool);
    if (err != VK_SUCCESS) {
        qFatal("Failed to create descriptor pool: %d", err);
    }

    // Define descriptor set layouts for binding and creation
    VkDescriptorSetLayoutBinding layoutBinding{};
    memset(&layoutBinding, 0, sizeof(layoutBinding));
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descLayoutInfo{};
    memset(&descLayoutInfo, 0, sizeof(descLayoutInfo));
    descLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descLayoutInfo.pNext = nullptr;
    descLayoutInfo.flags = 0;
    descLayoutInfo.bindingCount = 1;
    descLayoutInfo.pBindings = &layoutBinding;

    // Create descriptor set
    err = vdf->vkCreateDescriptorSetLayout(dev, &descLayoutInfo, nullptr, &vr_descSetLayout);
    if (err != VK_SUCCESS) {
        qFatal("Failed to create descriptor set layout: %d", err);
    }

    // Define allocation info for descriptor sets
    VkDescriptorSetAllocateInfo descSetAllocInfo{};
    memset(&descSetAllocInfo, 0, sizeof(descSetAllocInfo));
    descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descSetAllocInfo.pNext = nullptr;
    descSetAllocInfo.descriptorPool = vr_descPool;
    descSetAllocInfo.descriptorSetCount = concFrameCount;
    descSetAllocInfo.pSetLayouts = &vr_descSetLayout;

    // For each possible conccurrent frame...
    for (int i = 0; i < concFrameCount; ++i) {
        // Allocate descriptor set
        err = vdf->vkAllocateDescriptorSets(dev, &descSetAllocInfo, &vr_descSet[i]);
        if (err != VK_SUCCESS) {
            qFatal("Failed to allocate descriptor set: %d", err);
        }

        // Fill descriptor set
        VkWriteDescriptorSet descWrite;
        memset(&descWrite, 0, sizeof(descWrite));
        descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descWrite.dstSet = vr_descSet[i];
        descWrite.descriptorCount = 1;
        descWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descWrite.pBufferInfo = &vr_uniformBufInfo[i];
        vdf->vkUpdateDescriptorSets(dev, 1, &descWrite, 0, nullptr);
    }

    // Describe pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheInfo;
    memset(&pipelineCacheInfo, 0, sizeof(pipelineCacheInfo));
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCacheInfo.pNext = nullptr;
    pipelineCacheInfo.flags = 0;
    pipelineCacheInfo.initialDataSize = 0;

    // Create pipeline cache
    err = vdf->vkCreatePipelineCache(dev, &pipelineCacheInfo, nullptr, &vr_pipelineCache);
    if (err != VK_SUCCESS) {
        qFatal("Failed to create pipeline cache: %d", err);
    }

    // Describe pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pNext = nullptr;
    pipelineLayoutInfo.flags = 0;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &vr_descSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    // Create pipeline layout
    err = vdf->vkCreatePipelineLayout(dev, &pipelineLayoutInfo, nullptr, &vr_pipelineLayout);
    if (err != VK_SUCCESS) {
        qFatal("Failed to create pipeline layout: %d", err);
    }

    // Create shader modules
    VkShaderModule vertShaderModule = createShader("vert.spv");
    VkShaderModule fragShaderModule = createShader("frag.spv");

    // Describe shader stage
    VkPipelineShaderStageCreateInfo shaderStages[2];
    // Vertex shader
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].pNext = nullptr;
    shaderStages[0].flags = 0;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[0].pSpecializationInfo = nullptr;
    // Fragment shader
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].pNext = nullptr;
    shaderStages[1].flags = 0;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "main";
    shaderStages[1].pSpecializationInfo = nullptr;

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo ia;
    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Viewport and Scissor
    // The viewport and scissor will be set dynamically via vkCmdSetViewport/Scissor.
    // This way the pipeline does not need to be touched when resizing the window.
    VkPipelineViewportStateCreateInfo vp;
    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rs;
    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE; // we want the back face as well
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo ms;
    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // Enable multisampling.
    ms.rasterizationSamples = vkw->sampleCountFlagBits();

    // Depth Stencil
    VkPipelineDepthStencilStateCreateInfo ds;
    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    // Color Blending
    VkPipelineColorBlendStateCreateInfo cb;
    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // no blend, write out all of rgba
    VkPipelineColorBlendAttachmentState att;
    memset(&att, 0, sizeof(att));
    att.colorWriteMask = 0xF;
    cb.attachmentCount = 1;
    cb.pAttachments = &att;

    // Dynamic State
    VkDynamicState dynEnable[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyn;
    memset(&dyn, 0, sizeof(dyn));
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = sizeof(dynEnable) / sizeof(VkDynamicState);
    dyn.pDynamicStates = dynEnable;

    // Describe pipeline for creation
    VkGraphicsPipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo, 0, sizeof(pipelineInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = 0;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &ia;
    pipelineInfo.pViewportState = &vp;
    pipelineInfo.pRasterizationState = &rs;
    pipelineInfo.pMultisampleState = &ms;
    pipelineInfo.pDepthStencilState = &ds;
    pipelineInfo.pColorBlendState = &cb;
    pipelineInfo.pDynamicState = &dyn;
    pipelineInfo.layout = vr_pipelineLayout;
    pipelineInfo.renderPass = vkw->defaultRenderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    // Create pipeline
    err = vdf->vkCreateGraphicsPipelines(dev, vr_pipelineCache, 1, &pipelineInfo, nullptr, &vr_pipeline);
    if (err != VK_SUCCESS) {
        qFatal("Failed to create graphics pipeline: %d", err);
    }

    if (vertShaderModule) {
        vdf->vkDestroyShaderModule(dev, vertShaderModule, nullptr);
    }
    if (fragShaderModule) {
        vdf->vkDestroyShaderModule(dev, fragShaderModule, nullptr);
    }
}

void VKRenderer::initSwapChainResources() {
    vm4_proj = vkw->clipCorrectionMatrix();
    const QSize vkwSize = vkw->swapChainImageSize();
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
        vdf->vkDestroyPipeline(dev, vr_pipeline, nullptr);
    }
    if (vr_pipelineLayout) {
        vdf->vkDestroyPipelineLayout(dev, vr_pipelineLayout, nullptr);
    }
    if (vr_pipelineCache) {
        vdf->vkDestroyPipelineCache(dev, vr_pipelineCache, nullptr);
    }
    if (vr_descSetLayout) {
        vdf->vkDestroyDescriptorSetLayout(dev, vr_descSetLayout, nullptr);
    }
    if (vr_descPool) {
        vdf->vkDestroyDescriptorPool(dev, vr_descPool, nullptr);
    }
    if (vr_buf) {
        vdf->vkDestroyBuffer(dev, vr_buf, nullptr);
    }
    if (vr_bufMem) {
        vdf->vkFreeMemory(dev, vr_bufMem, nullptr);
    }
}

VkShaderModule VKRenderer::createShader(const std::string &name) {
    std::ifstream file(name, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + name);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string shaderSource = buffer.str();
    file.close();

    VkShaderModuleCreateInfo createInfo = {};
    memset(&createInfo, 0, sizeof(createInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.codeSize = shaderSource.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(shaderSource.c_str());
    
    VkShaderModule shaderModule;
    VkResult err = vdf->vkCreateShaderModule(dev, &createInfo, nullptr, &shaderModule);
    if (err != VK_SUCCESS) {
        qFatal("Failed to create shader module: %d", err);
    }

    return shaderModule;
}

void VKRenderer::startNextFrame() {
    VkResult err;
    VkCommandBuffer commandBuffer = vkw->currentCommandBuffer();
    const QSize vkwSize = vkw->swapChainImageSize();

    VkClearColorValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    VkClearDepthStencilValue clearDepthStencil = {1.0f, 0};
    VkClearValue clearValues[3];
    memset(clearValues, 0, sizeof(clearValues));
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil = clearDepthStencil;
    clearValues[2].color = clearColor;

    VkRenderPassBeginInfo renderPassInfo = {};
    memset(&renderPassInfo, 0, sizeof(renderPassInfo));
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.pNext = nullptr;
    renderPassInfo.renderPass = vkw->defaultRenderPass();
    renderPassInfo.framebuffer = vkw->currentFramebuffer();
    renderPassInfo.renderArea.extent.width = vkwSize.width();
    renderPassInfo.renderArea.extent.height = vkwSize.height();
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.clearValueCount = (vkw->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT) ? 3 : 2;
    renderPassInfo.pClearValues = clearValues;
    vdf->vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    quint8 *p;
    err = vdf->vkMapMemory(dev, vr_bufMem, vr_uniformBufInfo[vkw->currentFrame()].offset, UNIFORM_DATA_SIZE, 0, reinterpret_cast<void **>(&p));
    if (err != VK_SUCCESS) {
        qFatal("Failed to map memory for vertex buffer: %d", err);
    }
    QMatrix4x4 m = vm4_proj;
    m.rotate(0.0f, 0, 1, 0);
    memcpy(p, &m, sizeof(m));
    vdf->vkUnmapMemory(dev, vr_bufMem);

    vdf->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vr_pipeline);
    vdf->vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vr_pipelineLayout, 0, 1, &vr_descSet[vkw->currentFrame()], 0, nullptr);
    VkDeviceSize vr_offset = 0;
    vdf->vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vr_buf, &vr_offset);
    vdf->vkCmdBindIndexBuffer(commandBuffer, vr_buf, 0, VK_INDEX_TYPE_UINT32);

    VkViewport viewport;
    memset(&viewport, 0, sizeof(viewport));
    viewport.width = static_cast<float>(vkwSize.width());
    viewport.height = static_cast<float>(vkwSize.height());
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vdf->vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;
    vdf->vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vdf->vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
    vdf->vkCmdEndRenderPass(commandBuffer);

    vkw->frameReady();
    vkw->requestUpdate();
}
