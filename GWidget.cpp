/** GWidget.cpp 
 * 
 * atomix by Wade Burch
 * (braernoch@gmail.com)
 * 
 * GPL Open Source
*/

#include <iostream>
#include <QTimer>
#include "GWidget.hpp"

#define RADN(t) (glm::radians((t)))


GWidget::GWidget(QWidget *parent)
    : QOpenGLWidget(parent) {
}

GWidget::~GWidget() {
    cleanup();
}

void GWidget::cleanup() {
    makeCurrent();

    //std::cout << "Rendered " << gw_frame << " frames." << std::endl;

    glDeleteBuffers(1, &id_vbo);
    delete shaderProg;

    doneCurrent();
}

GLfloat GWidget::findRotationAngle(glm::vec3 startVec, glm::vec3 endVec) {
  GLfloat angle, zA, zB, xA, xB, dotProd;
  GLfloat width = this->width() / 2.0f;

  xA = glm::sin(((startVec.x - width) / width) * PI / 2);
  xB = glm::sin(((endVec.x - width) / width) * PI / 2);

  zA = glm::sqrt(1.0f - (xA * xA));
  zB = glm::sqrt(1.0f - (xB * xB));

  glm::vec3 vA(xA, 0.0, zA);
  glm::vec3 vB(xB, 0.0, zB);

  dotProd = glm::dot(glm::normalize(vA), glm::normalize(vB));

  return glm::acos(dotProd) * 1.20f;
}

bool GWidget::checkCompileShader(uint shader) {
    int success;
    char log[512];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, log);
        std::cout << "Shader Compilation Failed\n" << log << std::endl;
    }
    return success;
}

bool GWidget::checkCompileProgram(uint program) {
    int success;
    char log[512];

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, log);
        std::cout << "Program Compilation Failed\n" << log << std::endl;
    }
    return success;
}

void GWidget::initializeGL() {
    float zero, peak, edge, back, forX, forZ, root;
    edge = 0.6f;  // <-- Change this to scale diamond
    peak = edge * 1.2;
    zero = 0.0f;
    root = sqrt(3);
    back = root / 3 * edge;
    forZ = root / 6 * edge;
    forX = edge / 2;
    
    const GLfloat vertices[] = {
              //Vertex              //Colour
          zero,  peak,  zero,   0.5f, 0.5f, 0.5f,    //top
         -forX,  zero,  forZ,   0.1f, 0.3f, 0.1f,    //left
          forX,  zero,  forZ,   0.1f, 0.0f, 0.3f,    //right
          zero,  zero, -back,   0.3f, 0.1f, 0.1f,    //back
          zero, -peak,  zero,   0.0f, 0.0f, 0.0f     //bottom
    };

    GLuint indices[] = {
        0, 1, 2,
        2, 3, 0,
        3, 1, 0,
        1, 2, 4,
        2, 3, 4,
        3, 4, 1
    };
    this->gw_faces = sizeof(indices) / sizeof(indices[0]);
    
    /* Init -- Context and OpenGL */
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

    /* Program */
    shaderProg = new Program(this);
    shaderProg->addDefaultShaders();
    shaderProg->init();
    shaderProg->linkAndValidate();

    /* VAO */
    shaderProg->initVAO();
    shaderProg->bindVAO();

    /* VBO -- Init */
    glGenBuffers(1, &id_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, id_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* VBO -- Attribute Pointers -- Vertices*/
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    /* VBO -- Attribute Pointers -- Colours */
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    /* EBO */
    glGenBuffers(1, &id_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    /* Camera and World Init */
    glClearColor(0.0f, 0.05f, 0.08f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    m4_rotation = glm::mat4(1.0f);
    m4_translation = glm::mat4(1.0f);
    m4_proj = glm::mat4(1.0f);
    m4_view = glm::mat4(1.0f);
    m4_world = glm::mat4(1.0f);
    v3_cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f);
    v3_cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    v3_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    v3_orbitBegin = glm::vec3(0);
    v3_orbitEnd = glm::vec3(0);

    /* Matrices */
    shaderProg->enable();
    m4_view = glm::lookAt(v3_cameraPosition, v3_cameraTarget, v3_cameraUp);
    m4_proj = glm::perspective(RADN(45.0f), GLfloat(width()) / height(), 0.1f, 100.0f);
    shaderProg->setUniformMatrix(4, "worldMat", glm::value_ptr(m4_world));
    shaderProg->setUniformMatrix(4, "viewMat", glm::value_ptr(m4_view));
    shaderProg->setUniformMatrix(4, "projMat", glm::value_ptr(m4_proj));

    /* Release */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    shaderProg->clearVAO();
    shaderProg->disable();
}

void GWidget::paintGL() {
    /* Render */
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    shaderProg->beginRender();

    m4_view = glm::lookAt(v3_cameraPosition, v3_cameraTarget, v3_cameraUp);
    m4_rotation = glm::make_mat4(&q_TotalRot.matrix()[0]);
    m4_world = m4_view * m4_translation * m4_rotation;
    m4_proj = glm::perspective(RADN(45.0f), GLfloat(width()) / height(), 0.1f, 100.0f);
    shaderProg->setUniformMatrix(4, "worldMat", glm::value_ptr(m4_world));
    shaderProg->setUniformMatrix(4, "viewMat", glm::value_ptr(m4_view));
    shaderProg->setUniformMatrix(4, "projMat", glm::value_ptr(m4_proj));
    
    glDrawElements(GL_TRIANGLES, gw_faces, GL_UNSIGNED_INT, 0);

    shaderProg->endRender();
}

void GWidget::resizeGL(int w, int h) {
    m4_proj = glm::mat4(1.0f);
    m4_proj = glm::perspective(RADN(45.0f), GLfloat(w) / h, 0.1f, 100.0f);
}

void GWidget::mousePressEvent(QMouseEvent *e) {
    int x, y;
    x = e->pos().x();
    y = e->pos().y();

    if (e->button() == Qt::LeftButton) {
        v3_slideBegin = glm::vec3(x, height() - y, v3_cameraPosition.z);
        v3_slideEnd = v3_slideBegin;
        gw_sliding = true;
    } else if (e->button() == Qt::RightButton) {
        v3_orbitBegin = glm::vec3(x, height() - y, v3_cameraPosition.z);
        v3_orbitEnd = v3_orbitBegin;
        gw_orbiting = true;
    } else
        QWidget::mousePressEvent(e);
}

void GWidget::mouseMoveEvent(QMouseEvent *e) {
    int x, y;
    x = e->pos().x();
    y = e->pos().y();

    if (gw_orbiting) {
        v3_orbitBegin = v3_orbitEnd;
        v3_orbitEnd = glm::vec3(x, height() - y, v3_cameraPosition.z);

        /* Horizontal movement rotates */
        if (v3_orbitBegin.x != v3_orbitEnd.x) {
            float signAxis = v3_orbitBegin.x < v3_orbitEnd.x ? 1.0f : -1.0f;
            glm::vec3 orbitAxis = glm::vec3(0.0, signAxis, 0.0);
            GLfloat orbitAngle = findRotationAngle(v3_orbitBegin, v3_orbitEnd);
            Quaternion qOrbitRot = Quaternion(orbitAngle, orbitAxis, RAD);
            q_TotalRot = qOrbitRot * q_TotalRot;
        }
        update();
    }
    if (gw_sliding) {
        v3_slideBegin = v3_slideEnd;
        v3_slideEnd = glm::vec3(x, height() - y, v3_cameraPosition.z);
        
        /* Movement slides */
        if (v3_slideBegin != v3_slideEnd) {
            float slideFactor = v3_cameraPosition.z / width() * 2;
            glm::vec3 deltaSlide = slideFactor * (v3_slideEnd - v3_slideBegin);
            m4_translation = glm::translate(m4_translation, glm::vec3(deltaSlide.x, 0.0, -deltaSlide.y));
        }
        update();
    }
    

}

void GWidget::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::RightButton) {gw_orbiting = false;}
    if (e->button() == Qt::LeftButton) {gw_sliding = false;}
}
