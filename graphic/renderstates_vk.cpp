#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "renderstates.h"

#include <PilsCore/Gpu/Pass.h>
#include <PilsCore/Gpu/Command.h>
#include <PilsCore/Gpu/Context.h>

#include "view.h"
#include "gpuresource.h"


namespace unisim
{

RenderStates::RenderStates(const GpuDevice& device) :
    _gpuDevice(device)
{
    _vertexState.reset(new pils::gpu::VertexState());
    _topologyState.reset(new pils::gpu::TopologyState());
    _viewportState.reset(new pils::gpu::ViewportState(1, 1));
    _rasterizationState.reset(new pils::gpu::RasterizationState());
    _multisamplingState.reset(new pils::gpu::MultisamplingState());
    _blendState.reset(new pils::gpu::BlendState(0));
    _dynamicStates.reset(new pils::gpu::DynamicStates());
}

void RenderStates::setView(const View& view, bool isFirstPass, bool isLastPass)
{
    const Viewport& vp = view.viewport();

    if (!_renderPass)
    {
        const pils::gpu::Device& device = _gpuDevice.context().device();

        std::vector<pils::gpu::Attachment> attachements = view.frameBufferPrototype().attachements();
        PILS_ASSERT(attachements.size() == 1, "Only one attachement per render pass supported");
        attachements[0].setLoadOperation(isFirstPass ? pils::gpu::LoadOperation::DontCare : pils::gpu::LoadOperation::Load);
        attachements[0].setToPresent(isLastPass);

        _renderPass.reset(new pils::gpu::RenderPass(device, attachements));
        _blendState.reset(new pils::gpu::BlendState(attachements.size()));
    }

    if (view.window().imageIndex() != -1)
        _frameBuffer = &view.currentFrameBuffer();
    else
        _frameBuffer = nullptr;

    if (vp.width != _viewportState->vkViewport().width || vp.height != _viewportState->vkViewport().height)
    {
        _viewportState.reset(new pils::gpu::ViewportState(vp.width, vp.height));
    }
}

void RenderStates::setGeometry(const GpuGeometryResource* geometry)
{
    if (_geometry != geometry)
    {
        _vertexState.reset(new pils::gpu::VertexState());
        _geometry = geometry;
    }
}

RenderPassScope::RenderPassScope(const GpuDevice& gpuDevice, const RenderStates& renderStates) :
    _gpuDevice(gpuDevice)
{
    pils::gpu::CommandList& cmdList = _gpuDevice.commandList();

    cmdList.beginRenderPass(renderStates.renderPass(), renderStates.frameBuffer());
    cmdList.setViewport(renderStates.viewportState());
    cmdList.setScissor(renderStates.viewportState());

    if (const GpuGeometryResource* geometry = renderStates.geometry())
    {
        const GpuGeometryResourceHandle& vertexHandle = geometry->roHandle();
        cmdList.bindVertexBuffer(*vertexHandle.buffer, 0, vertexHandle.mapping->offset);
    }
}

RenderPassScope::~RenderPassScope()
{
    _gpuDevice.commandList().endRenderPass();
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK