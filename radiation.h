#ifndef RADIATION_H
#define RADIATION_H

#include <vector>
#include <memory>

#include <GLM/glm.hpp>
#include <GL/glew.h>


namespace unisim
{

class Scene;
class Camera;
struct Viewport;

class Radiation
{
public:
    Radiation();

    bool initialize(const Scene &scene, const Viewport& viewport);
    void setViewport(Viewport viewport);

    void draw(const Scene &scene, double dt, const Camera& camera);

    static const unsigned int BLUE_NOISE_TEX_COUNT = 64;
    static const unsigned int HALTON_SAMPLE_COUNT = 64;
    static const unsigned int MAX_FRAME_COUNT = 4096;

private:
    GLuint _vbo;
    GLuint _vao;

    GLuint _commonUbo;
    GLuint _instancesSSBO;
    GLuint _dirLightsSSBO;
    GLuint _materialsSSBO;
    GLuint _emittersSSBO;

    std::vector<GLuint> _objectToMat;

    GLuint _backgroundTexId;
    GLuint _backgroundLoc;
    GLint _backgroundUnit;

    GLuint _blueNoiseTexIds[BLUE_NOISE_TEX_COUNT];
    GLuint64 _blueNoiseTexHdls[BLUE_NOISE_TEX_COUNT];

    glm::vec4 _halton[HALTON_SAMPLE_COUNT];

    GLuint _pathTraceUAVId;
    GLuint _pathTraceLoc;
    GLuint _pathTraceUnit;
    GLint _pathTraceFormat;

    GLuint _computePathTraceId;
    GLuint _colorGradingId;

    unsigned int _frameIndex;
    std::size_t _lastFrameHash;
};

}

#endif // RADIATION_H
