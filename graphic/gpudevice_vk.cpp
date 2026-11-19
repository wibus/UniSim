#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "gpudevice.h"

#include "PilsCore/Utils/Assert.h"
#include "PilsCore/Gpu/Context.h"
#include "PilsCore/Gpu/Command.h"
#include "PilsCore/Gpu/Synchronization.h"

#include "gpuresource.h"
#include "programinterface.h"


namespace unisim
{

GpuDevice::GpuDevice()
{
    _deviceContext.reset(new pils::gpu::Context());

    pils::gpu::DescriptorPoolSettings poolSettings;
    poolSettings.enableBindless = true;
    poolSettings.maxSetCount = 1000;
    poolSettings.typeToCount[vk::DescriptorType::eUniformBuffer] = 100;
    poolSettings.typeToCount[vk::DescriptorType::eStorageBuffer] = 100;
    poolSettings.typeToCount[vk::DescriptorType::eStorageImage] = 100;
    poolSettings.typeToCount[vk::DescriptorType::eCombinedImageSampler] = 1000;
    _descriptorPool.reset(new pils::gpu::DescriptorPool(_deviceContext->device(), poolSettings));

    _garbageBins.resize(frameInFlightCount());
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

    if (fence)
    {
        PILS_ASSERT(_garbageBins[_garbageBinIndex].fence.get() == nullptr, "Garbage bin ", _garbageBinIndex, " is not reset.");
        PILS_ASSERT(_garbageBins[_garbageBinIndex].callbacks.empty(), "Garbage bin ", _garbageBinIndex, " is not reset.");
        _garbageBins[_garbageBinIndex].fence = fence;
    }
}

void GpuDevice::setupInitialization()
{
    setCommandList(std::make_shared<pils::gpu::CommandList>(_deviceContext->device()));
    setFrameFence(std::make_shared<pils::gpu::Fence>(_deviceContext->device()));
    _commandList->begin(true);
}

void GpuDevice::abortInitialization()
{
    _commandList->end();
    setCommandList({});
    setFrameFence({});
}

void GpuDevice::finalizeInitialization()
{
    _commandList->end();
    _deviceContext->device().submit(*_commandList, {}, {}, &*_frameFence);
    _deviceContext->device().waitForFence(*_frameFence);

    setCommandList({});
    setFrameFence({});
}

void GpuDevice::garbageCollect()
{
    _garbageBinIndex = (_garbageBinIndex + 1) % _garbageBins.size();

    if (!_garbageBins[_garbageBinIndex].fence)
    {
        PILS_ASSERT(_garbageBins[_garbageBinIndex].callbacks.empty(), "Garbage bin ", _garbageBinIndex, " has no fence but has resources to dispose.");
        return;
    }

    PILS_ASSERT(_garbageBins[_garbageBinIndex].fence->isSignaled(), "Garbage bin ", _garbageBinIndex, " was ready to be collected, but its fence is not signaled. Its resources will be disposed nonetheless.");

    for (const auto& callback : _garbageBins[_garbageBinIndex].callbacks)
        callback();

    _garbageBins[_garbageBinIndex].fence.reset();
    _garbageBins[_garbageBinIndex].callbacks.clear();
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

void GpuDevice::dispatch(unsigned int workGroupCountX, unsigned int workGroupCountY, unsigned int workGroupCountZ)
{
    _commandList->dispatch(workGroupCountX, workGroupCountY, workGroupCountZ);
}

void GpuDevice::draw(const GpuGeometryResource& resource)
{
    _commandList->draw(resource.vertexCount());
}

void GpuDevice::clearSwapChain()
{
    //PILS_ASSERT(false, "Not implemented!");
}

void GpuDevice::dispose(std::function<void(void)> disposeCallback)
{
    PILS_ASSERT(_garbageBinIndex < _garbageBins.size(), "Invalid active garbage bin index: ", _garbageBinIndex);
    PILS_ASSERT(_garbageBins[_garbageBinIndex].fence.get() != nullptr, "Active garbage bin ", _garbageBinIndex, " is missing a fence.");
    _garbageBins[_garbageBinIndex].callbacks.push_back(disposeCallback);
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK