#ifndef GPURESOURCE_H
#define GPURESOURCE_H

#include <vector>
#include <memory>
#include <string>

#include <GLM/glm.hpp>

#include <PilsCore/Utils/Assert.h>

#include "../resource/texture.h"


#ifdef UNISIM_GRAPHIC_BACKEND_GL
#include <GL/glew.h>
#endif // UNISIM_GRAPHIC_BACKEND_GL

namespace pils
{
namespace gpu
{
class BufferSet;
class StorageBuffer;
class UniformBuffer;
class ConstantBuffer;
class VertexBuffer;
class Image;
class Texture;
class ImageView;
class Sampler;
}
}

namespace unisim
{

class CompiledGpuProgramInterface;
class GpuDevice;
class View;
class GpuResourceManager;

enum class BindlessTextureIndex : int32_t {};
constexpr BindlessTextureIndex Invalid_BindlessTextureIndex = BindlessTextureIndex(~0x0);

typedef unsigned int ResourceId;
const ResourceId Invalid_ResourceId = ~0x0;

#define ResourceName(name) ResourceId_##name
#define DeclareResource(name) extern ResourceId ResourceName(name)
#define DefineResource(name) ResourceId ResourceName(name) = GpuResourceManager::registerStaticResource(#name)


class GpuTextureResourceHandle
{
public:
#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GpuTextureResourceHandle();

    GLuint texId;
    GLenum dimension;
#elif UNISIM_GRAPHIC_BACKEND_VK
    GpuTextureResourceHandle(
        const pils::gpu::ImageView& textureView,
        const pils::gpu::Sampler& sampler);

    const pils::gpu::ImageView& textureView;
    const pils::gpu::Sampler& sampler;
#endif
};

class GpuImageResourceHandle
{
public:
#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GpuImageResourceHandle();

    GLuint texId;
    GLenum internalFormat;
    GLenum dimension;
#elif UNISIM_GRAPHIC_BACKEND_VK
    GpuImageResourceHandle(
        const pils::gpu::ImageView& imageView);

    const pils::gpu::ImageView& imageView;
#endif
};

class GpuStorageResourceHandle
{
public:

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GpuStorageResourceHandle();

    GLuint bufferId;
#elif UNISIM_GRAPHIC_BACKEND_VK
    GpuStorageResourceHandle(
        const pils::gpu::StorageBuffer& buffer,
        uint64_t offset,
        uint64_t size);

    const pils::gpu::StorageBuffer& buffer;
    uint64_t offset;
    uint64_t size;
#endif
};

class GpuConstantResourceHandle
{
public:
#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GpuConstantResourceHandle();

    GLuint bufferId;
#elif UNISIM_GRAPHIC_BACKEND_VK
    GpuConstantResourceHandle(
        const pils::gpu::ConstantBuffer& buffer,
        uint64_t offset,
        uint64_t size);

    const pils::gpu::ConstantBuffer& buffer;
    uint64_t offset;
    uint64_t size;
#endif
};

class GpuGeometryResourceHandle
{
public:
#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GpuGeometryResourceHandle();

    GLuint vao;
    GLuint vbo;
#elif UNISIM_GRAPHIC_BACKEND_VK
    GpuGeometryResourceHandle(
        const pils::gpu::VertexBuffer& buffer,
        uint64_t offset,
        uint64_t size);

    const pils::gpu::VertexBuffer& buffer;
    uint64_t offset;
    uint64_t size;
#endif
};


class GpuResource
{
public:
    GpuResource(GpuResourceManager& manager, ResourceId id);
    virtual ~GpuResource();

    ResourceId id;

protected:
    GpuResourceManager& _manager;
};

class GpuTextureResource : public GpuResource
{
public:
    struct Definition
    {
        const Texture& texture;
    };

    GpuTextureResource(GpuResourceManager& manager, ResourceId id, Definition def);
    GpuTextureResource(GpuResourceManager& manager, GpuTextureResourceHandle&& handle);
    ~GpuTextureResource();

    bool accessible() const { return _handle.get() != nullptr; }
    const GpuTextureResourceHandle& handle() const { return *_handle; }

private:
    std::unique_ptr<GpuTextureResourceHandle> _handle;

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    std::unique_ptr<pils::gpu::Texture> _texture;
    std::unique_ptr<pils::gpu::ImageView> _textureView;
    std::unique_ptr<pils::gpu::Sampler> _sampler;
#endif // UNISIM_GRAPHIC_BACKEND_VK
};


class GpuImageResource : public GpuResource
{
public:
    struct Definition
    {
        int width;
        int height;
        int depth;
        TextureFormat format;
    };

    GpuImageResource(GpuResourceManager& manager, ResourceId id, Definition def);
    ~GpuImageResource();

    void update(const Definition& def);

    bool accessible() const { return _handle.get() != nullptr; }
    const GpuImageResourceHandle& handle() const { return *_handle; }

private:
    std::unique_ptr<GpuImageResourceHandle> _handle;

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    std::unique_ptr<pils::gpu::Image> _image;
    std::unique_ptr<pils::gpu::ImageView> _imageView;
#endif // UNISIM_GRAPHIC_BACKEND_VK
};

class GpuStorageResource : public GpuResource
{
public:
    struct Definition
    {
        std::size_t elemSize;
        std::size_t elemCount;
        void* data;
    };

