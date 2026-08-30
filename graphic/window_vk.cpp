#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "window.h"

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include <PilsCore/Utils/Assert.h>
#include <PilsCore/Gpu/Context.h>
#include <PilsCore/Gpu/Surface.h>
#include <PilsCore/Gpu/Command.h>

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

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult ret = glfwCreateWindowSurface(_gpuDevice.context().instance().vkInstance() , _glfwWindow, nullptr, &surface);

    if( VK_SUCCESS != ret )
        return false;

    _surface.reset(new pils::gpu::Surface(_gpuDevice.context().instance(), surface));
    _swapchain.reset(new pils::gpu::Swapchain(_gpuDevice.context().device(), *_surface, width, height));

    for(uint32_t i = 0; i < _swapchain->frameInFlightCount(); ++i)
    {
        _frameFences.emplace_back(new pils::gpu::Fence(_gpuDevice.context().device(), true));
        _frameCommandLists.emplace_back(new pils::gpu::CommandList(_gpuDevice.context().device()));
    }

    return true;
}

void Window::resetViewportNative()
{
    vkDeviceWaitIdle(_gpuDevice.context().device().vkDevice());
}

void Window::resizeViewportNative()
{
    _swapchain->createSwapchain(_width, _height);
}

bool Window::newFrameNative()
{
    const pils::gpu::Device& device = _gpuDevice.context().device();

    device.waitForFence(*_frameFences[_swapchain->frameIndex()]);

    _gpuDevice.setCommandList(_frameCommandLists[_swapchain->frameIndex()]);

    bool acquired = _swapchain->acquireImage();

    if (_swapchain->needsResize())
    {
        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(_glfwWindow, &width, &height);
        onResize(width, height);

        acquired = _swapchain->acquireImage();
        PILS_ASSERT(acquired, "Swapchain resized by still cannot acquire image");
        PILS_ASSERT(!_swapchain->needsResize(), "Swapchain still needs to be reized after a resize");
    }

    if(acquired && !_swapchain->needsResize())
    {
        device.resetFence(*_frameFences[_swapchain->frameIndex()]);
    }
    else
    {
        return false;
    }

    return true;
}

bool Window::presentNative()
{
    _gpuDevice.setCommandList({});

    if(_frameCommandLists[_swapchain->frameIndex()])
    {
        _gpuDevice.context().device().submit(
            *_frameCommandLists[_swapchain->frameIndex()],
            {&_swapchain->imageAvailableSemaphore(_swapchain->frameIndex())},
            {&_swapchain->renderFinishedSemaphore(_swapchain->imageIndex())},
            &*_frameFences[_swapchain->frameIndex()]
        );

        bool presented = _swapchain->presentImage();

        if (!presented)
            return false;
    }

    return true;
}

void Window::closeNative()
{
    vkDeviceWaitIdle(_gpuDevice.context().device().vkDevice());
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK
