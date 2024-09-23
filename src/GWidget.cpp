/**
 * GWidget.cpp
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

#include <iostream>
#include <QTimer>
#include "GWidget.hpp"

#define RADN(t) (glm::radians((t)))


GWidget::GWidget(QWidget *parent, ConfigParser *configParser)
    : QOpenGLWidget(parent), cfgParser(configParser) {
    setFocusPolicy(Qt::StrongFocus);
}

GWidget::~GWidget() {
    cleanup();
}

void GWidget::cleanup() {
    //makeCurrent();

    //std::std::cout << "Rendered " << gw_frame << " frames." << std::endl;

    delete crystalProg;
    delete waveProg;
    delete cloudProg;
    delete waveManager;
    delete cloudManager;
    delete cfgParser;
    delete gw_timer;

    //doneCurrent();
}

void GWidget::newCloudConfig() {
    cloudMode = true;
    waveMode = false;
    updateRequired = true;
}

void GWidget::newWaveConfig(AtomixConfig *cfg) {
    setUpdatesEnabled(false);

    if (renderConfig.waves != cfg->waves) {
        renderConfig.waves = cfg->waves;                  // Requires new {Vertices[cpu/gpu], VBO, EBO}
        updateFlags |= ORBITS;
    }
    if (renderConfig.amplitude != cfg->amplitude) {
        renderConfig.amplitude = cfg->amplitude;            // Requires new {Vertices[cpu]}
        updateFlags |= AMPLITUDE;
    }
    if (renderConfig.period != cfg->period) {
        renderConfig.period = cfg->period;                  // Requires new {Vertices[cpu]}
        updateFlags |= PERIOD;
    }
    if (renderConfig.wavelength != cfg->wavelength) {
        renderConfig.wavelength = cfg->wavelength;          // Requires new {Vertices[cpu]}
        updateFlags |= WAVELENGTH;
    }
    if (renderConfig.resolution != cfg->resolution) {
        renderConfig.resolution = cfg->resolution;          // Requires new {Vertices[cpu/gpu], VBO, EBO}
        updateFlags |= RESOLUTION;
    }
    if (renderConfig.parallel != cfg->parallel) {
        renderConfig.parallel = cfg->parallel;              // Requires new {Vertices[cpu]}
        updateFlags |= PARALLEL;
    }
    if (renderConfig.superposition != cfg->superposition) {
        renderConfig.superposition = cfg->superposition;    // Requires new {Vertices[cpu]}
        updateFlags |= SUPERPOSITION;
    }
    if (renderConfig.cpu != cfg->cpu) {
        renderConfig.cpu = cfg->cpu;                        // Requires new {Vertices[cpu/gpu]}
        updateFlags |= CPU;
    }
    if (renderConfig.sphere != cfg->sphere) {
        renderConfig.sphere = cfg->sphere;                  // Requires new {Vertices[cpu/gpu], VBO, EBO]}
        updateFlags |= SPHERE;
    }
    if (renderConfig.vert != cfg->vert) {
        renderConfig.vert = cfg->vert;                      // Requires new {Shader}
        updateFlags |= VERTSHADER;
    }
    if (renderConfig.frag != cfg->frag) {
        renderConfig.frag = cfg->frag;                      // Requires new {Shader}
        updateFlags |= FRAGSHADER;
    }

    updateRequired = true;
    waveMode = true;
    cloudMode = false;
    setUpdatesEnabled(true);
}

void GWidget::selectRenderedWaves(int id, bool checked) {
    // Send selection(s) to WaveManager for regeneration of indices
    renderedWaves = waveManager->selectWaves(id, checked);

    // Flag for EBO update
    newIndices = true;
    updateRequired = true;
}

void GWidget::processConfigChange() {
    //TODO get rid of this debug statement:
    assert(!newVertices);

    // Re-create Waves with new params from config
    if ((updateFlags & (ORBITS | RESOLUTION | SPHERE | CPU)) ||
        (renderConfig.cpu && (updateFlags & (AMPLITUDE | PERIOD | WAVELENGTH | PARALLEL)))) {
        waveManager->newConfig(&renderConfig);
        waveManager->newWaves();
        newVertices = true;
    } else if (!renderConfig.cpu && (updateFlags & (AMPLITUDE | PERIOD | WAVELENGTH))) {
        waveManager->newConfig(&renderConfig);
        newUniformsMaths = true;
    }

    if (updateFlags & (VERTSHADER | FRAGSHADER)) {
        swapShaders();
        newUniformsMaths = true;
        newUniformsColor = true;
    }

    if (updateFlags & (ORBITS | RESOLUTION | SPHERE)) {
        swapBuffers();
    } else if (newVertices) {
        swapVertices();
    } else if (newIndices) {
        swapIndices();
    }

    updateFlags = 0;
    updateRequired = false;
}

void GWidget::swapShaders() {
    // Detach current shaders
    waveProg->detachShaders();

    // Attach shaders, link/validate program, and clean up
    waveProg->attachShader(renderConfig.vert);
    waveProg->attachShader(renderConfig.frag);
    waveProg->linkAndValidate();
    waveProg->detachShaders();
}

void GWidget::swapBuffers() {
    // Load and bind new vertices and attributes
    waveProg->enable();
    waveProg->bindVAO();
    waveProg->clearBuffers();
    uint static_dynamic = renderConfig.cpu ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    GLuint vboID = waveProg->bindVBO(waveManager->getVertexSize(), waveManager->getVertexData(), static_dynamic);
    waveProg->setAttributeBuffer(0, vboID, 6 * sizeof(GLfloat));
    waveProg->bindEBO(waveManager->getIndexSize(), waveManager->getIndexData(), static_dynamic);
    waveProg->endRender();

    // Cleanup
    waveProg->deleteBuffers();
    newVertices = false;
    newIndices = false;
}

void GWidget::swapVertices() {
    waveProg->beginRender();
    waveProg->updateVBO(0, waveManager->getVertexSize(), waveManager->getVertexData());
    waveProg->endRender();

    newVertices = false;
}

void GWidget::swapIndices() {
    waveProg->beginRender();
    waveProg->updateEBO(0, waveManager->getIndexSize(), waveManager->getIndexData());
    waveProg->endRender();

    newIndices = false;
}

void GWidget::initCrystalProgram() {
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

    /* Program */
    crystalProg = new Program(this);
    crystalProg->addShader(vertName, GL_VERTEX_SHADER);
    crystalProg->addShader(fragName, GL_FRAGMENT_SHADER);
    crystalProg->init();
    crystalProg->attachShader(vertName);
    crystalProg->attachShader(fragName);
    crystalProg->linkAndValidate();
    crystalProg->detachDelete();
    crystalProg->initVAO();
    crystalProg->bindVAO();
    GLuint vboID = crystalProg->bindVBO(sizeof(vertices), vertices.data(), GL_STATIC_DRAW);
    crystalProg->setAttributeBuffer(0, vboID, 6 * sizeof(GLfloat));
    crystalProg->enableAttribute(0);
    crystalProg->setAttributePointerFormat(0, 0, 3, GL_FLOAT, 0, 0);                       // Vertices
    crystalProg->enableAttribute(1);
    crystalProg->setAttributePointerFormat(1, 0, 3, GL_FLOAT, 3 * sizeof(GLfloat), 0);     // Colours
    crystalProg->bindEBO(sizeof(indices), indices.data(), GL_STATIC_DRAW);

    /* Release */
    crystalProg->endRender();
    crystalProg->clearBuffers();
}

