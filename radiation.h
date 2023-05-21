#ifndef RADIATION_H
#define RADIATION_H

#include <vector>
#include <memory>

#include <GL/glew.h>


namespace unisim
{

class Object;
class Camera;
struct Viewport;

class Radiation
{
public:
    Radiation();

    bool initialize(const std::vector<std::shared_ptr<Object>>& objects, const Viewport& viewport);
    void setViewport(Viewport viewport);

    void draw(const std::vector<std::shared_ptr<Object>>& objects, double dt, const Camera& camera);

private:
    GLuint _vbo;
    GLuint _vao;

    GLuint _commonUbo;
    GLuint _instancesUbo;
    GLuint _materialsUbo;

    std::vector<GLuint> _objectToMat;

    GLuint _backgroundTexId;
    GLuint64 _backgroundHdl;

    GLuint _pathTraceUAVId;
    GLuint _pathTraceLoc;
    GLuint _pathTraceUnit;
    GLint _pathTraceFormat;

    GLuint _framentPathTraceId;
    GLuint _computePathTraceId;
    GLuint _colorGradingId;
};

}

#endif // RADIATION_H
