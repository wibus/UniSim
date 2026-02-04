#ifndef MATERIALTASK_H
#define MATERIALTASK_H

#include <GLM/glm.hpp>

#include <vector>

#include "../taskgraph/pathtracerprovider.h"


namespace unisim
{

struct Material;
struct GpuMaterial;


class MaterialTask : public PathTracerProviderTask
{
public:
    MaterialTask();
    
    void registerDynamicResources(GraphicContext& context) override;
    bool defineResources(GraphicContext& context) override;

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
        std::vector<GpuBindlessTextureDescriptor>& textures,
        std::vector<GpuMaterial>& materials);

    struct MaterialResources
    {
        ResourceId textureAlbedo;
        ResourceId textureSpecular;
        ResourceId bindlessAlbedo;
        ResourceId bindlessSpecular;
    };

    std::vector<MaterialResources> _materialsResourceIds;
};

}

#endif // MATERIALTASK_H
