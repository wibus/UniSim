#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "window.h"

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include <PilsCore/Utils/Assert.h>
#include <PilsCore/Gpu/Context.h>
#include <PilsCore/Gpu/Surface.h>

#include "graphic.h"


namespace unisim
{

bool Window::InitGlfwNative()
{
    if(GLFW_FALSE == glfwVulkanSupported())
        return false;

    // This tells GLFW to not create an OpenGL context with the window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    return true;
}

bool Window::CreateGlfwSurface()
{
    int width, height;
    glfwGetFramebufferSize(_glfwWindow, &width, &height);

    // Create window surface, looks a lot like a Vulkan function ( and not GLFW function )
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult ret = glfwCreateWindowSurface(_gpuDevice.context().instance().vkInstance() , _glfwWindow, nullptr, &surface);

    if( VK_SUCCESS != ret )
        return false;

    _surface.reset(new pils::gpu::Surface(_gpuDevice.context().instance(), surface));
    _swapchain.reset(new pils::gpu::Swapchain(_gpuDevice.context().device(), *_surface, width, height));

    return true;
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK
