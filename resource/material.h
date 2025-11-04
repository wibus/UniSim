#ifndef MATERIAL_H
#define MATERIAL_H

#include <GLM/glm.hpp>

#include <string>


namespace unisim
{

struct Texture;

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

    void ui();

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

}

#endif // MATERIAL_H
