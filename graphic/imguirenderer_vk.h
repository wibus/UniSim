#ifdef UNISIM_GRAPHIC_BACKEND_VK

#ifndef IMGUIRENDERER_VK_H
#define IMGUIRENDERER_VK_H

#include <vulkan/vulkan.h>


namespace pils
{
namespace gpu
{
class Device;
}
}


namespace unisim
{

class ImGuiNativeData
{
public:
    explicit ImGuiNativeData(const pils::gpu::Device& device);

    VkDescriptorPool descriptorPool() const { return _descriptorPool; }

private:
    VkDescriptorPool _descriptorPool;
};

}

#endif // WINDOW_VK_H

#endif // IMGUIRENDERER_VK_H