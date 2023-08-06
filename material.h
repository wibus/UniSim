#ifndef MATERIAL_H
#define MATERIAL_H

#include <GLM/glm.hpp>

#include <unordered_map>
#include <string>
#include <vector>

#include "graphic.h"


namespace unisim
{

struct GpuMaterial;


struct Texture
{
    enum Format {UNORM8, Float32};

    Texture();
    ~Texture();
    operator bool() const { return !data.empty(); }

    static const Texture BLACK_UNORM8;
    static const Texture BLACK_Float32;

    static Texture* load(const std::string& fileName);
    static Texture* loadJpeg(const std::string& fileName);
    static Texture* loadPng(const std::string& fileName);
    static Texture* loadExr(const std::string& fileName);

    int width;
    int height;
    Format format;
    int numComponents;
    std::vector<unsigned char> data;

    // ImGui image ID
    unsigned int handle;

private:
    Texture(Format format, unsigned char* pixelData);

};

class Material
{
public:
    Material(const std::string& name);
    ~Material();

    const std::string& name() const { return _name;}

    Texture* albedo() const { return _albedo; }
    bool loadAlbedo(const std::string& fileName);

    Texture* specular() const { return _specular; }
    bool loadSpecular(const std::string& fileName);

    glm::vec3 defaultAlbedo() const { return _defaultAlbedo; }
    void setDefaultAlbedo(const glm::vec3& albedo);

    glm::vec3 defaultEmissionColor() const { return _defaultEmissionColor; }
    void setDefaultEmissionColor(const glm::vec3& emissionColor);

    float defaultEmissionLuminance() const { return _defaultEmissionLuminance; }
    void setDefaultEmissionLuminance(float emissionLuminance);

    float defaultRoughness() const { return _defaultRoughness; }
    void setDefaultRoughness(float roughness);

    float defaultMetalness() const { return _defaultMetalness; }
    void setDefaultMetalness(float metalness);

    float defaultReflectance() const { return _defaultReflectance; }
    void setDefaultReflectance(float reflectance);

private:
    std::string _name;
    Texture* _albedo;
    Texture* _specular;

    glm::vec3 _defaultAlbedo;
    glm::vec3 _defaultEmissionColor;
    float _defaultEmissionLuminance;
    float _defaultRoughness;
    float _defaultMetalness;
    float _defaultReflectance;
};


using MaterialId = uint32_t;
const MaterialId MaterialId_Invalid = MaterialId(-1);


class MaterialDatabase : public GraphicTask
{
public:
    MaterialDatabase();

    void registerMaterial(const std::shared_ptr<Material>& material);
    MaterialId materialId(const std::shared_ptr<Material>& material) const;
    bool isMaterialRegistered(const std::shared_ptr<Material>& material) const;

    void registerDynamicResources(GraphicContext& context) override;
    bool definePathTracerModules(GraphicContext& context) override;
    bool defineResources(GraphicContext& context) override;

    void setPathTracerResources(
            GraphicContext& context,
            PathTracerInterface& interface) const override;

    void update(GraphicContext& context) override;
    void render(GraphicContext& context) override;

private:
    uint64_t toGpu(
            const GraphicContext& context,
            std::vector<GPUBindlessTexture>& textures,
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

#endif // MATERIAL_H
