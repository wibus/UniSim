#ifndef TRACER_H
#define TRACER_H

#include <memory>

#include <GLM/glm.hpp>

#include "graphictask.h"


namespace unisim
{

struct Viewport;
class PathTracerInterface;


class PathTracer : public GraphicTask
{
public:
    PathTracer();
    
    void registerDynamicResources(GraphicContext& context) override;
    bool definePathTracerModules(GraphicContext& context) override;
    bool definePathTracerInterface(GraphicContext& context, PathTracerInterface& interface) override;
    bool defineShaders(GraphicContext& context) override;
    bool defineResources(GraphicContext& context) override;

    void setPathTracerResources(
        GraphicContext& context,
            PathTracerInterface& interface) const override;
    
    void update(GraphicContext& context) override;
    void render(GraphicContext& context) override;

    std::shared_ptr<PathTracerInterface> initInterface();

    static const unsigned int BLUE_NOISE_TEX_COUNT = 64;
    static const unsigned int HALTON_SAMPLE_COUNT = 64;
    static const unsigned int MAX_FRAME_COUNT = 4096;

private:
    uint64_t toGpu(GraphicContext& context,
        struct GpuPathTracerCommonParams& gpuParams);

    ResourceId _blueNoiseTextureResourceIds[BLUE_NOISE_TEX_COUNT];
    ResourceId _blueNoiseBindlessResourceIds[BLUE_NOISE_TEX_COUNT];

    glm::vec4 _halton[HALTON_SAMPLE_COUNT];

    PathTracerModulePtr _utilsModule;
    PathTracerModulePtr _pathTraceModule;
    GraphicProgramPtr _pathTracerProgram;

    unsigned int _frameIndex;
    uint64_t _pathTracerHash;

    std::unique_ptr<Viewport> _viewport;

    std::shared_ptr<PathTracerInterface> _pathTracerInterface;
};

}

#endif // TRACER_H
