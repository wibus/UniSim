#ifndef MATERIAL_H
#define MATERIAL_H

#include <GLM/glm.hpp>

#include <string>


namespace unisim
{

struct Texture
{
    Texture() : width(0), height(0), data(nullptr) {}
    ~Texture() { delete [] data; }
    operator bool() const { return data != nullptr; }

    int width;
    int height;
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

private:
    Texture* loadJpeg(const std::string& fileName);
    Texture* loadPng(const std::string& fileName);

    std::string _name;
    Texture* _albedo;

    glm::dvec3 _defaultAlbedo;
    glm::dvec3 _defaultEmission;
};



// IMPLEMENTATION //
inline Texture* Material::albedo() const
{
    return _albedo;
}

}

#endif // MATERIAL_H
