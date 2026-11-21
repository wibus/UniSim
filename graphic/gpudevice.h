#ifndef GPUDEVICE_H
#define GPUDEVICE_H

#include <cstdint>

#ifdef UNISIM_GRAPHIC_BACKEND_VK
#include <memory>
#include <vector>
#include <functional>

namespace pils
{
namespace gpu
{
class Context;
class CommandList;
class Fence;
class DescriptorPool;
}
}
#endif // UNISIM_GRAPHIC_BACKEND_VK

namespace unisim
{

class GpuGeometryResource;


class GpuDevice
{
public:
    GpuDevice();
    ~GpuDevice();

    uint32_t frameInFlightCount() const;

    void setupInitialization();
    void abortInitialization();
    void finalizeInitialization();

    void garbageCollect();

    void begin();
    void end();

    void dispatch(unsigned int workGroupCountX, unsigned int workGroupCountY = 1, unsigned int workGroupCountZ = 1);
    void draw(const GpuGeometryResource& resource);

    void clearSwapChain();

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    pils::gpu::Context& context() { return *_deviceContext; }
    const pils::gpu::Context& context() const { return *_deviceContext; }

    pils::gpu::DescriptorPool& descriptorPool() const { return *_descriptorPool; }
    void dispose(std::function<void(void)> disposeCallback);
    template<typename T>
    void dispose(const std::shared_ptr<T> resource)
    {
        dispose([ptr = resource](){ });
    }

    pils::gpu::CommandList& commandList() const;
    void setCommandList(const std::shared_ptr<pils::gpu::CommandList>& commandList);

    const std::shared_ptr<pils::gpu::Fence>& frameFence() const;
    void setFrameFence(const std::shared_ptr<pils::gpu::Fence>& fence);
#endif // UNISIM_GRAPHIC_BACKEND_VK

private:
#ifdef UNISIM_GRAPHIC_BACKEND_VK
    std::unique_ptr<pils::gpu::Context> _deviceContext;
    std::unique_ptr<pils::gpu::DescriptorPool> _descriptorPool;
    std::shared_ptr<pils::gpu::CommandList> _commandList;
    std::shared_ptr<pils::gpu::Fence> _frameFence;

    struct GarbageBin
    {
        std::shared_ptr<pils::gpu::Fence> fence;
        std::vector<std::function<void(void)>> callbacks;
    };
    std::vector<GarbageBin> _garbageBins;
    uint32_t _garbageBinIndex;

#endif // UNISIM_GRAPHIC_BACKEND_VK
};

}

#endif // GPUDEVICE_H
