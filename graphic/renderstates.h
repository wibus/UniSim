#ifndef RENDERSTATES_H
#define RENDERSTATES_H

#include <memory>


#ifdef UNISIM_GRAPHIC_BACKEND_VK
namespace pils
{
namespace gpu
{
class RenderPass;
class FrameBuffer;
class VertexState;
class TopologyState;
class ViewportState;
class RasterizationState;
class MultisamplingState;
class BlendState;
class DynamicStates;
}
}
#endif // UNISIM_GRAPHIC_BACKEND_VK


namespace unisim
{

class View;
class GpuDevice;
class GpuGeometryResource;


class RenderStates
{
public:
    RenderStates(const GpuDevice& device);

    void setView(const View& view, bool isFirstPass, bool isLastPass);
    void setGeometry(const GpuGeometryResource* geometry);

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    const pils::gpu::RenderPass& renderPass() const {  return *_renderPass; }
    const pils::gpu::FrameBuffer& frameBuffer() const { return *_frameBuffer; }
    const GpuGeometryResource* geometry() const { return _geometry; }

    const pils::gpu::VertexState& vertexState() const { return *_vertexState; }
    const pils::gpu::TopologyState& topologyState() const { return *_topologyState; }
    const pils::gpu::ViewportState& viewportState() const { return *_viewportState; }
    const pils::gpu::RasterizationState& rasterizationState() const { return *_rasterizationState; }
    const pils::gpu::MultisamplingState& multisamplingState() const { return *_multisamplingState; }
    const pils::gpu::BlendState& blendState() const { return *_blendState; }
    const pils::gpu::DynamicStates& dynamicStates() const { return *_dynamicStates; }
#endif // UNISIM_GRAPHIC_BACKEND_VK

private:
#ifdef UNISIM_GRAPHIC_BACKEND_VK
    const GpuDevice& _gpuDevice;
    std::shared_ptr<pils::gpu::RenderPass> _renderPass;
    const pils::gpu::FrameBuffer* _frameBuffer{nullptr};
    const GpuGeometryResource* _geometry{nullptr};

    std::shared_ptr<pils::gpu::VertexState> _vertexState;
    std::shared_ptr<pils::gpu::TopologyState> _topologyState;
    std::shared_ptr<pils::gpu::ViewportState> _viewportState;
    std::shared_ptr<pils::gpu::RasterizationState> _rasterizationState;
    std::shared_ptr<pils::gpu::MultisamplingState> _multisamplingState;
    std::shared_ptr<pils::gpu::BlendState> _blendState;
    std::shared_ptr<pils::gpu::DynamicStates> _dynamicStates;
#endif // UNISIM_GRAPHIC_BACKEND_VK
};

typedef std::shared_ptr<RenderStates> RenderStatesPtr;


class RenderPassScope
{
public:
    RenderPassScope(const GpuDevice& gpuDevice, const RenderStates& renderStates);
    ~RenderPassScope();

private:
#ifdef UNISIM_GRAPHIC_BACKEND_VK
    const GpuDevice& _gpuDevice;
#endif // UNISIM_GRAPHIC_BACKEND_VK
};

}

#endif // RENDERSTATES_H