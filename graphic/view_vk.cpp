#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "view.h"

#include <PilsCore/Utils/Assert.h>

#include <PilsCore/Gpu/Context.h>
#include <PilsCore/Gpu/Pass.h>
#include <PilsCore/Gpu/Surface.h>

#include "graphic.h"


namespace unisim
{

void View::resizeViewportNative()
{
    _frameBuffer.reset();
    _renderPass.reset();

    const pils::gpu::Device& device = _window.gpuDevice().context().device();
    const pils::gpu::Swapchain& swapchain = _window.swapchain();

    if (swapchain.imageCount() == 0)
    {
        PILS_ERROR("Swapchain contains no image. Cannot create the main render pass and frame buffers.");
        return;
    }

    std::vector<pils::gpu::Attachment> renderPassAttachements;
    renderPassAttachements.emplace_back(swapchain.image(0), swapchain.imageView(0));
    _renderPass.reset(new pils::gpu::RenderPass(device, renderPassAttachements));

    for (pils::pilsU32 i = 0; i < swapchain.imageCount(); ++i)
    {
        std::vector<pils::gpu::Attachment> frameBufferAttachements;
        frameBufferAttachements.emplace_back(swapchain.image(i), swapchain.imageView(i));
        _frameBuffer.reset(new pils::gpu::FrameBuffer(device, *_renderPass, frameBufferAttachements));
    }
}

void View::setViewportNative() const
{

}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK
