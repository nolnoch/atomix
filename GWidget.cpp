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

const char *verShaderSrc =
    "#version 450 core\n"
    "layout(location = 0) in vec3 svPos;\n"
    "layout(location = 1) in vec3 svCol;\n"
    "out vec3 verColour;\n"
    "void main() {\n"
    "   gl_Position = vec4(svPos.x, svPos.y, svPos.z, 1.0);\n"
    "   verColour = svCol;\n"
    "}\n";

const char *fragShaderSrc =
    "#version 450 core\n"
    "out vec4 FragColour;\n"
    "in vec3 verColour;\n"
    "void main() {\n"
    "   FragColour = vec4(verColour, 1.0f);\n"
    "}\n";


GWidget::GWidget(QWidget *parent)
    : QOpenGLWidget(parent) {
}

GWidget::~GWidget() {
    cleanup();
}

void GWidget::cleanup() {
    makeCurrent();

    std::cout << "Rendered " << gw_frame << " frames." << std::endl;

    glDeleteVertexArrays(1, &gw_vao);
    glDeleteBuffers(1, &gw_vbo);
    glDeleteProgram(gw_prog);

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
    uint verShader, fragShader;
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
    //qgf = this;

    shaderProg = new Program(this);
    shaderProg->addDefaultShaders();
    shaderProg->init();
    shaderProg->linkAndValidate();

    /* Shaders -- Vertex (Load and compile) 
    verShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(verShader, 1, &verShaderSrc, NULL);
    glCompileShader(verShader);
    Q_ASSERT(checkCompileShader(verShader));

    /* Shaders -- Fragment (Load and compile) 
    fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragShaderSrc, NULL);
    glCompileShader(fragShader);
    Q_ASSERT(checkCompileShader(fragShader));
    
    /* Shaders -- Program (Attach shaders and link) 
    gw_prog = glCreateProgram();
    glAttachShader(gw_prog, verShader);
    glAttachShader(gw_prog, fragShader);
    glLinkProgram(gw_prog);
    Q_ASSERT(checkCompileProgram(gw_prog));
    glDeleteShader(verShader);
    glDeleteShader(fragShader);

    /* VAO */
    glGenVertexArrays(1, &gw_vao);
    glBindVertexArray(gw_vao);
    
    /* VBO (Define and load data) */
    glGenBuffers(1, &gw_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gw_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* Attribute Pointers */
    // Vertices
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Vertex Colours
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    /* Camera and World Init */
    //gw_camera.setToIdentity();
    //gw_camera.translate(0, 0, -1);
    //gw_world.setToIdentity();

    glClearColor(0.0f, 0.05f, 0.08f, 0.0f);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    
    //QTimer* Timer = new QTimer(this);
    //connect(Timer, SIGNAL(timeout()), this, SLOT(update()));
    //Timer->start(1000/33);
}

void GWidget::paintGL() {
    /* Render */
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(gw_prog);
    glBindVertexArray(gw_vao);
    
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);
    glUseProgram(0);

    ++gw_frame;
}

void GWidget::resizeGL(int w, int h) {
    gw_proj.setToIdentity();
    gw_proj.perspective(45.0f, GLfloat(w) / h, 0.1f, 100.0f);
}

