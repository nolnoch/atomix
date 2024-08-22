/**
 * GWidget.cpp
 *
 *    Created on: Oct 3, 2023
 *   Last Update: Oct 16, 2023
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *  Copyright 2023 Wade Burch (GPLv3)
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


GWidget::GWidget(QWidget *parent)
    : QOpenGLWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);

    ap.apProgs.push_back(&firstProgs);
    ap.apProgs.push_back(&secondProgs);
    ap.apOrbits.push_back(&firstOrbits);
    ap.apOrbits.push_back(&secondOrbits);
    ap.apConfigs.push_back(&renderConfig);
    ap.apConfigs.push_back(&createConfig);
}

GWidget::~GWidget() {
    cleanup();
}

void GWidget::cleanup() {
    makeCurrent();

    //std::cout << "Rendered " << gw_frame << " frames." << std::endl;

    delete crystalProg;

    doneCurrent();
}

void GWidget::configReceived(WaveConfig *cfg) {
    cout << "\nConfig received! Changing from: \n";
    printConfig(ap.apConfigs[ap.apIdxCreate % 2]);
    
    ap.apConfigs[ap.apIdxCreate % 2]->orbits = cfg->orbits;
    ap.apConfigs[ap.apIdxCreate % 2]->amplitude = cfg->amplitude;
    ap.apConfigs[ap.apIdxCreate % 2]->period = cfg->period;
    ap.apConfigs[ap.apIdxCreate % 2]->wavelength = cfg->wavelength;
    ap.apConfigs[ap.apIdxCreate % 2]->resolution = cfg->resolution;
    ap.apConfigs[ap.apIdxCreate % 2]->parallel = cfg->parallel;
    ap.apConfigs[ap.apIdxCreate % 2]->superposition = cfg->superposition;
    ap.apConfigs[ap.apIdxCreate % 2]->cpu = cfg->cpu;
    ap.apConfigs[ap.apIdxCreate % 2]->sphere = cfg->sphere;
    ap.apConfigs[ap.apIdxCreate % 2]->shader = cfg->shader;
    ap.apConfigs[ap.apIdxCreate % 2]->frag = cfg->frag;

    cout << "\nTo: \n";
    printConfig(ap.apConfigs[ap.apIdxCreate % 2]);

    setUpdatesEnabled(false);
    initWavePrograms();
    ap.apIdxRender++;
    renderProgs = ap.apProgs[ap.apIdxRender % 2];
    renderOrbits = ap.apOrbits[ap.apIdxRender % 2];
    setUpdatesEnabled(true);
    update();
}

void GWidget::crystalProgram() {
    float zero, peak, edge, back, forX, forZ, root;
    edge = 0.6f;  // <-- Change this to scale diamond
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
    crystalProg->addShader("crystal.vert", GL_VERTEX_SHADER);
    crystalProg->addShader("crystal.frag", GL_FRAGMENT_SHADER);
    crystalProg->init();
    crystalProg->linkAndValidate();
    crystalProg->initVAO();
    crystalProg->bindVAO();
    crystalProg->bindVBO(sizeof(vertices), vertices.data(), GL_STATIC_DRAW);
    crystalProg->attributePointer(0, 3, 6 * sizeof(float), (void*)0);                       // Vertices
    crystalProg->attributePointer(1, 3, 6 * sizeof(float), (void*)(3 * sizeof(float)));     // Colours
    crystalProg->enableAttributes();
    crystalProg->bindEBO(sizeof(indices), indices.data());

    /* Release */
    crystalProg->endRender();
    crystalProg->clearBuffers();
}

void GWidget::initWavePrograms() {
    if (ap.apIdxCreate > 1) {
        clearProgram(ap.apIdxCreate % 2);
    }
    
    for (int i = 1; i <= ap.apConfigs[ap.apIdxCreate % 2]->orbits; i++) {
        waveProgram(i, ap.apProgs[ap.apIdxCreate % 2], ap.apOrbits[ap.apIdxCreate % 2], ap.apConfigs[ap.apIdxCreate % 2]);
    }
    ap.apIdxCreate++;
}

