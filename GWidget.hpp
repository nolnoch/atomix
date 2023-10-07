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
#include <QOpenGLContext>
#include <QOpenGLFunctions_3_1>
#include <QOpenGLVersionFunctionsFactory>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>

#define GWIDTH 1843
#define GHEIGHT 1196

class GWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_1 {
    Q_OBJECT

public:
    GWidget(QWidget *parent = nullptr);
    ~GWidget();

    int gw_frame = 0;

public slots:
    void cleanup();
    void renderLater();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

private:
    QOpenGLContext *gw_context = nullptr;
    QOpenGLShaderProgram *gw_program = nullptr;
    //QOpenGLVertexArrayObject gw_vao;
    //QOpenGLBuffer gw_vbo;
    QMatrix4x4 gw_proj;
    QMatrix4x4 gw_camera;
    QMatrix4x4 gw_world;
    int gw_posAttr = 0;
    int gw_colAttr = 0;
    int gw_uniMatrix = 0;
    bool gw_init = false;
};

#endif