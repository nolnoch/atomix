/**
 * GWidget.cpp
 *
 *    Created on: Oct 3, 2023
 *   Last Update: Oct 14, 2023
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

void GWidget::crystalProgram() {
    float zero, peak, edge, back, forX, forZ, root;
    edge = 0.6f;  // <-- Change this to scale diamond
    peak = edge * 1.2;
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
    crystalProg->bindVBO(sizeof(vertices), vertices.data());
    crystalProg->attributePointer(0, 3, 6 * sizeof(float), (void*)0);                       // Vertices
    crystalProg->attributePointer(1, 3, 6 * sizeof(float), (void*)(3 * sizeof(float)));     // Colours
    crystalProg->enableAttributes();
    crystalProg->bindEBO(sizeof(indices), indices.data());

    /* Release */
    crystalProg->endRender();
    crystalProg->clearBuffers();
}

void GWidget::waveProgram(uint r) {
    float deg = 360 / STEPS;
    double two_pi_L_r, two_pi_T;
    two_pi_L_r = TWO_PI / L * r;
    two_pi_T = TWO_PI / T;

    std::vector<GLfloat> vertices;
    for (int i = 0; i < STEPS; i++) {
        double theta = i * deg * RAD_FAC;
        
        GLfloat x = r * cos(theta);
        GLfloat z = r * sin(theta);
        
        vertices.push_back(x);
        vertices.push_back(0.0f);
        vertices.push_back(z);

        /* y = A * sin((two_pi_L_r * theta) - (two_pi_T * t)) */
        vertices.push_back(A);
        vertices.push_back(two_pi_L_r * theta);
        vertices.push_back(two_pi_T);
    }

    /* EBO Indices */
    std::vector<GLuint> indices(STEPS);
    std::iota(std::begin(indices), std::end(indices), 0);
    this->gw_points = indices.size();

    /* Program */
    Program *prog = new Program(this);
    waveProgs.push_back(prog);
    prog->addShader("wave.vert", GL_VERTEX_SHADER);
    prog->addShader("wave.frag", GL_FRAGMENT_SHADER);
    prog->init();
    prog->linkAndValidate();
    prog->initVAO();
    prog->bindVAO();
    
    prog->bindVBO((vertices.size() * sizeof(GLfloat)), vertices.data());
    prog->attributePointer(0, 3, 6 * sizeof(float), (void *)0);                         // x,z coords
    prog->attributePointer(1, 3, 6 * sizeof(float), (void *)(3 * sizeof(float)));       // y-coord factors
    prog->enableAttributes();
    
    prog->bindEBO((indices.size() * sizeof(GLuint)), indices.data());

    /* Release */
    prog->endRender();
    prog->clearBuffers();
}

void GWidget::initVecsAndMatrices() {
    q_TotalRot.zero();
    m4_rotation = glm::mat4(1.0f);
    m4_translation = glm::mat4(1.0f);
    m4_proj = glm::mat4(1.0f);
    m4_view = glm::mat4(1.0f);
    m4_world = glm::mat4(1.0f);
    v3_cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f);
    v3_cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    v3_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    v3_slideBegin = glm::vec3(0);
    v3_slideEnd = glm::vec3(0);
    v3_orbitBegin = glm::vec3(0);
    v3_orbitEnd = glm::vec3(0);
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
    glClearColor(0.0f, 0.05f, 0.08f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    /* Init -- Matrices */
    initVecsAndMatrices();
    m4_view = glm::lookAt(v3_cameraPosition, v3_cameraTarget, v3_cameraUp);
    m4_proj = glm::perspective(RADN(45.0f), GLfloat(width()) / height(), 0.1f, 100.0f);

    /* Init -- Programs and Shaders */
    crystalProgram();
    for (int i = 1; i <= WAVES; i++) {
        waveProgram((float) i);
    }

    /* Init -- Timer */
    //gw_timer = new QTimer(this);
    //connect(gw_timer, &QTimer::timeout, this, &GWidget::updateWaves);
    //gw_timer->start(33);
}

void GWidget::paintGL() {
    /* Per-frame Setup */
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

    /* Render -- Waves */
    for (int i = 0; i < waveProgs.size(); i++) {
    waveProgs[i]->beginRender();
    waveProgs[i]->setUniformMatrix(4, "worldMat", glm::value_ptr(m4_world));
    waveProgs[i]->setUniformMatrix(4, "viewMat", glm::value_ptr(m4_view));
    waveProgs[i]->setUniformMatrix(4, "projMat", glm::value_ptr(m4_proj));
    waveProgs[i]->setUniform(GL_FLOAT, "time", gw_time);
    glDrawElements(GL_LINE_LOOP, gw_points, GL_UNSIGNED_INT, 0);
    waveProgs[i]->endRender();
    }

    q_TotalRot.normalize();
}

void GWidget::resizeGL(int w, int h) {
    m4_proj = glm::mat4(1.0f);
    m4_proj = glm::perspective(RADN(45.0f), GLfloat(w) / h, 0.1f, 100.0f);
}

