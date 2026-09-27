#include "gpuresource.h"

#include <PilsCore/Gpu/Memory.h>


namespace unisim
{

unsigned int GpuResourceManager::_staticResourceCount = 0;
std::vector<std::string> GpuResourceManager::_staticNames;

DefineResource(FullScreenTriangle);


GpuResource::GpuResource(GpuResourceManager& manager, ResourceId id) :
    id(id),
    _manager(manager)
{
}

GpuResource::~GpuResource()
{
}

GpuResourceManager::GpuResourceManager(const GpuDevice& gpuDevice) :
    _gpuDevice(gpuDevice),
    _resourceCount(_staticResourceCount),
    _names(_staticNames),
    _isBindlessTextureDirty(true)
{
    PILS_ASSERT(_names.size() == _resourceCount, "Static resource name count differs from static resource count");
}

GpuResourceManager::~GpuResourceManager()
{
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
    PILS_ASSERT(_names.size() == _resourceCount, "Resource name count differs from resource count");
    _resources.resize(_resourceCount);

    const uint32_t MAX_BINDLESS_TEXTURES = 100;
    _bindlessTextures.resize(MAX_BINDLESS_TEXTURES);
    _bindlessTextureFreeList.resize(MAX_BINDLESS_TEXTURES);
    for (uint32_t i = 0; i < MAX_BINDLESS_TEXTURES; ++i)
        _bindlessTextureFreeList[i] = BindlessTextureIndex(MAX_BINDLESS_TEXTURES - i - 1);

    initializeNative();

    std::vector<glm::vec3> fullScreenTriangleVerts;
    fullScreenTriangleVerts.push_back({-1.0f, -1.0f,  0.0f});
    fullScreenTriangleVerts.push_back({ 3.0f, -1.0f,  0.0f});
    fullScreenTriangleVerts.push_back({-1.0f,  3.0f,  0.0f});
    define<GpuGeometryResource>(ResourceName(FullScreenTriangle), {fullScreenTriangleVerts});
}

BindlessTextureIndex GpuResourceManager::registerBindlessTexture(const GpuTextureResource& textureResource, const Texture& texture)
{
    PILS_ASSERT(!_bindlessTextureFreeList.empty(), "Ran out of bindless texture slots");
    BindlessTextureIndex index = _bindlessTextureFreeList.back();
    _bindlessTextureFreeList.pop_back();

    _bindlessTextures[int32_t(index)].reset(new GpuBindlessTexture(textureResource, texture));

    _isBindlessTextureDirty = true;

    return index;
}

void GpuResourceManager::unregisterBindlessTexture(BindlessTextureIndex index)
{
    PILS_ASSERT(_bindlessTextures[int32_t(index)].get() != nullptr, "Trying to unregister bindless texture that was never registered at index ", int(index));
    _bindlessTextures[int32_t(index)].reset();
    _bindlessTextureFreeList.push_back(index);
    _isBindlessTextureDirty = true;
}


}
