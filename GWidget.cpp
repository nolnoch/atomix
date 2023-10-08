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

    std::cout << "Rendered " << gw_frame << " frames." << std::endl;

    if (gw_program)
        delete gw_program;

    doneCurrent();
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
    /* Context and OpenGL Init */
    if (!context()) {
        gw_context = new QOpenGLContext(this);
        if (!gw_context->create())
            std::cout << "Failed to create OpenGL context" << std::endl;
    } else {
        gw_context = context();
    }
    makeCurrent();
    if (!gw_init) {
        if (!this->initializeOpenGLFunctions())
            std::cout << "Failed to initialize OpenGL functions" << std::endl;
        else
            gw_init = true;
    }

    /* Shader Init */
    gw_program = new QOpenGLShaderProgram(this);
    gw_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    gw_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    if (!gw_program->link())
        std::cout << "Shader program failed to link." << std::endl;
    gw_posAttr = gw_program->attributeLocation("posAttr");
    gw_colAttr = gw_program->attributeLocation("colAttr");
    gw_uniMatrix = gw_program->uniformLocation("matrix");
    Q_ASSERT(gw_posAttr != -1);
    Q_ASSERT(gw_colAttr != -1);
    Q_ASSERT(gw_uniMatrix != -1);

    /* Camera and World Init */
    //gw_camera.setToIdentity();
    //gw_camera.translate(0, 0, -1);
    //gw_world.setToIdentity();

    glClearColor(0.0f, 0.03f, 0.05f, 0.0f);
    //glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    
    //QTimer* Timer = new QTimer(this);
    //connect(Timer, SIGNAL(timeout()), this, SLOT(update()));
    //Timer->start(1000/33);
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
    gw_program->setUniformValue(gw_uniMatrix, matrix);

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
    gw_proj.perspective(45.0f, GLfloat(w) / h, -10.0f, 100.0f);
}