void GWidget::initAtomixProg() {
    
}

void GWidget::initWaveProgram() {
    /* Waves */
    waveManager = new WaveManager(&renderConfig);

    /* Program */
    // Dynamic Draw for updating vertices per-render (CPU) or Static Draw for one-time load (GPU)
    uint static_dynamic = renderConfig.cpu ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    
    // Create Program
    waveProg = new Program(this);
    
    // Add all shaders
    waveProg->addAllShaders(&cfgParser->vshFiles, GL_VERTEX_SHADER);
    waveProg->addAllShaders(&cfgParser->fshFiles, GL_FRAGMENT_SHADER);
    waveProg->init();
    
    // Attach shaders, link/validate program, and clean up
    waveProg->attachShader(waveManager->getShaderVert());
    waveProg->attachShader(waveManager->getShaderFrag());
    waveProg->linkAndValidate();
    waveProg->detachShaders();
    
    // Load and bind vertices and attributes
    waveProg->initVAO();
    waveProg->bindVAO();
    GLuint vboID = waveProg->bindVBO(waveManager->getVertexSize(), waveManager->getVertexData(), static_dynamic);
    waveProg->setAttributeBuffer(0, vboID, 6 * sizeof(GLfloat));
    waveProg->enableAttribute(0);
    waveProg->setAttributePointerFormat(0, 0, 3, GL_FLOAT, 0, 0);                         // x,y,z coords or factorsA
    waveProg->enableAttribute(1);
    waveProg->setAttributePointerFormat(1, 0, 3, GL_FLOAT, 3 * sizeof(GLfloat), 0);       // r,g,b colour or factorsB
    waveProg->bindEBO(waveManager->getIndexSize(), waveManager->getIndexData(), static_dynamic);
    //waveProg->assignFragColour();

    /* Release */
    waveProg->endRender();
    waveProg->clearBuffers();
    newUniformsMaths = true;
    newUniformsColor = true;
    this->renderWave = true;
    updateRequired = false;

    currentProg = waveProg;
    currentManager = waveManager;
}

void GWidget::initCloudProgram() {
    /* Waves */
    cloudManager = new CloudManager(&renderConfig);

    cloudManager->genOrbitalExplicit(4, 2, 0);
    // cloudManager->genWavealExplicit(7, 1, 0);
    // cloudManager->genWavealExplicit(3, 2, -1);
    // cloudManager->genWavealExplicit(3, 2, -2);
    // cloudManager->genWavealsThroughN(8);
    // cloudManager->genWavealsOfN(3);
    // cloudManager->cloudTest(8);
    cloudManager->bakeOrbitalsForRender();
    this->printSize();

    /* Program */
    // Dynamic Draw for updating vertices per-render (CPU) or Static Draw for one-time load (GPU)
    uint static_dynamic = renderConfig.cpu ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    // Create Program
    cloudProg = new Program(this);

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
    GLuint vboIDa = cloudProg->bindVBO(cloudManager->getVertexSize(), cloudManager->getVertexData(), static_dynamic);
    cloudProg->setAttributeBuffer(0, vboIDa, 3 * sizeof(GLfloat));
    cloudProg->enableAttribute(0);
    cloudProg->setAttributePointerFormat(0, 0, 3, GL_FLOAT, 0, 0);                         // x,y,z coords or factorsA

    /* VBO 2: RDPs */
    GLuint vboIDb = cloudProg->bindVBO(cloudManager->getRDPSize(), cloudManager->getRDPData(), static_dynamic);
    cloudProg->setAttributeBuffer(1, vboIDb, 1 * sizeof(GLfloat));
    cloudProg->enableAttribute(1);
    cloudProg->setAttributePointerFormat(1, 1, 3, GL_FLOAT, 0, 0);                         // r,g,b colour or factorsB

    /* EBO: Indices */
    cloudProg->bindEBO(cloudManager->getIndexSize(), cloudManager->getIndexData(), static_dynamic);

    /* Release */
    cloudProg->endRender();
    cloudProg->clearBuffers();
    this->renderCloud = true;
    updateRequired = false;

    currentProg = cloudProg;
    currentManager = cloudManager;
}

void GWidget::changeModes() {
    delete currentManager;
    delete currentProg;

    if (waveMode) {
        cloudManager = 0;
        cloudProg = 0;
    } else if (cloudMode) {
        waveManager = 0;
        waveProg = 0;
    }

    currentManager = 0;
    currentProg = 0;
}

void GWidget::initVecsAndMatrices() {
    if (!renderCloud) {
        gw_startDist = 16.0f;
        gw_nearDist = 0.1f;
        gw_farDist = 500.0f;
    } else {
        gw_startDist = 80.0f;
        gw_nearDist = 4.0f;
        gw_farDist = 200.0f;
    }
    
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
}

void GWidget::initializeGL() {
    /* Init -- OpenGL Context and Functions */
    if (!context()) {
        gw_context = new QOpenGLContext(this);
        if (!gw_context->create())
            std::cout << "Failed to create OpenGL context" << std::endl;
    } else {
        gw_context = context();
    }
    makeCurrent();
    if (!gw_init) {
        if (!initializeOpenGLFunctions())
            std::cout << "Failed to initialize OpenGL functions" << std::endl;
        else
            gw_init = true;
    }

    /* Init -- Camera and OpenGL State */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    /* Init -- Matrices */
    initVecsAndMatrices();
    

    /* Init -- Programs and Shaders */
    initCrystalProgram();

    /* Init -- Time */
    gw_timeStart = QDateTime::currentMSecsSinceEpoch();
    gw_timer = new QTimer(this);
    connect(gw_timer, &QTimer::timeout, this, QOverload<>::of(&GWidget::update));
    gw_timer->start(33);
}

