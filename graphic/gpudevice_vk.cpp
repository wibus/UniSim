#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "gpudevice.h"

#include "PilsCore/Utils/Assert.h"
#include "PilsCore/Gpu/Context.h"
#include "PilsCore/Gpu/Command.h"
#include "PilsCore/Gpu/Synchronization.h"

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

uint32_t GpuDevice::frameInFlightCount() const
{
    return 2;
}

pils::gpu::CommandList& GpuDevice::commandList() const
{
    PILS_ASSERT(_commandList.get() != nullptr, "No command list available to record commands");
    return *_commandList;
}

void GpuDevice::setCommandList(const std::shared_ptr<pils::gpu::CommandList>& commandList)
{
    PILS_ASSERT(_commandList.get() == nullptr || commandList.get() == nullptr, "Command list should not be overriden.");
    _commandList = commandList;
}

const std::shared_ptr<pils::gpu::Fence>& GpuDevice::frameFence() const
{
    PILS_ASSERT(_frameFence.get() != nullptr, "No fence available for current frame");
    return _frameFence;
}

void GpuDevice::setFrameFence(const std::shared_ptr<pils::gpu::Fence>& fence)
{
    PILS_ASSERT(_frameFence.get() == nullptr || fence.get() == nullptr, "Frame fence should not be overriden.");
    _frameFence = fence;
}

void GpuDevice::setupInitialization()
{
    setCommandList(std::make_shared<pils::gpu::CommandList>(_deviceContext->device()));
    setFrameFence(std::make_shared<pils::gpu::Fence>(_deviceContext->device()));
    _commandList->begin(true);
}

void GpuDevice::finalizeInitialization()
{
    _commandList->end();
    _deviceContext->device().submit(*_commandList, {}, {}, &*_frameFence);
    _deviceContext->device().waitForFence(*_frameFence);

    setCommandList({});
    setFrameFence({});
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

void GpuDevice::bindBuffer(const GpuStorageResource& resource, const GpuProgramBindlessBindPoint& bindPoint)
{
    // Do nothing
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