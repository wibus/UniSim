#ifdef UNISIM_GRAPHIC_BACKEND_GL

#include "renderstates.h"


namespace unisim
{

RenderStates::RenderStates(const GpuDevice& device)
{

}

void RenderStates::setView(const View& view, bool isFirstPass, bool isLastPass)
{

}

void RenderStates::setGeometry(const GpuGeometryResource* geometry)
{

}

RenderPassScope::RenderPassScope(const GpuDevice& gpuDevice, const RenderStates& renderStates)
{

}

RenderPassScope::~RenderPassScope()
{

}

}

#endif // UNISIM_GRAPHIC_BACKEND_GL