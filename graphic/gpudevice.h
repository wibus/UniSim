#ifndef GPUDEVICE_H
#define GPUDEVICE_H

#ifdef UNISIM_GRAPHIC_BACKEND_VK
#include <memory>
#include <vector>

namespace pils
{
namespace gpu
{
class Context;
class CommandList;
}
}
#endif // UNISIM_GRAPHIC_BACKEND_VK

namespace unisim
{

class GpuConstantResource;
class GpuStorageResource;
class GpuTextureResource;
class GpuImageResource;
class GpuGeometryResource;

struct GpuProgramConstantBindPoint;
struct GpuProgramStorageBindPoint;
struct GpuProgramTextureBindPoint;
struct GpuProgramImageBindPoint;


class GpuDevice
{
public:
    GpuDevice();
    ~GpuDevice();

    void begin();
    void end();

    void bindBuffer(const GpuConstantResource& resource, const GpuProgramConstantBindPoint& bindPoint);
    void bindBuffer(const GpuStorageResource& resource, const GpuProgramStorageBindPoint& bindPoint);
    void bindTexture(const GpuTextureResource& resource, const GpuProgramTextureBindPoint& unit);
    void bindTexture(const GpuImageResource& resource, const GpuProgramTextureBindPoint& unit);
    void bindImage(const GpuImageResource& resource, const GpuProgramImageBindPoint& unit);

    void dispatch(unsigned int workGroupCountX, unsigned int workGroupCountY = 1, unsigned int workGroupCountZ = 1);
    void draw(const GpuGeometryResource& resource);

    void clearSwapChain();

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    pils::gpu::Context& context() { return *_deviceContext; }
    const pils::gpu::Context& context() const { return *_deviceContext; }

    pils::gpu::CommandList& commandList() const;
    void setCommandList(const std::shared_ptr<pils::gpu::CommandList>& commandList) { _commandList = commandList; }
#endif // UNISIM_GRAPHIC_BACKEND_VK

private:
#ifdef UNISIM_GRAPHIC_BACKEND_VK
    std::unique_ptr<pils::gpu::Context> _deviceContext;
    std::shared_ptr<pils::gpu::CommandList> _commandList;
#endif // UNISIM_GRAPHIC_BACKEND_VK
};

}

#endif // GPUDEVICE_H