void GWidget::waveProgram(uint i, std::vector<Program *> *vecProgs, std::vector<Orbit *> *vecOrbits, WaveConfig *cfg) {
    int c = i - 1;
    vecOrbits->push_back(new Orbit(cfg, c > 0 ? (*vecOrbits)[c - 1] : 0));

    /* Program */
    uint static_dynamic = cfg->cpu ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    Program *prog = new Program(this);
    vecProgs->push_back(prog);
    prog->addShader(cfg->shader, GL_VERTEX_SHADER);
    prog->addShader(cfg->frag, GL_FRAGMENT_SHADER);
    prog->init();
    prog->linkAndValidate();
    prog->initVAO();
    prog->bindVAO();
    prog->bindVBO(vecOrbits->back()->vertexSize(), vecOrbits->back()->vertexData(), static_dynamic);
    prog->attributePointer(0, 3, 6 * sizeof(float), (void *)0);                         // x,y,z coords or factorsA
    prog->attributePointer(1, 3, 6 * sizeof(float), (void *)(3 * sizeof(float)));       // r,g,b colour or factorsB
    prog->enableAttributes();
    prog->bindEBO(vecOrbits->back()->indexSize(), vecOrbits->back()->indexData());

    /* Release */
    prog->endRender();
    prog->clearBuffers();
}

void GWidget::clearProgram(uint i) {
    for (auto p : (*ap.apProgs[i]))
        delete (p);
    (*ap.apProgs[i]).clear();
    for (auto p : (*ap.apOrbits[i]))
        delete (p);
    (*ap.apOrbits[i]).clear();
}

