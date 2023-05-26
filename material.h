#ifndef MATERIAL_H
#define MATERIAL_H

#include <GLM/glm.hpp>

#include <string>


namespace unisim
{

struct Texture
{
    enum Format {UNORM8, Float32};

    Texture() : width(0), height(0), format(UNORM8), numComponents(4), data(nullptr) {}
    ~Texture() { delete [] data; }
    operator bool() const { return data != nullptr; }

    int width;
    int height;
    Format format;
    int numComponents;
    unsigned char* data;
};

class Material
{
public:
    Material(const std::string& name);
    ~Material();

    bool loadAlbedo(const std::string& fileName);
    Texture* albedo() const;

    glm::dvec3 defaultAlbedo() const { return _defaultAlbedo; }
    void setDefaultAlbedo(const glm::dvec3& albedo);

    glm::dvec3 defaultEmission() const { return _defaultEmission; }
    void setDefaultEmission(const glm::dvec3& emission);

    float defaultRoughness() const { return _defaultRoughness; }
    void setDefaultRoughness(float roughness);

    float defaultMetalness() const { return _defaultMetalness; }
    void setDefaultMetalness(float metalness);

    float defaultReflectance() const { return _defaultReflectance; }
    void setDefaultReflectance(float reflectance);

private:
    Texture* loadJpeg(const std::string& fileName);
    Texture* loadPng(const std::string& fileName);
    Texture* loadExr(const std::string& fileName);

    std::string _name;
    Texture* _albedo;

    glm::dvec3 _defaultAlbedo;
    glm::dvec3 _defaultEmission;
    float _defaultRoughness;
    float _defaultMetalness;
    float _defaultReflectance;
};



// IMPLEMENTATION //
inline Texture* Material::albedo() const
{
    return _albedo;
}

}

#endif // MATERIAL_H