    GpuStorageResource(GpuResourceManager& manager, ResourceId id, Definition def);
    ~GpuStorageResource();

    void update(const Definition& def);

    bool accessible() const { return _handle.get() != nullptr; }
    const GpuStorageResourceHandle& handle() const { return *_handle; }

private:
    std::unique_ptr<GpuStorageResourceHandle> _handle;
};


class GpuConstantResource : public GpuResource
{
public:
    struct Definition
    {
        std::size_t size;
        void* data;
    };

    GpuConstantResource(GpuResourceManager& manager, ResourceId id, Definition def);
    ~GpuConstantResource();

    void update(const Definition& def);
    void invalidate();

    bool accessible() const { return _handle.get() != nullptr; }
    const GpuConstantResourceHandle& handle() const { return *_handle; }

private:
    std::unique_ptr<GpuConstantResourceHandle> _handle;
};


class GpuGeometryResource : public GpuResource
{
public:
    struct Definition
    {
        // Assumed to be triangle list
        std::vector<glm::vec3> vertices;
    };

    GpuGeometryResource(GpuResourceManager& manager, ResourceId id, Definition def);
    ~GpuGeometryResource();

    bool accessible() const { return _handle.get() != nullptr; }
    const GpuGeometryResourceHandle& handle() const { return *_handle; }

private:
    std::unique_ptr<GpuGeometryResourceHandle> _handle;
};


class GpuBindlessTexture
{
public:
    GpuBindlessTexture(
        const GpuTextureResource& textureResource,
        const Texture& texture);
    ~GpuBindlessTexture();

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint64 handle;
#endif // UNISIM_GRAPHIC_BACKEND_GL
};


class GpuBindlessTextureDescriptor
{
public:
    GpuBindlessTextureDescriptor();
    GpuBindlessTextureDescriptor(const GpuBindlessTexture& bindlessTexture);

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint64 texture;
    GLuint64 padding;
#endif // UNISIM_GRAPHIC_BACKEND_GL
};


class GpuResourceManager
{
public:
    GpuResourceManager(const GpuDevice& gpuDevice);
    ~GpuResourceManager();

    static ResourceId registerStaticResource(const std::string& name);
    ResourceId registerDynamicResource(const std::string& name);

    const GpuDevice& gpuDevice() const { return _gpuDevice; }

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    pils::gpu::BufferSet& bufferSet() { return *_bufferSet; }
#endif // UNISIM_GRAPHIC_BACKEND_VK

    void initialize();
    void garbageCollect();
    void commitAllocations();

    template<typename Resource>
    bool define(ResourceId id, const typename Resource::Definition& definition);

    template<typename Resource>
    bool update(ResourceId id, const typename Resource::Definition& definition);

    template<typename Resource>
    const Resource& get(ResourceId id) const;

    const std::vector<std::shared_ptr<GpuBindlessTexture>>& bindlessTextures() { return _bindlessTextures; }
    BindlessTextureIndex registerBindlessTexture(const GpuTextureResource& textureResource, const Texture& texture);
    void unregisterBindlessTexture(BindlessTextureIndex index);

private:
    void initializeNative();

    const GpuDevice& _gpuDevice;

    static unsigned int _staticResourceCount;
    static std::vector<std::string> _staticNames;

    unsigned int _resourceCount;
    std::vector<std::string> _names;

    std::vector<std::shared_ptr<GpuResource>> _resources;

    bool _isBindlessTextureDirty;
    std::vector<BindlessTextureIndex> _bindlessTextureFreeList;
    std::vector<std::shared_ptr<GpuBindlessTexture>> _bindlessTextures;

#ifdef UNISIM_GRAPHIC_BACKEND_GL
#elif UNISIM_GRAPHIC_BACKEND_VK
    std::unique_ptr<pils::gpu::BufferSet> _bufferSet;
#endif // UNISIM_GRAPHIC_BACKEND_VK
};

template<typename Resource>
bool GpuResourceManager::define(ResourceId id, const typename Resource::Definition& definition)
{
    PILS_ASSERT(id < _resourceCount, "Invalid resource ID");
    PILS_ASSERT(_resources[id].get() == nullptr, "GPU resource already declared for: ", _names[id]);

    _resources[id] = std::make_shared<Resource>(*this, id, definition);

    return true;
}

template<typename Resource>
bool GpuResourceManager::update(ResourceId id, const typename Resource::Definition& definition)
{
    PILS_ASSERT(id < _resourceCount, "Invalid resource ID");
    PILS_ASSERT(_resources[id].get() != nullptr, "Trying to update a resource that has not been defined yet for: ", _names[id]);

    static_cast<Resource*>(_resources[id].get())->update(definition);

    return true;
}

template<typename Resource>
const Resource& GpuResourceManager::get(ResourceId id) const
{
    PILS_ASSERT(id < _resourceCount, "Invalid resource ID");
    PILS_ASSERT(_resources[id].get() != nullptr, "Trying to access a resource that has not been defined yet for: ", _names[id]);

    const Resource* resource = dynamic_cast<const Resource*>(_resources[id].get());
    PILS_ASSERT(resource != nullptr, "Resource is null or not of the correct type for: ", _names[id]);
    PILS_ASSERT(resource->accessible(), "Resource is not available this frame. Did you forget to update: ", _names[id]);

    return *resource;
}

}

#endif // GPURESOURCE_H
