#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "gpudevice.h"

#include "PilsCore/Utils/Assert.h"
#include "PilsCore/Gpu/Context.h"
#include "PilsCore/Gpu/Command.h"

#include "gpuresource.h"
#include "gpuprograminterface.h"


namespace unisim
{

GpuDevice::GpuDevice()
{
    _deviceContext.reset(new pils::gpu::Context());
}

GpuDevice::~GpuDevice()
{

}

pils::gpu::CommandList& GpuDevice::commandList() const
{
    PILS_ASSERT(_commandList.get() != nullptr, "No command available to record commands");
    return *_commandList;
}

void GpuDevice::begin()
{
    _commandList->reset();
    _commandList->begin(true);
}

void GpuDevice::end()
{
    _commandList->end();
}

void GpuDevice::bindBuffer(const GpuConstantResource& resource, const GpuProgramConstantBindPoint& bindPoint)
{
    PILS_ASSERT(false, "Not implemented!");
}

void GpuDevice::bindBuffer(const GpuStorageResource& resource, const GpuProgramStorageBindPoint& bindPoint)
{
    PILS_ASSERT(false, "Not implemented!");
}

void GpuDevice::bindTexture(const GpuTextureResource& resource, const GpuProgramTextureBindPoint& bindPoint)
{
    PILS_ASSERT(false, "Not implemented!");
}

void GpuDevice::bindTexture(const GpuImageResource& resource, const GpuProgramTextureBindPoint& unit)
{
    PILS_ASSERT(false, "Not implemented!");
}

void GpuDevice::bindImage(const GpuImageResource& resource, const GpuProgramImageBindPoint& unit)
{
    PILS_ASSERT(false, "Not implemented!");
}

void GpuDevice::dispatch(unsigned int workGroupCountX, unsigned int workGroupCountY, unsigned int workGroupCountZ)
{
    PILS_ASSERT(false, "Not implemented!");
}

void GpuDevice::draw(const GpuGeometryResource& resource)
{
    PILS_ASSERT(false, "Not implemented!");
}

void GpuDevice::clearSwapChain()
{
    PILS_ASSERT(false, "Not implemented!");
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK