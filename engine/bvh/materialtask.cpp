#include "materialtask.h"

#include <imgui/imgui.h>

#include "../system/profiler.h"

#include "../resource/instance.h"
#include "../resource/material.h"
#include "../resource/primitive.h"
#include "../resource/terrain.h"

#include "../graphic/gpudevice.h"
#include "../graphic/gpuresource.h"

#include "../scene.h"


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


MaterialTask::MaterialTask() :
    PathTracerProviderTask("Material")
{
}

void MaterialTask::registerDynamicResources(GraphicContext& context)
{
    context.scene.materialDb()->unregisterAllMaterials();

    auto registerMaterials = [&](const std::vector<std::shared_ptr<Instance>>& instances)
    {
        for(const auto& instance : instances)
        {
            for(const auto& primitive : instance->primitives())
            {
                const auto& material = primitive->material();
                if(material && !context.scene.materialDb()->isMaterialRegistered(material))
                {
                    context.scene.materialDb()->registerMaterial(material);
                }
            }
        }
    };

    registerMaterials(context.scene.instances());
    if (Terrain* terrain = context.scene.terrain().get())
        registerMaterials(terrain->instances());
    
    GpuResourceManager& resources = context.resources;

    for(const auto& material : context.scene.materialDb()->materials())
    {
        _materialsResourceIds.push_back({
            resources.registerDynamicResource(material->name() + "_texture_albedo"),
            resources.registerDynamicResource(material->name() + "_texture_specular"),
            resources.registerDynamicResource(material->name() + "_bindless_albedo"),
            resources.registerDynamicResource(material->name() + "_bindless_specular"),
        });
    }
}

bool MaterialTask::defineResources(GraphicContext& context)
{
    bool ok = true;

    GpuResourceManager& resources = context.resources;

    const std::vector<std::shared_ptr<Material>>& materials = context.scene.materialDb()->materials();

    for(std::size_t i = 0; i < materials.size(); ++i)
    {
        const Material& material = *materials[i];

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

bool MaterialTask::definePathTracerModules(GraphicContext& context, std::vector<std::shared_ptr<PathTracerModule>>& modules)
{
    return true;
}

bool MaterialTask::definePathTracerInterface(GraphicContext& context, PathTracerInterface& interface)
{
    bool ok = true;

    ok = ok && interface.declareStorage({"Textures"});
    ok = ok && interface.declareStorage({"Materials"});

    return ok;
}

void MaterialTask::bindPathTracerResources(
    GraphicContext& context,
    CompiledGpuProgramInterface& compiledGpi) const
{
    GpuResourceManager& resources = context.resources;

    context.device.bindBuffer(resources.get<GpuStorageResource>(ResourceName(BindlessTextures)), compiledGpi.getStorageBindPoint("Textures"));
    context.device.bindBuffer(resources.get<GpuStorageResource>(ResourceName(MaterialDatabase)), compiledGpi.getStorageBindPoint("Materials"));
}

void MaterialTask::update(GraphicContext& context)
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

void MaterialTask::render(GraphicContext& context)
{
}

uint64_t MaterialTask::toGpu(
    const GraphicContext& context,
    std::vector<GpuBindlessTextureDescriptor>& gpuBindless,
    std::vector<GpuMaterial>& gpuMaterials)
{
    GpuResourceManager& resources = context.resources;

    const std::vector<std::shared_ptr<Material>>& materials = context.scene.materialDb()->materials();

    for(std::size_t i = 0; i < materials.size(); ++i)
    {
        const Material& material = *materials[i];

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
