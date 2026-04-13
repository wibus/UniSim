#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <vector>


namespace unisim
{

enum class TextureFormat
{
    R8G8B8A8_UNORM,
    R32G32B32A32_FLOAT
};

struct Texture
{
    Texture();
    ~Texture();
    operator bool() const { return !data.empty(); }

    static const Texture BLACK_UNORM8;
    static const Texture BLACK_Float32;

    static Texture* load(const std::string& fileName);
    static Texture* loadJpeg(const std::string& fileName);
    static Texture* loadPng(const std::string& fileName);
    static Texture* loadExr(const std::string& fileName);

    void ui();

    int width;
    int height;
    int depth{1};
    TextureFormat format;
    int numComponents;
    std::vector<unsigned char> data;

    // ImGui image ID
    unsigned int handle;

private:
    Texture(TextureFormat format, unsigned char* pixelData);

};

}

#endif // TEXTURE_H
