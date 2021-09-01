#ifndef RADIATION_H
#define RADIATION_H

#include <vector>

#include <GL/glew.h>


namespace unisim
{

class Body;
class Camera;

class Radiation
{
public:
    Radiation();

    bool initialize(const std::vector<Body*>& bodies);

    void draw(const std::vector<Body*>& bodies, double dt, Camera& camera);

private:
    GLuint _vbo;
    GLuint _vao;

    GLuint _commonUbo;
    GLuint _bodiesUbo;
    GLuint _materialsUbo;

    std::vector<GLuint> _bodyToMat;

    GLuint _backgroundTexId;
    GLuint64 _backgroundHdl;

    GLuint _programId;
};

}

#endif // RADIATION_H
