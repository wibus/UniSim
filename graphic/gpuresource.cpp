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

void GpuResourceManager::resetPathTracerProviders()
{
    _pathTracerProviders.clear();
}

void GpuResourceManager::registerPathTracerProvider(const std::shared_ptr<PathTracerProvider>& provider)
{
    _pathTracerProviders.push_back(provider);
}

void GpuResourceManager::setPathTracerModules(const std::vector<std::shared_ptr<PathTracerModule>>& modules)
{
    _pathTracerModules = modules;

    // Remove duplicated modules
    std::sort( _pathTracerModules.begin(), _pathTracerModules.end() );
    _pathTracerModules.erase( std::unique( _pathTracerModules.begin(), _pathTracerModules.end() ), _pathTracerModules.end());
}

void GpuResourceManager::bindPathTracerResources(GraphicContext& context, PathTracerInterface& interface) const
{
    for(const auto& provider : _pathTracerProviders)
        provider->bindPathTracerResources(context, interface);
}

uint64_t GpuResourceManager::pathTracerHash() const
{
    uint64_t hash;
    for(const auto& provider : _pathTracerProviders)
        hash = PathTracerProvider::combineHashes(hash, provider->hash());
    return hash;
}


}
