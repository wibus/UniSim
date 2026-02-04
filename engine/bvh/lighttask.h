#ifndef LIGHTTASK_H
#define LIGHTTASK_H

#include <vector>

#include <GLM/glm.hpp>

#include "../taskgraph/pathtracerprovider.h"



namespace unisim
{

class Scene;

struct GpuEmitter;
struct GpuDirectionalLight;


class LightTask : public PathTracerProviderTask
{
public:
    LightTask();

    bool defineResources(GraphicContext& context) override;

    bool definePathTracerInterface(
        GraphicContext& context,
        PathTracerInterface& interface) override;
    void bindPathTracerResources(
        GraphicContext& context,
        CompiledGpuProgramInterface& compiledGpi) const override;

    void update(GraphicContext& context) override;

private:
    uint64_t toGpu(
        GraphicContext& context,
            std::vector<GpuEmitter>& gpuEmitters,
            std::vector<GpuDirectionalLight>& gpuDirectionalLights);
};

}

#endif // LIGHTTASK_H