void GWidget::initVecsAndMatrices() {
    float camStart = ap.apConfigs[ap.apIdxRender % 2]->orbits * 2.0f;

    q_TotalRot.zero();
    m4_rotation = glm::mat4(1.0f);
    m4_translation = glm::mat4(1.0f);
    m4_proj = glm::mat4(1.0f);
    m4_view = glm::mat4(1.0f);
    m4_world = glm::mat4(1.0f);
    v3_cameraPosition = glm::vec3(0.0f, 0.0f, camStart);
    v3_cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    v3_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    v3_mouseBegin = glm::vec3(0);
    v3_mouseEnd = glm::vec3(0);
    v3_mouseBegin = glm::vec3(0);
    v3_mouseEnd = glm::vec3(0);
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
    glClearColor(0.03f, 0.00f, 0.06f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    /* Init -- Matrices */
    initVecsAndMatrices();
    m4_view = glm::lookAt(v3_cameraPosition, v3_cameraTarget, v3_cameraUp);
    m4_proj = glm::perspective(RADN(45.0f), GLfloat(width()) / height(), 0.1f, 100.0f);

    /* Init -- Programs and Shaders */
    crystalProgram();
    initWavePrograms();
    renderProgs = ap.apProgs[ap.apIdxRender % 2];
    renderOrbits = ap.apOrbits[ap.apIdxRender % 2];

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

    /* Per-frame Setup */
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    /* Re-calculate world state matrices */
    m4_rotation = glm::make_mat4(&q_TotalRot.matrix()[0]);
    m4_world = m4_translation * m4_rotation;
    m4_view = glm::lookAt(v3_cameraPosition, v3_cameraTarget, v3_cameraUp);
    m4_proj = glm::perspective(RADN(45.0f), GLfloat(width()) / height(), 0.1f, 100.0f);
    
    /* Render -- Crystal */
    crystalProg->beginRender();
    crystalProg->setUniformMatrix(4, "worldMat", glm::value_ptr(m4_world));
    crystalProg->setUniformMatrix(4, "viewMat", glm::value_ptr(m4_view));
    crystalProg->setUniformMatrix(4, "projMat", glm::value_ptr(m4_proj));
    glDrawElements(GL_TRIANGLES, gw_faces, GL_UNSIGNED_INT, 0);
    crystalProg->endRender();

    /* (CPU) Update -- Waves */
    if (ap.apConfigs[ap.apIdxRender % 2]->cpu) {
        for (int i = 0; i < renderOrbits->size(); i++) {
            (*renderOrbits)[i]->updateOrbit(time);
        }
    }
    
    /* Render -- Waves */
    for (int i = 0; i < renderOrbits->size(); i++) {
        (*renderProgs)[i]->beginRender();
        (*renderProgs)[i]->updateVBO(0, (*renderOrbits)[i]->vertexSize(), (*renderOrbits)[i]->vertexData());
        (*renderProgs)[i]->setUniformMatrix(4, "worldMat", glm::value_ptr(m4_world));
        (*renderProgs)[i]->setUniformMatrix(4, "viewMat", glm::value_ptr(m4_view));
        (*renderProgs)[i]->setUniformMatrix(4, "projMat", glm::value_ptr(m4_proj));
        //(*renderProgs)[i]->setUniform(GL_FLOAT, "two_pi_L", gw_orbits[0]->two_pi_L);
        //(*renderProgs)[i]->setUniform(GL_FLOAT, "two_pi_T", gw_orbits[0]->two_pi_T);
        //(*renderProgs)[i]->setUniform(GL_FLOAT, "amp", gw_orbits[0]->amplitude);
        (*renderProgs)[i]->setUniform(GL_FLOAT, "time", time);
        glDrawElements(GL_POINTS, (*renderOrbits)[i]->indexCount(), GL_UNSIGNED_INT, 0);
        (*renderProgs)[i]->endRender();
    }

    q_TotalRot.normalize();
}

void GWidget::resizeGL(int w, int h) {
    gw_scrHeight = height();
    gw_scrWidth = width();
    m4_proj = glm::mat4(1.0f);
    m4_proj = glm::perspective(RADN(45.0f), GLfloat(w) / h, 0.1f, 100.0f);
}

void GWidget::printConfig(WaveConfig *cfg) {
    cout << "Orbits: " << cfg->orbits << "\n";
    cout << "Amplitude: " << cfg->amplitude << "\n";
    cout << "Period: " << cfg->period << "\n";
    cout << "Wavelength: " << cfg->wavelength << "\n";
    cout << "Resolution: " << cfg->resolution << "\n";
    cout << "Parallel: " << cfg->parallel << "\n";
    cout << "Superposition: " << cfg->superposition << "\n";
    cout << "CPU: " << cfg->cpu << "\n";
    cout << "Sphere: " << cfg->sphere << "\n";
    cout << "Vert Shader: " << cfg->shader << "\n";
    cout << "Frag Shader: " << cfg->frag << endl;
}

void GWidget::wheelEvent(QWheelEvent *e) {
    int scrollClicks = e->angleDelta().y() / -120;
    float scrollScale = 1.0f + ((float) scrollClicks/ 10);
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
        /* Right-click-drag HORIZONTAL movement will orbit about Y axis */
        if (v3_mouseBegin.x != v3_mouseEnd.x) {
            float dragRatio = (v3_mouseEnd.x - v3_mouseBegin.x) / gw_scrWidth;
            GLfloat orbitAngleH = TWO_PI * dragRatio;
            glm::vec3 orbitAxisH = glm::vec3(0.0, 1.0f, 0.0);
            Quaternion qOrbitRotH = Quaternion(orbitAngleH, orbitAxisH, RAD);
            q_TotalRot = qOrbitRotH * q_TotalRot;
        }
        /* Right-click-drag VERTICAL movement will orbit about X and Z axes */
        if (v3_mouseBegin.y != v3_mouseEnd.y) {
            float dragRatio = (v3_mouseBegin.y - v3_mouseEnd.y) / gw_scrHeight;
            GLfloat orbitAngleV = TWO_PI * dragRatio;
            glm::vec3 cameraUnit = glm::normalize(glm::vec3(cameraVec.x, 0.0f, cameraVec.z));
            glm::vec3 orbitAxisV = glm::vec3(cameraUnit.z, 0.0f, -cameraUnit.x);
            Quaternion qOrbitRotV = Quaternion(orbitAngleV, orbitAxisV, RAD);
            q_TotalRot = qOrbitRotV * q_TotalRot;
        }
    } else if (gw_movement & Qt::LeftButton) {
        /* Left-click drag will grab and slide world */
        if (v3_mouseBegin != v3_mouseEnd) {
            glm::vec3 deltaSlide = 0.01f * (v3_mouseEnd - v3_mouseBegin);
            glm::vec3 cameraSlide = glm::vec3(deltaSlide.x, deltaSlide.y, 0.0f);
            m4_translation = glm::translate(m4_translation, cameraSlide);
        }
        
    } else if (gw_movement & Qt::MiddleButton) {
        /* Middle-click-drag will orbit about camera look vector */
        if (v3_mouseBegin.x != v3_mouseEnd.x) {
            float dragRatio = (v3_mouseBegin.x - v3_mouseEnd.x) / gw_scrWidth;
            GLfloat orbitAngleL = TWO_PI * dragRatio;
            glm::vec3 orbitAxisL = glm::normalize(cameraVec);
            Quaternion qOrbitRotL = Quaternion(orbitAngleL, orbitAxisL, RAD);
            q_TotalRot = qOrbitRotL * q_TotalRot;
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
