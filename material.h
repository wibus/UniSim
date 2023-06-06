#ifndef MATERIAL_H
#define MATERIAL_H

#include <GLM/glm.hpp>

#include <string>


namespace unisim
{

struct Texture
{
    enum Format {UNORM8, Float32};

    Texture();
    ~Texture();
    operator bool() const { return data != nullptr; }

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
    unsigned char* data;
    bool ownsMemory;

    // ImGui image ID
    unsigned int handle;

private:
    Texture(Format format, unsigned char *pixelData);

};

class Material
{
public:
    Material(const std::string& name);
    ~Material();

    bool loadAlbedo(const std::string& fileName);
    Texture* albedo() const;

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

    glm::vec3 _defaultAlbedo;
    glm::vec3 _defaultEmissionColor;
    float _defaultEmissionLuminance;
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
