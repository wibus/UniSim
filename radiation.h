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
class PathTracerInterface;

struct GpuEmitter;
struct GpuDirectionalLight;
struct GpuPathTracerCommonParams;


class Radiation : public GraphicTask
{
public:
    Radiation();

    void registerDynamicResources(GraphicContext& context) override;
    bool definePathTracerModules(GraphicContext& context) override;
    bool defineResources(GraphicContext& context) override;

    void setPathTracerResources(
            GraphicContext& context,
            PathTracerInterface& interface) const override;

    void update(GraphicContext& context) override;
    void render(GraphicContext& context) override;

    static const unsigned int BLUE_NOISE_TEX_COUNT = 64;
    static const unsigned int HALTON_SAMPLE_COUNT = 64;
    static const unsigned int MAX_FRAME_COUNT = 4096;

private:
    uint64_t toGpu(
            GraphicContext& context,
            GpuPathTracerCommonParams& gpuCommonParams,
            std::vector<GpuEmitter>& gpuEmitters,
            std::vector<GpuDirectionalLight>& gpuDirectionalLights);

    GLuint _vbo;
    GLuint _vao;

    ResourceId _blueNoiseTextureResourceIds[BLUE_NOISE_TEX_COUNT];
    ResourceId _blueNoiseBindlessResourceIds[BLUE_NOISE_TEX_COUNT];

    glm::vec4 _halton[HALTON_SAMPLE_COUNT];

    GLuint _pathTraceLoc;
    GLuint _pathTraceUnit;
    GLint _pathTraceFormat;

    GLuint _pathTraceUtilsShaderId;
    GLuint _computePathTraceShaderId;
    GLuint _computePathTraceProgramId;
    GLuint _colorGradingId;

    unsigned int _frameIndex;
    uint64_t _pathTracerHash;

    Viewport _viewport;

    std::shared_ptr<PathTracerInterface> _pathTracerInterface;
};

}

#endif // RADIATION_H
