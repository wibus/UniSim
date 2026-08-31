#ifndef IMGUIRENDERER_H
#define IMGUIRENDERER_H

#include <memory>

#include <PilsCore/types.h>


#ifdef UNISIM_GRAPHIC_BACKEND_VK
#include <vulkan/vulkan.h>

namespace pils
{
namespace gpu
{
class Device;
}
}
#endif // UNISIM_GRAPHIC_BACKEND_VK


namespace unisim
{

class Window;
class View;
class GpuDevice;


class ImGuiNativeData
{
public:
#ifdef UNISIM_GRAPHIC_BACKEND_GL
    explicit ImGuiNativeData();
#endif // UNISIM_GRAPHIC_BACKEND_GL

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    explicit ImGuiNativeData(const pils::gpu::Device& device);
#endif // UNISIM_GRAPHIC_BACKEND_VK

    ~ImGuiNativeData();

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    VkDescriptorPool descriptorPool() const { return _descriptorPool; }
#endif // UNISIM_GRAPHIC_BACKEND_VK

private:
#ifdef UNISIM_GRAPHIC_BACKEND_VK
    const pils::gpu::Device& _device;
    VkDescriptorPool _descriptorPool;
#endif // UNISIM_GRAPHIC_BACKEND_VK
};


class ImGuiRenderer
{
public:
    ImGuiRenderer(const View& view);
    ~ImGuiRenderer();

    void ImGuiNewFrame() const;

    void render(GpuDevice& gpuDevice, const View& view) const;

private:
    void ImGuiInitNative();
    void ImGuiNewFrameNative() const;
    void renderNative(GpuDevice& gpuDevice, const View& view) const;

    const View& _view;

    std::unique_ptr<class ImGuiNativeData> _imguiNativeData;
};

}

#endif // IMGUIRENDERER_H
