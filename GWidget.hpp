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
//#include <eigen3/Eigen/Eigen>
#include "program.hpp"

#define GWIDTH 1843
#define GHEIGHT 1196

class GWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    GWidget(QWidget *parent = nullptr);
    ~GWidget();

    int gw_frame = 0;

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

    uint gw_vbo = 0;
    QMatrix4x4 gw_proj;
    QMatrix4x4 gw_camera;
    QMatrix4x4 gw_world;
    int gw_projMat = 0;
    int gw_moveMat = 0;
    int gw_normMat = 0;
    int gw_lightMat = 0;
    int gw_posAttr = 0;
    int gw_colAttr = 0;
    int gw_uniMatrix = 0;
    bool gw_init = false;
};

#endif