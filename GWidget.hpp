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
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QOpenGLFunctions_4_5_Core>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "program.hpp"
#include "quaternion.hpp"

#define GWIDTH 1843
#define GHEIGHT 1196

enum { RX = 32, RY = 64, RZ = 128 };

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

    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);

private:
    bool checkCompileShader(uint shader);
    bool checkCompileProgram(uint program);
    GLfloat findRotationAngle(glm::vec3 startVec, glm::vec3 endVec, uint axis);
    void initVecsAndMatrices();

    Program *shaderProg = nullptr;
    QOpenGLContext *gw_context = nullptr;

    GLuint id_vbo = 0;
    GLuint id_ebo = 0;
    glm::mat4 m4_proj;
    glm::mat4 m4_view;
    glm::mat4 m4_world;
    glm::mat4 m4_rotation;
    glm::mat4 m4_translation;
    glm::vec3 v3_cameraPosition;
    glm::vec3 v3_cameraTarget;
    glm::vec3 v3_cameraUp;
    glm::vec3 v3_orbitBegin;
    glm::vec3 v3_orbitEnd;
    glm::vec3 v3_slideBegin;
    glm::vec3 v3_slideEnd;
    Quaternion q_TotalRot;
    uint gw_faces = 0;
    int gw_frame = 0;
    bool gw_init = false;
    bool gw_orbiting = false;
    bool gw_sliding = true;
};

#endif