#ifndef RESOURCE_H
#define RESOURCE_H

#include <map>
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
#define DefineResource(name) ResourceId ResourceName(name) = ResourceManager::registerStaticResource(#name)


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

    GLuint bufferId;
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

    GLuint bufferId;
};


struct GPUBindlessTexture
{
    GPUBindlessTexture() :
        texture(0),
        padding(0)
    {}

    GPUBindlessTexture(GLuint64 tex) :
        texture(tex),
        padding(0)
    {}

    GLuint64 texture;
    GLuint64 padding;
};


class PathTracerInterface
{
public:
    PathTracerInterface(GLuint programId);

    GLuint programId() const { return _programId; }

    bool declareUbo(const std::string& blockName);
    bool declareSsbo(const std::string& blockName);

    GLuint getUboBindPoint(const std::string& blockName) const;
    GLuint getSsboBindPoint(const std::string& blockName) const;

    GLuint grabTextureUnit();
    void resetTextureUnits();

    bool isValid() const { return _programId != 0; }

private:
    GLuint _programId;
    GLuint _nextTextureUnit;

    GLuint _nextUboBindPoint;
    std::map<std::string, GLuint> _uboBindPoints;

    GLuint _nextSsboBindPoint;
    std::map<std::string, GLuint> _ssboBindPoints;
};


class PathTracerProvider
{
public:
    PathTracerProvider();
    virtual ~PathTracerProvider();

    virtual std::vector<GLuint> pathTracerModules() const;

    virtual bool definePathTracerModules(GraphicContext& context);

    virtual void setPathTracerResources(GraphicContext& context, PathTracerInterface& interface) const;

    uint64_t hash() const { return _hash; }

    template<typename T>
    static uint64_t hashVal(const T& data, uint64_t seed)
    {
        return combineHashes(seed, std::hash<std::string_view>()(
            std::string_view((char*)&data, sizeof (T))));
    }

    template<typename T>
    static uint64_t hashVec(const std::vector<T>& data, uint64_t seed)
    {
        return combineHashes(seed, std::hash<std::string_view>()(
            std::string_view((char*)data.data(), sizeof (T) * data.size())));
    }

    static uint64_t combineHashes(uint64_t h1, uint64_t h2)
    {
        return h1 ^ (h2 + 0x9e3779b9 + (h1<<6) + (h1>>2));
    }

protected:
    uint64_t _hash;
};


class ResourceManager
{
public:
    ResourceManager();
    ~ResourceManager();

    static ResourceId registerStaticResource(const std::string& name);
    ResourceId registerDynamicResource(const std::string& name);

    void initialize();

    template<typename Resource>
    bool define(ResourceId id, const typename Resource::Definition& definition);

    template<typename Resource>
    const Resource& get(ResourceId id) const;

    void registerPathTracerProvider(const std::shared_ptr<PathTracerProvider>& provider);
    std::vector<GLuint> pathTracerModules() const;
    void setPathTracerResources(GraphicContext& context, PathTracerInterface& interface) const;

    uint64_t pathTracerHash() const;

private:
    static unsigned int _staticResourceCount;
    static std::vector<std::string> _staticNames;

    unsigned int _resourceCount;
    std::vector<std::string> _names;

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
