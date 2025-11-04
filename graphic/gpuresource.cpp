#include "gpuresource.h"

#include <algorithm>

#include "../resource/texture.h"

#include "pathtracer.h"


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
    case TextureFormat::UNORM8 :
        internalFormat = def.texture.numComponents == 3 ? GL_RGB8 : GL_RGBA8;
        type = GL_UNSIGNED_BYTE;
        break;
    case TextureFormat::Float32:
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
        case TextureFormat::UNORM8:
            format = def.texture->numComponents == 3 ? GL_RGB8 : GL_RGBA8;
            break;
        case TextureFormat::Float32:
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


GpuStorageResource::GpuStorageResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    glGenBuffers(1, &bufferId);
    update(def);
}

GpuStorageResource::~GpuStorageResource()
{
    glDeleteBuffers(1, &bufferId);
}

void GpuStorageResource::update(const Definition& def) const
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
    GLsizei dataSize = def.elemSize * def.elemCount;
    glBufferData(GL_SHADER_STORAGE_BUFFER, dataSize, def.data, GL_STATIC_DRAW);
}


GpuConstantResource::GpuConstantResource(ResourceId id, Definition def) :
    GpuResource(id)
{
    glGenBuffers(1, &bufferId);
    update(def);
}

GpuConstantResource::~GpuConstantResource()
{
    glDeleteBuffers(1, &bufferId);
}

void GpuConstantResource::update(const Definition& def) const
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
    glBindBuffer(GL_UNIFORM_BUFFER, bufferId);
    glBufferData(GL_UNIFORM_BUFFER, def.size, def.data, GL_STATIC_DRAW);
}


unsigned int GpuResourceManager::_staticResourceCount = 0;
std::vector<std::string> GpuResourceManager::_staticNames;

GpuResourceManager::GpuResourceManager() :
    _resourceCount(_staticResourceCount),
    _names(_staticNames)
{
}

GpuResourceManager::~GpuResourceManager()
{
    _resources.clear();
}

ResourceId GpuResourceManager::registerStaticResource(const std::string& name)
{
    _staticNames.push_back(name);
    return _staticResourceCount++;
}

ResourceId GpuResourceManager::registerDynamicResource(const std::string& name)
{
    _names.push_back(name);
    return _resourceCount++;
}

void GpuResourceManager::initialize()
{
    _resources.resize(_resourceCount);
}

void GpuResourceManager::registerPathTracerProvider(const std::shared_ptr<PathTracerProvider>& provider)
{
    _providers.push_back(provider);
}

std::vector<std::shared_ptr<PathTracerModule>> GpuResourceManager::pathTracerModules() const
{
    std::vector<std::shared_ptr<PathTracerModule>> modules;
    auto appendShaders = [&](const std::vector<std::shared_ptr<PathTracerModule>>& s)
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

void GpuResourceManager::setPathTracerResources(GraphicContext& context, PathTracerInterface& interface) const
{
    for(const auto& provider : _providers)
        provider->setPathTracerResources(context, interface);
}

uint64_t GpuResourceManager::pathTracerHash() const
{
    uint64_t hash;
    for(const auto& provider : _providers)
        hash = PathTracerProvider::combineHashes(hash, provider->hash());
    return hash;
}


}