void GWidget::wheelEvent(QWheelEvent *e) {
    int scrollClicks = e->angleDelta().y() / -120;
    float scrollScale = 1.0f + ((float) scrollClicks/ 10);
    v3_cameraPosition = scrollScale * v3_cameraPosition;
    update();
}

void GWidget::mousePressEvent(QMouseEvent *e) {
    int x, y;
    x = e->pos().x();
    y = e->pos().y();
    glm::vec3 mouseVec = glm::vec3(x, height() - y, v3_cameraPosition.z);

    switch(e->button()) {
        case(Qt::LeftButton):
            v3_slideBegin = mouseVec;
            v3_slideEnd = v3_slideBegin;
            gw_sliding = true;
            break;
        case(Qt::RightButton):
            v3_orbitBegin = mouseVec;
            v3_orbitEnd = v3_orbitBegin;
            gw_orbiting = true;
            break;
        case(Qt::MiddleButton):
            v3_rollBegin = mouseVec;
            v3_rollEnd = v3_rollBegin;
            gw_rolling = true;
            break;
        default:
            QWidget::mousePressEvent(e);
    }
}

/** 
 * The reason for repeating so much of this code with different variables is that,
 * at least in the case of Left and Right mouse buttons, multiple of these may be
 * concurrently true and must be handled in parallel.
*/
void GWidget::mouseMoveEvent(QMouseEvent *e) {
    int x, y, scrHeight, scrWidth;
    scrHeight = height();
    scrWidth = width();
    x = e->pos().x();
    y = e->pos().y();
    glm::vec3 mouseVec = glm::vec3(x, scrHeight - y, v3_cameraPosition.z);
    glm::vec3 cameraVec = v3_cameraPosition - v3_cameraTarget;

    //GLfloat currentAngle = atanf(cameraVec.y / hypot(cameraVec.x, cameraVec.z));

    if (gw_orbiting) {
        v3_orbitBegin = v3_orbitEnd;
        v3_orbitEnd = mouseVec;
        
        /* Right-click-drag horizontal movement will orbit about Y axis */
        if (v3_orbitBegin.x != v3_orbitEnd.x) {
            float dragRatio = (v3_orbitEnd.x - v3_orbitBegin.x) / scrWidth;
            GLfloat orbitAngleH = TWO_PI * dragRatio;
            glm::vec3 orbitAxisH = glm::vec3(0.0, 1.0f, 0.0);
            Quaternion qOrbitRotH = Quaternion(orbitAngleH, orbitAxisH, RAD);
            q_TotalRot = qOrbitRotH * q_TotalRot;
        }
        /* Right-click-drag vertical movement will orbit about X and Z axes */
        if (v3_orbitBegin.y != v3_orbitEnd.y) {
            float dragRatio = (v3_orbitBegin.y - v3_orbitEnd.y) / scrHeight;
            GLfloat orbitAngleV = TWO_PI * dragRatio;
            glm::vec3 cameraUnit = glm::normalize(glm::vec3(cameraVec.x, 0.0f, cameraVec.z));
            glm::vec3 orbitAxisV = glm::vec3(cameraUnit.z, 0.0f, -cameraUnit.x);
            Quaternion qOrbitRotV = Quaternion(orbitAngleV, orbitAxisV, RAD);
            q_TotalRot = qOrbitRotV * q_TotalRot;
        }
        update();
    }
    if (gw_rolling) {
        v3_rollBegin = v3_rollEnd;
        v3_rollEnd = mouseVec;

        /* Middle-click-drag will orbit about camera look vector */
        if (v3_rollBegin.x != v3_rollEnd.x) {
            float dragRatio = (v3_rollBegin.x - v3_rollEnd.x) / scrWidth;
            GLfloat orbitAngleL = TWO_PI * dragRatio;
            glm::vec3 orbitAxisL = glm::normalize(cameraVec);
            Quaternion qOrbitRotL = Quaternion(orbitAngleL, orbitAxisL, RAD);
            q_TotalRot = qOrbitRotL * q_TotalRot;
        }
        update();
    }
    if (gw_sliding) {
        v3_slideBegin = v3_slideEnd;
        v3_slideEnd = mouseVec;
        
        /* Left-click drag will grab and slide world */
        if (v3_slideBegin != v3_slideEnd) {
            glm::vec3 deltaSlide = 0.01f * (v3_slideEnd - v3_slideBegin);
            glm::vec3 cameraSlide = glm::vec3(deltaSlide.x, deltaSlide.y, 0.0f);
            m4_translation = glm::translate(m4_translation, cameraSlide);
        }
        update();
    }
}

void GWidget::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::RightButton)
        gw_orbiting = false;
    else if (e->button() == Qt::LeftButton)
        gw_sliding = false;
    else if (e->button() == Qt::MiddleButton)
        gw_rolling = false;
    else
        QWidget::mouseReleaseEvent(e);
}

void GWidget::keyPressEvent(QKeyEvent * e) {
    if (e->key() == Qt::Key_Home) {
        initVecsAndMatrices();
        update();
    } else
        QWidget::keyPressEvent(e);
}