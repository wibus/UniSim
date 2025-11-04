#ifndef GPURESOURCE_H
#define GPURESOURCE_H

#include <string>
#include <vector>


namespace unisim
{

enum class TextureFormat
{
    UNORM8,
    Float32
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
    TextureFormat format;
    int numComponents;
    std::vector<unsigned char> data;

    // ImGui image ID
    unsigned int handle;

private:
    Texture(TextureFormat format, unsigned char* pixelData);

};

}

#endif // GPURESOURCE_H
