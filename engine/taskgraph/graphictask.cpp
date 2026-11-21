#include "graphictask.h"

#include "../system/profiler.h"

#include "../graphic/gpudevice.h"
#include "../graphic/imguirenderer.h"
#include "../graphic/renderstates.h"
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


DefineProfilePoint(ImGui_Render);
DefineProfilePointGpu(ImGui_Render);

Ui::Ui(const ImGuiRenderer& imguiRenderer) :
    GraphicTask("UI"),
    _imGuiRenderer(imguiRenderer)
{

}

bool Ui::defineShaders(GraphicContext& context)
{
    _renderStates.reset(new RenderStates(context.device));
    _renderStates->setView(context.view, false, true);

    return true;
}

void Ui::render(GraphicContext& context)
{
    Profile(ImGui_Render);
    ProfileGpu(ImGui_Render);

    _renderStates->setView(context.view, false, true);

    RenderPassScope renderPassScope(context.device, *_renderStates);

    _imGuiRenderer.render(context.device);
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
