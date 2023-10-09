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


GWidget::GWidget(QWidget *parent)
    : QOpenGLWidget(parent) {
}

GWidget::~GWidget() {
    cleanup();
}

void GWidget::cleanup() {
    makeCurrent();

    //std::cout << "Rendered " << gw_frame << " frames." << std::endl;

    glDeleteBuffers(1, &gw_vbo);

    doneCurrent();
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
    const GLfloat vertices[] = {
         0.0f,  0.69f, 0.0f,  1.0f, 0.0f, 0.0f,
        -0.4f, -0.4f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.4f, -0.4f, 0.0f,  0.0f, 0.0f, 1.0f  
    };
    
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

    shaderProg = new Program(this);
    shaderProg->addDefaultShaders();
    shaderProg->init();
    shaderProg->linkAndValidate();
    shaderProg->initVAO();
    shaderProg->bindVAO();

    /* VBO */
    glGenBuffers(1, &gw_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gw_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* Attribute Pointers -- Vertices*/
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    /* Attribute Pointers -- Colours */
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    /* Camera and World Init */
    glClearColor(0.0f, 0.05f, 0.08f, 0.0f);
    //gw_camera.setToIdentity();
    //gw_camera.translate(0, 0, -1);
    //gw_world.setToIdentity();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    shaderProg->clearVAO();
}

void GWidget::paintGL() {
    /* Render */
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    glClear(GL_COLOR_BUFFER_BIT);
    shaderProg->beginRender();
    
    glDrawArrays(GL_TRIANGLES, 0, 3);

    shaderProg->endRender();
    //++gw_frame;
}

void GWidget::resizeGL(int w, int h) {
    gw_proj.setToIdentity();
    gw_proj.perspective(45.0f, GLfloat(w) / h, 0.1f, 100.0f);
}

