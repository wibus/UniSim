#ifndef MATERIAL_H
#define MATERIAL_H

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

private:
    Texture* loadJpeg(const std::string& fileName);

    std::string _name;
    Texture* _albedo;
};



// IMPLEMENTATION //
inline Texture* Material::albedo() const
{
    return _albedo;
}

}

#endif // MATERIAL_H
