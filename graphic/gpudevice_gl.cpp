#ifdef UNISIM_GRAPHIC_BACKEND_GL

#include "gpudevice.h"

#include "gpuresource.h"
#include "gpuprograminterface.h"


namespace unisim
{

GpuDevice::GpuDevice()
{
}

GpuDevice::~GpuDevice()
{

}

uint32_t GpuDevice::frameInFlightCount() const
{
    return 1;
}

void GpuDevice::setupInitialization()
{

}

void GpuDevice::finalizeInitialization()
{

}

void GpuDevice::begin()
{
}

void GpuDevice::end()
{
}

void GpuDevice::dispatch(unsigned int workGroupCountX, unsigned int workGroupCountY, unsigned int workGroupCountZ)
{
    glDispatchCompute(workGroupCountX, workGroupCountY, workGroupCountZ);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void GpuDevice::draw(const GpuGeometryResource& resource)
{
    glBindVertexArray(resource.handle().vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void GpuDevice::clearSwapChain()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

}

#endif // UNISIM_GRAPHIC_BACKEND_GL
