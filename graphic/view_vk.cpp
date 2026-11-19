#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "view.h"

#include <PilsCore/Utils/Assert.h>

#include <PilsCore/Gpu/Context.h>
#include <PilsCore/Gpu/Pass.h>
#include <PilsCore/Gpu/Surface.h>


namespace unisim
{

void View::resetViewportNative()
{
    _frameBuffers.clear();
    _renderPass.reset();
}

void View::resizeViewportNative()
{
    const pils::gpu::Device& device = _window.gpuDevice().context().device();
    const pils::gpu::Swapchain& swapchain = _window.swapchain();

    if (swapchain.imageCount() == 0)
    {
        PILS_FATAL("Swapchain contains no image. Cannot create the main render pass and frame buffers.");
        return;
    }

    std::vector<pils::gpu::Attachment> renderPassAttachements;
    renderPassAttachements.emplace_back(swapchain.image(0), swapchain.imageView(0));
    _renderPass.reset(new pils::gpu::RenderPass(device, renderPassAttachements));

    for (pils::pilsU32 i = 0; i < swapchain.imageCount(); ++i)
    {
        std::vector<pils::gpu::Attachment> frameBufferAttachements;
        frameBufferAttachements.emplace_back(swapchain.image(i), swapchain.imageView(i));
        _frameBuffers.emplace_back(new pils::gpu::FrameBuffer(device, *_renderPass, frameBufferAttachements));
    }
}

void View::setViewportNative() const
{

}

const pils::gpu::FrameBuffer& View::currentFrameBuffer() const
{
    PILS_ASSERT(_window.imageIndex() >= 0, "Swapchain image index is undefined");
    PILS_ASSERT(_window.imageIndex() < _frameBuffers.size(), "No framebuffer defined for this image index");
    return *_frameBuffers[_window.imageIndex()];
}

const pils::gpu::FrameBuffer& View::frameBufferPrototype() const
{
    PILS_ASSERT(!_frameBuffers.empty(), "Frame buffers have not been define yet");
    return *_frameBuffers[0];
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK
