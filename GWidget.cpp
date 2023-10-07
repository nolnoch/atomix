/** GWidget.cpp 
 * 
 * atomix by Wade Burch
 * (braernoch@gmail.com)
 * 
 * GPL Open Source
*/

#include <iostream>
//#include <GL/glew.h>
#include "GWidget.hpp"

GWidget::GWidget(QWidget *parent)
    : QOpenGLWidget(parent) {
    
    initializeGL();
}

GWidget::~GWidget() {
    cleanup();
}

void GWidget::cleanup() {
    makeCurrent();

    std::cout << "Rendered " << gw_frame << " frames." << std::endl;

    delete gw_program;
    delete gw_offscreen;

    doneCurrent();
}

void GWidget::renderLater() {
    //requestUpdate();
}

static const char *vertexShaderSource =
    "attribute highp vec4 posAttr;\n"
    "attribute lowp vec4 colAttr;\n"
    "varying lowp vec4 col;\n"
    "uniform highp mat4 matrix;\n"
    "void main() {\n"
    "   col = colAttr;\n"
    "   gl_Position = matrix * posAttr;\n"
    "}\n";

static const char *fragmentShaderSource =
    "varying lowp vec4 col;\n"
    "void main() {\n"
    "   gl_FragColor = col;\n"
    "}\n";

void GWidget::initializeGL() {
    /* OpenGL Init */
    if (!gw_context) {
        gw_context = new QOpenGLContext(this);
        if (!gw_context->create())
            std::cout << "Failed to create OpenGL context" << std::endl;
        else
            std::cout << "Init OpenGL Context" << std::endl;
    }
    gw_offscreen = new QOffscreenSurface();
    gw_offscreen->create();
    gw_context->makeCurrent(gw_offscreen);
    if (!gw_init) {
        if (!this->initializeOpenGLFunctions()) {
            std::cout << "Failed to initialize OpenGL functions" << std::endl;
        } else {
            gw_init = true;
            std::cout << "Init OpenGL functions" << std::endl;
        }
    }
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);

    /* Shader Init */
    gw_program = new QOpenGLShaderProgram(this);
    gw_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    gw_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    gw_program->link();
    gw_posAttr = gw_program->attributeLocation("posAttr");
    gw_colAttr = gw_program->attributeLocation("colAttr");
    gw_uniMatrix = gw_program->uniformLocation("matrix");

    /* Camera Init */
    gw_camera.setToIdentity();
    gw_camera.translate(0, 0, -1);
    gw_world.setToIdentity();

    makeCurrent();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    //connect(gw_context, &QOpenGLContext::aboutToBeDestroyed, this, &GWidget::cleanup);
}

void GWidget::paintGL() {
    makeCurrent();

    /* Render */
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    glClear(GL_COLOR_BUFFER_BIT);
    gw_program->bind();

    QMatrix4x4 matrix;
    matrix.perspective(60.0f, 4.0f / 3.0f, 0.1f, 100.0f);
    matrix.translate(0, 0, -2);
    matrix.rotate(100.0f * gw_frame / screen()->refreshRate(), 0, 1, 0);

    gw_program->setUniformValue(gw_uniMatrix, QVector3D(0, 0, 70));

    static const GLfloat vertices[] = {
         0.0f,  0.707f,
        -0.5f, -0.5f,
         0.5f, -0.5f
    };

    static const GLfloat colors[] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };

    glVertexAttribPointer(gw_posAttr, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glVertexAttribPointer(gw_colAttr, 3, GL_FLOAT, GL_FALSE, 0, colors);

    glEnableVertexAttribArray(gw_posAttr);
    glEnableVertexAttribArray(gw_colAttr);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(gw_colAttr);
    glDisableVertexAttribArray(gw_posAttr);
    
    gw_program->release();

    ++gw_frame;
}

void GWidget::resizeGL(int w, int h) {
    gw_proj.setToIdentity();
    gw_proj.perspective(45.0f, GLfloat(w) / h, 0.01f, 100.0f);
}

