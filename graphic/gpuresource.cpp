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
    _names(_staticNames)
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

    initializeNative();

    std::vector<glm::vec3> fullScreenTriangleVerts;
    fullScreenTriangleVerts.push_back({-1.0f, -1.0f,  0.0f});
    fullScreenTriangleVerts.push_back({ 3.0f, -1.0f,  0.0f});
    fullScreenTriangleVerts.push_back({-1.0f,  3.0f,  0.0f});
    define<GpuGeometryResource>(ResourceName(FullScreenTriangle), {fullScreenTriangleVerts});
}


}
