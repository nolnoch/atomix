/** GWidget.hpp 
 * 
 * atomix by Wade Burch
 * (braernoch@gmail.com)
 * 
 * GPL Open Source
*/

#ifndef GWIDGET_H
#define GWIDGET_H


#include <QScreen>
#include <QBoxLayout>
#include <QOpenGLWidget>
//#include <QOpenGLContext>
#include <QOpenGLFunctions_4_5_Core>
#include <QMatrix4x4>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
//#include <eigen3/Eigen/Eigen>
#include "program.hpp"

#define GWIDTH 1843
#define GHEIGHT 1196

class GWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    GWidget(QWidget *parent = nullptr);
    ~GWidget();

public slots:
    void cleanup();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

private:
    bool checkCompileShader(uint shader);
    bool checkCompileProgram(uint program);

    Program *shaderProg = nullptr;
    QOpenGLContext *gw_context = nullptr;

    GLuint id_vbo = 0;
    GLuint id_ebo = 0;
    glm::mat4 m4_proj;
    glm::mat4 m4_view;
    glm::mat4 m4_world;
    int gw_frame = 0;
    bool gw_init = false;
};

#endif