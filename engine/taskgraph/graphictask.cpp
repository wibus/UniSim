#include "graphictask.h"

#include "../system/profiler.h"

#include "../graphic/gpudevice.h"
#include "../graphic/view.h"

namespace unisim
{

GraphicTask::GraphicTask(const std::string& name) :
    _name(name)
{
}

GraphicTask::~GraphicTask()
{
}


ClearSwapChain::ClearSwapChain() :
    GraphicTask("Clear")
{

}

DefineProfilePointGpu(Clear);

void ClearSwapChain::render(GraphicContext& context)
{
    ProfileGpu(Clear);

    context.view.setViewport();
    context.device.clearSwapChain();
}

}
