#include "imguirenderer.h"

#include <imgui/imgui_impl_glfw.h>

#include "window.h"
#include "view.h"

#include "imguirenderer.h"


namespace unisim
{

ImGuiRenderer::ImGuiRenderer(const View& view) :
    _view(view)
{
    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    ImGuiInitNative();
}

ImGuiRenderer::~ImGuiRenderer()
{

}

void ImGuiRenderer::ImGuiNewFrame() const
{
    ImGuiNewFrameNative();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiRenderer::render(GpuDevice& gpuDevice, const View& view) const
{
    ImGui::Render();
    renderNative(gpuDevice, view);
}

}
