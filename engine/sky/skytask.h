#ifndef SKYTASK_H
#define SKYTASK_H

#include "../../resource/sky.h"

#include "../taskgraph/pathtracerprovider.h"


namespace unisim
{

class SkyTask : public PathTracerProviderTask
{
public:
    SkyTask();

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

private:
    uint64_t toGpu(
        const GraphicContext& context,
        struct GpuStarsParams& params) const;

    uint64_t toGpu(
        const GraphicContext& context,
        struct GpuMoonLightParams& moonLightParams,
        struct GpuAtmosphereParams& atmosphereParams) const;

    struct AtmosphereRenderState
    {
        AtmosphereRenderState();
        ~AtmosphereRenderState();

        glm::mat4 _moonTransform;
        glm::vec4 _moonQuaternion;

        GraphicProgramPtr _moonLightProgram;
        GpuProgramInterfacePtr _moonLightGpi;

        int _moonTexSize;
        std::unique_ptr<Texture> _moonAlbedo;

        std::size_t _atmosphereHash;
        bool _moonIsDirty;
    };

    std::unique_ptr<AtmosphereRenderState> _atmosphereRenderState;
};

}

#endif // SKY_H
