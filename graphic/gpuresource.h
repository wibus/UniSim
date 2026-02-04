#ifndef GPURESOURCE_H
#define GPURESOURCE_H

#include <vector>
#include <memory>
#include <string>

#include <GLM/glm.hpp>

#include <PilsCore/Utils/Assert.h>

#ifdef UNISIM_GRAPHIC_BACKEND_GL
#include "gpuresource_gl.h"
#endif // UNISIM_GRAPHIC_BACKEND_GL

#ifdef UNISIM_GRAPHIC_BACKEND_VK
#include "gpuresource_vk.h"
#endif // UNISIM_GRAPHIC_BACKEND_VK

namespace unisim
{

struct Texture;
class GraphicContext;
class CompiledGpuProgramInterface;


typedef unsigned int ResourceId;
const ResourceId Invalid_ResourceId = ~0x0;

#define ResourceName(name) ResourceId_##name
#define DeclareResource(name) extern ResourceId ResourceName(name)
#define DefineResource(name) ResourceId ResourceName(name) = GpuResourceManager::registerStaticResource(#name)


class GpuResource
{
public:
    GpuResource(ResourceId id);
    virtual ~GpuResource();

    ResourceId id;
};

class GpuTextureResource : public GpuResource
{
public:
    struct Definition
    {
        const Texture& texture;
    };

    GpuTextureResource(ResourceId id, Definition def);
    GpuTextureResource(GpuTextureResourceHandle&& handle);
    ~GpuTextureResource();

    const GpuTextureResourceHandle& handle() const { return *_handle; }

private:
    std::unique_ptr<GpuTextureResourceHandle> _handle;
};


class GpuImageResource : public GpuResource
{
public:
    struct Definition
    {
        int width;
        int height;
        GLint format;
    };

    GpuImageResource(ResourceId id, Definition def);
    ~GpuImageResource();

    void update(const Definition& def) const;

    const GpuImageResourceHandle& handle() const { return *_handle; }

private:
    std::unique_ptr<GpuImageResourceHandle> _handle;
};


class GpuBindlessResource : public GpuResource
{
public:
    struct Definition
    {
        const Texture* texture;
        const GpuTextureResource& textureResource;
    };

    GpuBindlessResource(ResourceId id, Definition def);
    ~GpuBindlessResource();

    const GpuBindlessResourceHandle& handle() const { return *_handle; }

private:
    std::unique_ptr<GpuBindlessResourceHandle> _handle;
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

    GpuStorageResource(ResourceId id, Definition def);
    ~GpuStorageResource();

    void update(const Definition& def) const;

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

    GpuConstantResource(ResourceId id, Definition def);
    ~GpuConstantResource();

    void update(const Definition& def) const;

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

    GpuGeometryResource(ResourceId id, Definition def);
    ~GpuGeometryResource();

    const GpuGeometryResourceHandle& handle() const { return *_handle; }

private:
    std::unique_ptr<GpuGeometryResourceHandle> _handle;
};


class GpuResourceManager
{
public:
    GpuResourceManager();
    ~GpuResourceManager();

    static ResourceId registerStaticResource(const std::string& name);
    ResourceId registerDynamicResource(const std::string& name);

    void initialize();

    template<typename Resource>
    bool define(ResourceId id, const typename Resource::Definition& definition);

    template<typename Resource>
    bool update(ResourceId id, const typename Resource::Definition& definition);

    template<typename Resource>
    const Resource& get(ResourceId id) const;

private:
    static unsigned int _staticResourceCount;
    static std::vector<std::string> _staticNames;

    unsigned int _resourceCount;
    std::vector<std::string> _names;

    std::vector<std::shared_ptr<GpuResource>> _resources;
};

template<typename Resource>
bool GpuResourceManager::define(ResourceId id, const typename Resource::Definition& definition)
{
    PILS_ASSERT(id < _resourceCount, "Invalid resource ID");
    PILS_ASSERT(_resources[id].get() == nullptr, "GPU resource already declared");

    _resources[id] = std::make_shared<Resource>(id, definition);

    return true;
}

template<typename Resource>
bool GpuResourceManager::update(ResourceId id, const typename Resource::Definition& definition)
{
    PILS_ASSERT(id < _resourceCount, "Invalid resource ID");
    PILS_ASSERT(_resources[id].get() != nullptr, "Trying to update a resource that has not been defined yet.");

    static_cast<Resource*>(_resources[id].get())->update(definition);

    return true;
}

template<typename Resource>
const Resource& GpuResourceManager::get(ResourceId id) const
{
    PILS_ASSERT(id < _resourceCount, "Invalid resource ID");
    PILS_ASSERT(_resources[id].get() != nullptr, "Trying to access a resource that has not been defined yet.");

    const Resource* resource = dynamic_cast<const Resource*>(_resources[id].get());
    assert(resource/* resource is null or not of the correct type */);

    return *resource;
}

}

#endif // GPURESOURCE_H
