#ifndef TRACER_H
#define TRACER_H

#include <memory>

#include <GLM/glm.hpp>

#include "pathtracer.h"


namespace unisim
{

struct Viewport;
class PathTracerInterface;


class PathTracer : public PathTracerProvider
{
public:
    PathTracer();
    
    void registerDynamicResources(GraphicContext& context) override;
    bool defineResources(GraphicContext& context) override;
    bool defineShaders(GraphicContext& context) override;

    bool definePathTracerModules(
        GraphicContext& context,
        std::vector<std::shared_ptr<PathTracerModule>>& modules) override;
    bool definePathTracerInterface(
        GraphicContext& context,
        PathTracerInterface& interface) override;
    void bindPathTracerResources(
        GraphicContext& context,
        CompiledGpuProgramInterface& compiledGpi) const override;
    
    void update(GraphicContext& context) override;
    void render(GraphicContext& context) override;

    void setPathTracerProviders(const std::vector<PathTracerProviderPtr>& providers);

    static const unsigned int BLUE_NOISE_TEX_COUNT = 64;
    static const unsigned int HALTON_SAMPLE_COUNT = 64;
    static const unsigned int MAX_FRAME_COUNT = 4096;

private:
    uint64_t toGpu(GraphicContext& context,
        struct GpuPathTracerCommonParams& gpuParams);

    ResourceId _blueNoiseTextureResourceIds[BLUE_NOISE_TEX_COUNT];
    ResourceId _blueNoiseBindlessResourceIds[BLUE_NOISE_TEX_COUNT];

    glm::vec4 _halton[HALTON_SAMPLE_COUNT];

    GraphicProgramPtr _pathTracerProgram;

    unsigned int _frameIndex;
    uint64_t _pathTracerHash;

    std::unique_ptr<Viewport> _viewport;

    std::shared_ptr<PathTracerInterface> _pathTracerInterface;
    std::vector<std::shared_ptr<PathTracerModule>> _pathTracerModules;
    std::vector<std::shared_ptr<PathTracerProvider>> _pathTracerProviders;
};

}

#endif // TRACER_H
