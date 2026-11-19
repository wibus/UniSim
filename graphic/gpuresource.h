#ifndef GPURESOURCE_H
#define GPURESOURCE_H

#include <vector>
#include <memory>
#include <string>
#include <list>

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
class Device;
class BufferSet;
class StorageBuffer;
class UniformBuffer;
class ConstantBuffer;
class VertexBuffer;
class Image;
class Texture;
class ImageView;
class Sampler;
class Descriptor;
class Mapping;
class Buffer;
class Fence;
}
}

namespace unisim
{

class ProgramInterfaceTemplate;
class ProgramInterface;
class Program;
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


enum class ResourceState
{
    Undefined,
    ReadOnly,
    ReadWrite
};


class GpuBindlessTextureResourceHandle
{
public:
    GpuBindlessTextureResourceHandle();

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint64 handle;
#elif UNISIM_GRAPHIC_BACKEND_VK
    std::shared_ptr<ResourceState> state;
    std::shared_ptr<pils::gpu::Descriptor> descriptor;
#endif
};

class GpuTextureResourceHandle
{
public:
    GpuTextureResourceHandle();

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint texId;
    GLenum dimension;
#elif UNISIM_GRAPHIC_BACKEND_VK
    std::shared_ptr<ResourceState> state;
    std::shared_ptr<pils::gpu::Image> image;
    std::shared_ptr<pils::gpu::Sampler> sampler;
    std::shared_ptr<pils::gpu::ImageView> imageView;
    std::shared_ptr<pils::gpu::Descriptor> descriptor;
#endif
};

class GpuImageResourceHandle
{
public:
    GpuImageResourceHandle();

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint texId;
    GLenum internalFormat;
    GLenum dimension;
#elif UNISIM_GRAPHIC_BACKEND_VK
    std::shared_ptr<ResourceState> state;
    std::shared_ptr<pils::gpu::Image> image;
    std::shared_ptr<pils::gpu::ImageView> imageView;
    std::shared_ptr<pils::gpu::Descriptor> descriptor;
#endif
};

class GpuStorageResourceHandle
{
public:
    GpuStorageResourceHandle();

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint bufferId;
#elif UNISIM_GRAPHIC_BACKEND_VK
    GpuStorageResourceHandle(
        pils::gpu::StorageBuffer* buffer,
        const std::shared_ptr<pils::gpu::Mapping>& mapping);

    std::shared_ptr<ResourceState> state;
    pils::gpu::Buffer* buffer{nullptr};
    std::shared_ptr<pils::gpu::Mapping> mapping;
    std::shared_ptr<pils::gpu::Descriptor> descriptor;
#endif
};

class GpuConstantResourceHandle
{
public:
    GpuConstantResourceHandle();

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint bufferId;
#elif UNISIM_GRAPHIC_BACKEND_VK
    GpuConstantResourceHandle(
        pils::gpu::ConstantBuffer* buffer,
        const std::shared_ptr<pils::gpu::Mapping>& mapping);

    std::shared_ptr<ResourceState> state;
    pils::gpu::Buffer* buffer{nullptr};
    std::shared_ptr<pils::gpu::Mapping> mapping;
    std::shared_ptr<pils::gpu::Descriptor> descriptor;
#endif
};

class GpuGeometryResourceHandle
{
public:
    GpuGeometryResourceHandle();

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    GLuint vao;
    GLuint vbo;
#elif UNISIM_GRAPHIC_BACKEND_VK
    GpuGeometryResourceHandle(
        pils::gpu::VertexBuffer* buffer,
        const std::shared_ptr<pils::gpu::Mapping>& mapping);

    std::shared_ptr<ResourceState> state;
    pils::gpu::Buffer* buffer{nullptr};
    std::shared_ptr<pils::gpu::Mapping> mapping;
    std::shared_ptr<pils::gpu::Descriptor> descriptor;
#endif
};


class GpuResource
{
public:
    GpuResource(GpuResourceManager& manager, ResourceId id, ResourceState initialState);
    virtual ~GpuResource();

    ResourceId id;

protected:
    GpuResourceManager& _manager;
    std::shared_ptr<ResourceState> _state;
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
    const GpuTextureResourceHandle& roHandle() const { return *_handle; }

private:
    std::shared_ptr<GpuTextureResourceHandle> _handle;
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

    bool accessible() const { return _imageHandle.get() != nullptr && _textureHandle.get() != nullptr; }
    const GpuImageResourceHandle& rwHandle() const { return *_imageHandle; }
    const GpuTextureResourceHandle& roHandle() const { return *_textureHandle; }

private:
    void createHandles(const Definition& def);

    std::shared_ptr<GpuImageResourceHandle> _imageHandle;
    std::shared_ptr<GpuTextureResourceHandle> _textureHandle;
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
    const GpuStorageResourceHandle& rwHandle() const { return *_handle; }

private:
    void createHandle(const Definition& def);

    std::shared_ptr<GpuStorageResourceHandle> _handle;
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
    const GpuConstantResourceHandle& roHandle() const { return *_handle; }

private:
    std::shared_ptr<GpuConstantResourceHandle> _handle;
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
    const GpuGeometryResourceHandle& roHandle() const { return *_handle; }

    uint32_t vertexCount() const { return _vertexCount; }

private:
    std::shared_ptr<GpuGeometryResourceHandle> _handle;
    uint32_t _vertexCount;
};


class GpuBindlessTexture
{
public:
    GpuBindlessTexture();
    GpuBindlessTexture(
        const GpuTextureResource& textureResource,
        const Texture& texture);
    ~GpuBindlessTexture();

    bool accessible() const { return _handle.get() != nullptr; }
    const GpuBindlessTextureResourceHandle& roHandle() const { return *_handle; }

private:
    std::shared_ptr<GpuBindlessTextureResourceHandle> _handle;
};


class GpuResourceManager
{
public:
    GpuResourceManager(GpuDevice& gpuDevice);
    ~GpuResourceManager();

    static ResourceId registerStaticResource(const std::string& name);
    ResourceId registerDynamicResource(const std::string& name);

    GpuDevice& gpuDevice() const { return _gpuDevice; }

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

    const ProgramInterfaceTemplate& persistentPiTemplate() const { return *_persistentPiTemplate; }
    const ProgramInterfaceTemplate& framePiTemplate() const { return *_framePiTemplate; }

    bool compileAndSetPassPi(const Program& program);

    const std::vector<std::shared_ptr<GpuBindlessTexture>>& bindlessTextures() { return _bindlessTextures; }
    BindlessTextureIndex registerBindlessTexture(const GpuTextureResource& textureResource, const Texture& texture);
    void unregisterBindlessTexture(BindlessTextureIndex index);

private:
    void initializeNative();

    GpuDevice& _gpuDevice;

    static unsigned int _staticResourceCount;
    static std::vector<std::string> _staticNames;

    unsigned int _resourceCount;
    std::vector<std::string> _names;

    std::vector<std::shared_ptr<GpuResource>> _resources;

    std::shared_ptr<ProgramInterfaceTemplate> _persistentPiTemplate;
    std::shared_ptr<ProgramInterface> _persistentPi;

    std::shared_ptr<ProgramInterfaceTemplate> _framePiTemplate;
    std::shared_ptr<ProgramInterface> _framePi;

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
