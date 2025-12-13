#include "materialdatabase.h"

#include <imgui/imgui.h>

#include "../system/profiler.h"

#include "../resource/instance.h"
#include "../resource/material.h"
#include "../resource/primitive.h"

#include "../graphic/gpuresource.h"

#include "scene.h"


namespace unisim
{

DefineProfilePoint(Material);

DefineResource(MaterialDatabase);
DefineResource(BindlessTextures);


struct GpuMaterial
{
    glm::vec4 albedo;
    glm::vec4 emission;
    glm::vec4 specular;

    int albedoTexture;
    int specularTexture;
    int pad1;
    int pad2;
};


MaterialDatabase::MaterialDatabase() :
    GraphicTask("Material Registry")
{
}

void MaterialDatabase::registerMaterial(const std::shared_ptr<Material>& material)
{
    bool isRegistered = isMaterialRegistered(material);
    assert(!isRegistered);
    if(isRegistered)
        return;

    assert(_materialIds.find(uint64_t(material.get())) == _materialIds.end());
    _materialIds[uint64_t(material.get())] = MaterialId(_materials.size());
    _materials.push_back(material);
}

unsigned int MaterialDatabase::materialId(const std::shared_ptr<Material>& material) const
{
    auto it = _materialIds.find(uint64_t(material.get()));
    assert(it != _materialIds.end());

    if(it != _materialIds.end())
        return it->second;
    else
        return MaterialId_Invalid;
}

bool MaterialDatabase::isMaterialRegistered(const std::shared_ptr<Material>& material) const
{
    return _materialIds.find(uint64_t(material.get())) != _materialIds.end();
}

void MaterialDatabase::registerDynamicResources(GraphicContext& context)
{
    for(const auto& instance : context.scene.instances())
    {
        for(const auto& primitive : instance->primitives())
        {
            const auto& material = primitive->material();
            if(material && !isMaterialRegistered(material))
                registerMaterial(material);
        }
    }
    
    GpuResourceManager& resources = context.resources;

    for(std::size_t i = 0; i < _materials.size(); ++i)
    {
        _materialsResourceIds.push_back({
            resources.registerDynamicResource(_materials[i]->name() + "_texture_albedo"),
            resources.registerDynamicResource(_materials[i]->name() + "_texture_specular"),
            resources.registerDynamicResource(_materials[i]->name() + "_bindless_albedo"),
            resources.registerDynamicResource(_materials[i]->name() + "_bindless_specular"),
        });
    }
}

bool MaterialDatabase::definePathTracerModules(GraphicContext& context)
{
    return true;
}

bool MaterialDatabase::definePathTracerInterface(GraphicContext& context, PathTracerInterface& interface)
{
    bool ok = true;

    ok = ok && interface.declareStorage("Textures");
    ok = ok && interface.declareStorage("Materials");

    return ok;
}

bool MaterialDatabase::defineResources(GraphicContext& context)
{
    bool ok = true;
    
    GpuResourceManager& resources = context.resources;

    for(std::size_t i = 0; i < _materials.size(); ++i)
    {
        const Material& material = *_materials[i];

        if(material.albedo() != nullptr)
        {
            ok = ok && resources.define<GpuTextureResource>(_materialsResourceIds[i].textureAlbedo, {*material.albedo()});
            const auto& albedoTexure = resources.get<GpuTextureResource>(_materialsResourceIds[i].textureAlbedo);
            ok = ok && resources.define<GpuBindlessResource>(_materialsResourceIds[i].bindlessAlbedo, {material.albedo(), albedoTexure});
        }

        if(material.specular() != nullptr)
        {
            ok = ok && resources.define<GpuTextureResource>(_materialsResourceIds[i].textureSpecular, {*material.specular()});
            const auto& specularTexture = resources.get<GpuTextureResource>(_materialsResourceIds[i].textureSpecular);
            ok = ok && resources.define<GpuBindlessResource>(_materialsResourceIds[i].bindlessSpecular, {material.specular(), specularTexture});
        }
    }

    std::vector<GpuMaterial> gpuMaterials;
    std::vector<GpuBindlessTextureDescriptor> gpuTextures;
    _hash = toGpu(context, gpuTextures, gpuMaterials);

    ok = ok && context.resources.define<GpuStorageResource>(
        ResourceName(MaterialDatabase),
        {sizeof (GpuMaterial), gpuMaterials.size(), gpuMaterials.data()});

    ok = ok && context.resources.define<GpuStorageResource>(
        ResourceName(BindlessTextures),
        {sizeof (GpuBindlessTextureDescriptor), gpuTextures.size(), gpuTextures.data()});

    return ok;
}

void MaterialDatabase::setPathTracerResources(
    GraphicContext& context,
    PathTracerInterface& interface) const
{
    GpuResourceManager& resources = context.resources;

    context.device.bindBuffer(resources.get<GpuStorageResource>(ResourceName(BindlessTextures)), interface.getStorageBindPoint("Textures"));
    context.device.bindBuffer(resources.get<GpuStorageResource>(ResourceName(MaterialDatabase)), interface.getStorageBindPoint("Materials"));
}

void MaterialDatabase::update(GraphicContext& context)
{
    Profile(Material);

    std::vector<GpuMaterial> gpuMaterials;
    std::vector<GpuBindlessTextureDescriptor> gpuTextures;
    uint64_t hash = toGpu(context, gpuTextures, gpuMaterials);

    if(_hash == hash)
        return;

    _hash = hash;
    
    GpuResourceManager& resources = context.resources;
    resources.get<GpuStorageResource>(ResourceName(BindlessTextures)).update(
                {sizeof(GpuBindlessTextureDescriptor), gpuTextures.size(), gpuTextures.data()});
    resources.get<GpuStorageResource>(ResourceName(MaterialDatabase)).update(
                {sizeof(GpuMaterial), gpuMaterials.size(), gpuMaterials.data()});
}

void MaterialDatabase::render(GraphicContext& context)
{
}

uint64_t MaterialDatabase::toGpu(
    const GraphicContext& context,
    std::vector<GpuBindlessTextureDescriptor>& gpuBindless,
    std::vector<GpuMaterial>& gpuMaterials)
{
    GpuResourceManager& resources = context.resources;

    for(std::size_t i = 0; i < _materials.size(); ++i)
    {
        const Material& material = *_materials[i];

        GpuMaterial gpuMaterial;

        gpuMaterial.albedo = glm::vec4(material.defaultAlbedo(), 1.0);

        gpuMaterial.emission = glm::vec4(material.defaultEmissionColor()
                                         * material.defaultEmissionLuminance(),
                                         1.0);

        gpuMaterial.specular = glm::vec4(
                    // Roughness to GGX's 'a' parameter
                    material.defaultRoughness() * material.defaultRoughness(),
                    material.defaultMetalness(),
                    material.defaultReflectance(),
                    0);

        if(material.albedo() != nullptr)
        {
            gpuMaterial.albedoTexture = gpuBindless.size();
            gpuBindless.emplace_back(resources.get<GpuBindlessResource>(_materialsResourceIds[i].bindlessAlbedo).handle());
        }
        else
        {
            gpuMaterial.albedoTexture = -1;
        }

        if(material.specular() != nullptr)
        {
            gpuMaterial.specularTexture = gpuBindless.size();
            gpuBindless.emplace_back(resources.get<GpuBindlessResource>(_materialsResourceIds[i].bindlessSpecular).handle());
        }
        else
        {
            gpuMaterial.specularTexture = -1;
        }

        gpuMaterials.push_back(gpuMaterial);
    }

    uint64_t hash = 0;
    hash = hashVec(gpuBindless, hash);
    hash = hashVec(gpuMaterials, hash);

    return hash;
}


}
