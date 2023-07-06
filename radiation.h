#ifndef RADIATION_H
#define RADIATION_H

#include <vector>
#include <memory>

#include <GLM/glm.hpp>
#include <GL/glew.h>

#include "graphic.h"
#include "camera.h"


namespace unisim
{

class Scene;

class Radiation : public GraphicTask
{
public:
    Radiation();

    void registerDynamicResources(GraphicContext& context) override;
    bool defineResources(GraphicContext& context) override;

    void update(GraphicContext& context) override;
    void render(GraphicContext& context) override;

    static const unsigned int BLUE_NOISE_TEX_COUNT = 64;
    static const unsigned int HALTON_SAMPLE_COUNT = 64;
    static const unsigned int MAX_FRAME_COUNT = 4096;

private:
    void setViewport(Viewport viewport);

    GLuint _vbo;
    GLuint _vao;

    GLuint _commonUbo;
    GLuint _instancesSSBO;
    GLuint _dirLightsSSBO;
    GLuint _materialsSSBO;
    GLuint _emittersSSBO;

    std::vector<GLuint> _objectToMat;

    struct MaterialResources
    {
        ResourceId textureAlbedo;
        ResourceId textureSpecular;
        ResourceId bindlessAlbedo;
        ResourceId bindlessSpecular;
    };

    std::vector<MaterialResources> _objectsResourceIds;

    ResourceId _blueNoiseTextureResourceIds[BLUE_NOISE_TEX_COUNT];
    ResourceId _blueNoiseBindlessResourceIds[BLUE_NOISE_TEX_COUNT];


    glm::vec4 _halton[HALTON_SAMPLE_COUNT];

    GLuint _backgroundLoc;
    GLint _backgroundUnit;

    GLuint _pathTraceLoc;
    GLuint _pathTraceUnit;
    GLint _pathTraceFormat;

    GLuint _computePathTraceId;
    GLuint _colorGradingId;

    unsigned int _frameIndex;
    std::size_t _lastFrameHash;

    Viewport _viewport;
};

}

#endif // RADIATION_H
