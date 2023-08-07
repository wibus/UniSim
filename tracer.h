#ifndef TRACER_H
#define TRACER_H

#include <memory>

#include <GLM/glm.hpp>
#include <GL/glew.h>

#include "graphic.h"


namespace unisim
{

struct Viewport;
class PathTracerInterface;
struct GpuPathTracerCommonParams;


class PathTracer : public GraphicTask
{
public:
    PathTracer();

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
            GpuPathTracerCommonParams& gpuCommonParams);

    ResourceId _blueNoiseTextureResourceIds[BLUE_NOISE_TEX_COUNT];
    ResourceId _blueNoiseBindlessResourceIds[BLUE_NOISE_TEX_COUNT];

    glm::vec4 _halton[HALTON_SAMPLE_COUNT];

    GLuint _pathTraceLoc;
    GLuint _pathTraceUnit;
    GLint _pathTraceFormat;

    GLuint _pathTraceUtilsShaderId;
    GLuint _computePathTraceShaderId;
    GLuint _computePathTraceProgramId;

    unsigned int _frameIndex;
    uint64_t _pathTracerHash;

    std::unique_ptr<Viewport> _viewport;

    std::unique_ptr<PathTracerInterface> _pathTracerInterface;
};

}

#endif // TRACER_H
