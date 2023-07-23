#include "resource.h"

#include <algorithm>

#include "material.h"


namespace unisim
{

GpuResource::GpuResource(ResourceId id) :
    id(id)
{
}

GpuResource::~GpuResource()
{
}


GpuTextureResource::GpuTextureResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    glGenTextures(1, &texId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    GLint internalFormat = 0;
    GLenum type = 0;
    switch(def.texture.format)
    {
    case Texture::UNORM8 :
        internalFormat = def.texture.numComponents == 3 ? GL_RGB8 : GL_RGBA8;
        type = GL_UNSIGNED_BYTE;
        break;
    case Texture::Float32:
        internalFormat = GL_RGBA32F;
        type = GL_FLOAT;
        break;
    }

    GLenum format = def.texture.numComponents == 3 ? GL_RGB : GL_RGBA;

    glTexImage2D(
        GL_TEXTURE_2D,
        0, // mip level
        internalFormat,
        def.texture.width,
        def.texture.height,
        0, // border
        format,
        type,
        &def.texture.data.front());

    // For ImGui
    Texture& texNoConst = const_cast<Texture&>(def.texture);
    texNoConst.handle = texId;

    glBindTexture(GL_TEXTURE_2D, 0);
}

GpuTextureResource::~GpuTextureResource()
{
    glDeleteTextures(1, &texId);
}


GpuImageResource::GpuImageResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    glGenTextures(1, &texId);

    glBindTexture(GL_TEXTURE_2D, texId);
    glTexStorage2D(GL_TEXTURE_2D, 1, def.format, def.width, def.height);
    glBindTexture(GL_TEXTURE_2D, 0);
}

GpuImageResource::~GpuImageResource()
{
    glDeleteTextures(1, &texId);
}


GpuBindlessResource::GpuBindlessResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    GLenum format = GL_RGBA8;
    if(def.texture != nullptr)
    {
        switch(def.texture->format)
        {
        case Texture::UNORM8:
            format = def.texture->numComponents == 3 ? GL_RGB8 : GL_RGBA8;
            break;
        case Texture::Float32:
            format = def.texture->numComponents == 3 ? GL_RGB32F : GL_RGBA32F;
            break;
        }
    }

    handle = glGetImageHandleARB(def.texId, 0, GL_FALSE, 0, format);
    glMakeImageHandleResidentARB(handle, GL_READ_ONLY);
}

GpuBindlessResource::~GpuBindlessResource()
{
    glMakeImageHandleNonResidentARB(handle);
}


PathTracerProvider::PathTracerProvider()
{
}

PathTracerProvider::~PathTracerProvider()
{
}

void PathTracerProvider::setPathTracerResources(GraphicContext& context, GLuint programId, GLuint& nextTextureUnit) const
{
}

unsigned int ResourceManager::_resourceCount = 0;
std::vector<std::string> ResourceManager::_names;

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
    _resources.clear();
}

ResourceId ResourceManager::registerResource(const std::string& name)
{
    ++_resourceCount;
    _names.push_back(name);
    return _resourceCount;
}

void ResourceManager::initialize()
{
    _resources.resize(_resourceCount);
}

void ResourceManager::registerPathTracerProvider(const std::shared_ptr<PathTracerProvider>& provider)
{
    _providers.push_back(provider);
}

std::vector<GLuint> ResourceManager::pathTracerModules() const
{
    std::vector<GLuint> modules;
    auto appendShaders = [&](const std::vector<GLuint>& s)
    {
        modules.insert(modules.end(), s.begin(), s.end());
    };

    // Append modules from all providers
    for(const auto& provider : _providers)
        appendShaders(provider->pathTracerModules());

    // Remove duplicated modules
    std::sort( modules.begin(), modules.end() );
    modules.erase( std::unique( modules.begin(), modules.end() ), modules.end());

    return modules;
}

void ResourceManager::setPathTracerResources(GraphicContext& context, GLuint programId, GLuint& nextTextureUnit) const
{
    for(const auto& provider : _providers)
        provider->setPathTracerResources(context, programId, nextTextureUnit);
}


}