void GWidget::paintGL() {
    if (!gw_pause)
        gw_timeEnd = QDateTime::currentMSecsSinceEpoch();
    float time = (gw_timeEnd - gw_timeStart) / 1000.0f;

    /* Pre-empt painting for new or updated Wave configuration */
    if (updateRequired) {
        if (waveMode) {
            if (renderWave) {
                processConfigChange();
            } else {
                renderCloud = false;
                changeModes();
                initWaveProgram();
                initVecsAndMatrices();
            }
        } else if (cloudMode) {
            renderWave = false;
            changeModes();
            initCloudProgram();
            initVecsAndMatrices();
        }
    }

    /* Per-frame Setup */
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    /* Re-calculate world state matrices */
    m4_rotation = glm::make_mat4(&q_TotalRot.matrix()[0]);
    m4_world = m4_translation * m4_rotation;
    m4_view = glm::lookAt(v3_cameraPosition, v3_cameraTarget, v3_cameraUp);

    /* Render -- Crystal */
    crystalProg->beginRender();
    crystalProg->setUniformMatrix(4, "worldMat", glm::value_ptr(m4_world));
    crystalProg->setUniformMatrix(4, "viewMat", glm::value_ptr(m4_view));
    crystalProg->setUniformMatrix(4, "projMat", glm::value_ptr(m4_proj));
    glDrawElements(GL_TRIANGLES, gw_faces, GL_UNSIGNED_INT, 0);
    crystalProg->endRender();

    /* Render -- Waves */
    if (renderCloud || renderWave) {
        currentProg->beginRender();
        if (renderWave && renderConfig.cpu) {
            cloudManager->updateCloud(time);
            waveProg->updateVBO(0, cloudManager->getVertexSize(), cloudManager->getVertexData());
        }
        if (renderWave && newUniformsMaths) {
            waveProg->setUniform(GL_FLOAT, "two_pi_L", waveManager->two_pi_L);
            waveProg->setUniform(GL_FLOAT, "two_pi_T", waveManager->two_pi_T);
            waveProg->setUniform(GL_FLOAT, "amp", waveManager->waveAmplitude);
            newUniformsMaths = false;
        }
        if (renderWave && newUniformsColor) {
            waveProg->setUniform(GL_UNSIGNED_INT, "peak", waveManager->peak);
            waveProg->setUniform(GL_UNSIGNED_INT, "base", waveManager->base);
            waveProg->setUniform(GL_UNSIGNED_INT, "trough", waveManager->trough);
            newUniformsColor = false;
        }
        currentProg->setUniformMatrix(4, "worldMat", glm::value_ptr(m4_world));
        currentProg->setUniformMatrix(4, "viewMat", glm::value_ptr(m4_view));
        currentProg->setUniformMatrix(4, "projMat", glm::value_ptr(m4_proj));
        currentProg->setUniform(GL_FLOAT, "time", time);
        glDrawElements(GL_POINTS, currentManager->getIndexCount(), GL_UNSIGNED_INT, 0);
        currentProg->endRender();
    }

    q_TotalRot.normalize();
}

void GWidget::resizeGL(int w, int h) {
    gw_scrHeight = height();
    gw_scrWidth = width();
    // m4_proj = glm::mat4(1.0f);
    m4_proj = glm::perspective(RADN(45.0f), GLfloat(w) / h, gw_nearDist, gw_farDist);
}

void GWidget::wheelEvent(QWheelEvent *e) {
    int scrollClicks = e->angleDelta().y() / -120;
    float scrollScale = 1.0f + ((float) scrollClicks / 6);
    v3_cameraPosition = scrollScale * v3_cameraPosition;
    update();
}

void GWidget::mousePressEvent(QMouseEvent *e) {
    glm::vec3 mouseVec = glm::vec3(e->pos().x(), height() - e->pos().y(), v3_cameraPosition.z);
    v3_mouseBegin = mouseVec;
    v3_mouseEnd = mouseVec;

    if (!gw_movement && (e->button() & (Qt::LeftButton | Qt::RightButton | Qt::MiddleButton)))
        gw_movement |= e->button();
    else
        QWidget::mousePressEvent(e);
}

void GWidget::mouseMoveEvent(QMouseEvent *e) {
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
            glm::vec3 cameraSlide = glm::vec3(deltaSlide.x, deltaSlide.y, 0.0f);
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

void GWidget::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() & (Qt::RightButton | Qt::LeftButton | Qt::MiddleButton))
        gw_movement = false;
    else
        QWidget::mouseReleaseEvent(e);
}

void GWidget::keyPressEvent(QKeyEvent * e) {
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
    } else
        QWidget::keyPressEvent(e);
}

void GWidget::checkErrors(std::string str) {
    GLenum err;
    int messages = 0;

    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "\n" << str << std::hex << err;
        messages++;
    }

    if (messages)
        std::cout << std::endl;
}

void GWidget::setColorsWaves(int id, uint colorChoice) {
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
    newUniformsColor = true;
}

void GWidget::printSize() {
    int64_t VSize, DSize, ISize;
    VSize = cloudManager->getVertexSize();
    DSize = cloudManager->getRDPSize() * 2;
    ISize = cloudManager->getIndexSize();

    int64_t divisorMB = 1024 * 1024;

    if (ISize > 1048576)
        ISize /= divisorMB;

    std::cout << "\nVertex Total Size: " << std::setw(6) << VSize / divisorMB << " MB\n";
    std::cout << "RDProb Total Size: " << std::setw(6) << DSize / divisorMB << " MB\n";
    std::cout << "Indice Total Size: " << std::setw(6) << ISize  << ((ISize > 1048576) ? " MB" : "  B") << std::endl;
}

void GWidget::printConfig(AtomixConfig *cfg) {
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
