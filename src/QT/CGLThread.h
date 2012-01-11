
#ifndef GLTHREAD
#define GLTHREAD

#include <QObject>
#include <QThread>
#include <QSize>
#include <string>

using namespace std;

class CGLWidget;

class CGLThread : public QThread {
    Q_OBJECT
public:
    CGLThread(CGLWidget * glWidget);

    static void CheckOpenGLError(string function_name);
    void resizeViewport(const QSize &size);  
    void run();
    void stop();
private:
    void glDrawTriangle();
    bool doRendering;
    bool doResize;
    int w;
    int h;
    float rotAngle;
    CGLWidget * mGLWidget;
    int id;
    static int count;
};
    
#endif