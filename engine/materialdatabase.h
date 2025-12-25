#ifndef MATERIALDATABASE_H
#define MATERIALDATABASE_H

#include <GLM/glm.hpp>

#include <unordered_map>
#include <vector>

#include "pathtracer.h"


namespace unisim
{

struct Material;
struct GpuMaterial;

using MaterialId = uint32_t;
const MaterialId MaterialId_Invalid = MaterialId(-1);


class MaterialDatabase : public PathTracerProvider
{
public:
    MaterialDatabase();

    void registerMaterial(const std::shared_ptr<Material>& material);
    MaterialId materialId(const std::shared_ptr<Material>& material) const;
    bool isMaterialRegistered(const std::shared_ptr<Material>& material) const;
    
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

    std::vector<std::shared_ptr<Material>> _materials;
    std::unordered_map<uint64_t, MaterialId> _materialIds;

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

#endif // MATERIALDATABASE_H
