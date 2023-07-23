#ifndef RESOURCE_H
#define RESOURCE_H

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include <cassert>

#include <GL/glew.h>


namespace unisim
{

class Texture;
class GraphicContext;


typedef unsigned int ResourceId;
const ResourceId Invalid_ResourceId = ~0x0;

#define ResourceName(name) ResourceId_##name
#define DeclareResource(name) extern ResourceId ResourceName(name)
#define DefineResource(name) ResourceId ResourceName(name) = ResourceManager::registerResource(#name)


class GpuResource
{
public:
    GpuResource(ResourceId id);
    virtual ~GpuResource();

    virtual void bind() {}

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
    ~GpuTextureResource();

    GLuint texId;
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

    GLuint texId;
};


class GpuBindlessResource : public GpuResource
{
public:
    struct Definition
    {
        const Texture* texture;
        GLuint texId;
    };

    GpuBindlessResource(ResourceId id, Definition def);
    ~GpuBindlessResource();

    GLuint64 handle;
};


class PathTracerProvider
{
public:
    PathTracerProvider();
    virtual ~PathTracerProvider();

    std::vector<GLuint> pathTracerModules() const { return _pathTracerModules; }

    virtual bool definePathTracerModules(GraphicContext& context) = 0;

    virtual void setPathTracerResources(GraphicContext& context, GLuint programId, GLuint& nextTextureUnit) const;

protected:
    std::vector<GLuint> _pathTracerModules;
};


class ResourceManager
{
public:
    ResourceManager();
    ~ResourceManager();

    static ResourceId registerResource(const std::string& name);

    void initialize();

    template<typename Resource>
    bool define(ResourceId id, const typename Resource::Definition& definition);

    template<typename Resource>
    const Resource& get(ResourceId id) const;

    void registerPathTracerProvider(const std::shared_ptr<PathTracerProvider>& provider);
    std::vector<GLuint> pathTracerModules() const;
    void setPathTracerResources(GraphicContext& context, GLuint programId, GLuint& nextTextureUnit) const;

private:
    static unsigned int _resourceCount;
    static std::vector<std::string> _names;

    std::vector<std::shared_ptr<GpuResource>> _resources;

    std::vector<std::shared_ptr<PathTracerProvider>> _providers;
};

template<typename Resource>
bool ResourceManager::define(ResourceId id, const typename Resource::Definition& definition)
{
    assert(id < _resourceCount);

    _resources[id] = std::make_shared<Resource>(id, definition);

    return true;
}

template<typename Resource>
const Resource& ResourceManager::get(ResourceId id) const
{
    assert(id < _resourceCount);
    assert(_resources[id].get() != nullptr);

    const Resource* resource = dynamic_cast<const Resource*>(_resources[id].get());
    assert(resource/* resource is null or not of the correct type */);

    return *resource;
}

}

#endif // RESOURCE_H
