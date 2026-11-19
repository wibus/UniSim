#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "renderstates.h"

#include <PilsCore/Gpu/Pass.h>
#include <PilsCore/Gpu/Command.h>

#include "view.h"
#include "gpuresource.h"


namespace unisim
{

RenderStates::RenderStates(const View& view) :
    _renderPass(view.renderPass()),
    _frameBuffer((view.window().imageIndex() != -1) ? view.currentFrameBuffer() : view.frameBufferPrototype())
{
    uint32_t width = _frameBuffer.vkExtent().width;
    uint32_t height = _frameBuffer.vkExtent().height;

    _vertexState.reset(new pils::gpu::VertexState());
    _topologyState.reset(new pils::gpu::TopologyState());
    _viewportState.reset(new pils::gpu::ViewportState(width, height));
    _rasterizationState.reset(new pils::gpu::RasterizationState());
    _multisamplingState.reset(new pils::gpu::MultisamplingState());
    _blendState.reset(new pils::gpu::BlendState(_frameBuffer.attachementCount()));
    _dynamicStates.reset(new pils::gpu::DynamicStates());
}


RenderPassScope::RenderPassScope(const GpuDevice& gpuDevice, const RenderStates& renderStates, const GpuGeometryResource& geometry) :
    _gpuDevice(gpuDevice)
{
    pils::gpu::CommandList& cmdList = _gpuDevice.commandList();

    cmdList.beginRenderPass(renderStates.renderPass(), renderStates.frameBuffer());
    cmdList.setViewport(renderStates.viewportState());

    const GpuGeometryResourceHandle& vertexHandle = geometry.roHandle();
    cmdList.bindVertexBuffer(*vertexHandle.buffer, 0, vertexHandle.mapping->offset);
}

RenderPassScope::~RenderPassScope()
{
    _gpuDevice.commandList().endRenderPass();
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK