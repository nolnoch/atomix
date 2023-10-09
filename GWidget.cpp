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

#define RAD(t) (glm::radians((t)))


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
            //Vertex               //Colour
         0.0f,  0.4f,  0.0f,   0.6f, 0.6f, 0.6f,    //top
        -0.4f, -0.4f,  0.4f,   0.0f, 0.6f, 0.0f,    //left
         0.4f, -0.4f,  0.4f,   0.0f, 0.0f, 0.69f,   //right
         0.0f, -0.4f, -0.4f,   0.6f, 0.0f, 0.0f     //back
    };

    GLuint indices[] = {
        0, 1, 2,
        2, 3, 0,
        3, 1, 0,
        3, 2, 1
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
    m4_proj = glm::mat4(1.0f);
    m4_view = glm::mat4(1.0f);
    m4_world = glm::mat4(1.0f);

    /* Matrices */
    shaderProg->enable();
    m4_world = glm::rotate(m4_world, RAD(-38.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m4_view = glm::translate(m4_view, glm::vec3(0.1f, 0.1f, -1.7f));
    m4_proj = glm::perspective(RAD(40.0f), GLfloat(width()) / height(), 0.1f, 100.0f);
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
    
    glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);

    shaderProg->endRender();
}

void GWidget::resizeGL(int w, int h) {
    m4_proj = glm::mat4(1.0f);
    m4_proj = glm::perspective(RAD(40.0f), GLfloat(w) / h, 0.1f, 100.0f);
}

