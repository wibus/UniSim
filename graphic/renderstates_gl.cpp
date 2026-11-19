#ifdef UNISIM_GRAPHIC_BACKEND_GL

#include "renderstates.h"


namespace unisim
{

RenderStates::RenderStates(const View& view)
{
}


RenderPassScope::RenderPassScope(const GpuDevice& gpuDevice, const RenderStates& renderStates, const GpuGeometryResource& geometry)
{

}

RenderPassScope::~RenderPassScope()
{

}

}

#endif // UNISIM_GRAPHIC_BACKEND_GL