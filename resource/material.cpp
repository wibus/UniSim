#include "material.h"

#include <imgui/imgui.h>

#include "../resource/texture.h"

namespace unisim
{

Material::Material(const std::string& name) :
    _name(name),
    _albedo(nullptr),
    _specular(nullptr),
    _defaultAlbedo(1, 1, 1),
    _defaultEmissionColor(0, 0, 0),
    _defaultEmissionLuminance(0),
    _defaultRoughness(1),
    _defaultMetalness(0),
    _defaultReflectance(0.04)
{

}

Material::~Material()
{
    delete _albedo;
}


void Material::setDefaultAlbedo(const glm::vec3& albedo)
{
    _defaultAlbedo = albedo;
}

void Material::setDefaultEmissionColor(const glm::vec3& emissionColor)
{
    _defaultEmissionColor = emissionColor;
}

void Material::setDefaultEmissionLuminance(float emissionLuminance)
{
    _defaultEmissionLuminance = emissionLuminance;
}

void Material::setDefaultRoughness(float roughness)
{
    _defaultRoughness = roughness;
}

void Material::setDefaultMetalness(float metalness)
{
    _defaultMetalness = metalness;
}

void Material::setDefaultReflectance(float reflectance)
{
    _defaultReflectance = reflectance;
}

bool Material::loadAlbedo(const std::string& fileName)
{
    if(_albedo)
    {
        delete _albedo;
        _albedo = nullptr;
    }

    _albedo = Texture::load(fileName);

    return _albedo != nullptr;
}

bool Material::loadSpecular(const std::string &fileName)
{
    if(_specular)
    {
        delete _specular;
        _specular = nullptr;
    }

    _specular = Texture::load(fileName);

    return _specular != nullptr;
}

void Material::ui()
{
    if(_albedo && ImGui::TreeNode("Albedo Texture"))
    {
        _albedo->ui();
        ImGui::TreePop();
    }

    if(_specular && ImGui::TreeNode("Specular Texture"))
    {
        _specular->ui();
        ImGui::TreePop();
    }

    glm::vec3 albedo = _defaultAlbedo;
    if(ImGui::ColorEdit3("Albedo", &albedo[0]))
        setDefaultAlbedo(albedo);

    glm::vec3 emissionColor = _defaultEmissionColor;
    if(ImGui::ColorEdit3("Emission", &emissionColor[0]))
        setDefaultEmissionColor(emissionColor);

    float emissionLuminance = _defaultEmissionLuminance;
    if(ImGui::InputFloat("Luminance", &emissionLuminance))
        setDefaultEmissionLuminance(emissionLuminance);

    float roughness = _defaultRoughness;
    if(ImGui::SliderFloat("Roughness", &roughness, 0, 1))
        setDefaultRoughness(roughness);

    float metalness = _defaultMetalness;
    if(ImGui::SliderFloat("Metalness", &metalness, 0, 1))
        setDefaultMetalness(metalness);

    float reflectance = _defaultReflectance;
    if(ImGui::InputFloat("Reflectance", &reflectance, 0.01f))
    {
        reflectance = glm::clamp(reflectance, 0.0f, 1.0f);
        setDefaultReflectance(reflectance);
    }
}


// Material Database
MaterialDatabase::MaterialDatabase()
{
}

void MaterialDatabase::unregisterAllMaterials()
{
    _materialIds.clear();
    _materials.clear();
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

}
