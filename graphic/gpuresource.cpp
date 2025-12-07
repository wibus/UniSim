#include "gpuresource.h"

#include <algorithm>

#include "pathtracer.h"


namespace unisim
{

DefineResource(FullScreenTriangle);


GpuResource::GpuResource(ResourceId id) :
    id(id)
{
}

GpuResource::~GpuResource()
{
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

    std::vector<glm::vec3> fullScreenTriangleVerts;
    fullScreenTriangleVerts.push_back({-1.0f, -1.0f,  0.0f});
    fullScreenTriangleVerts.push_back({ 3.0f, -1.0f,  0.0f});
    fullScreenTriangleVerts.push_back({-1.0f,  3.0f,  0.0f});
    define<GpuGeometryResource>(ResourceName(FullScreenTriangle), {fullScreenTriangleVerts});
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
